#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include "card_picker.hpp"

#include <commctrl.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <set>

#include "effect_editor_ids.h"

namespace kingdom {

std::wstring fold_card_search(std::wstring_view text) {
    if (text.empty()) return {};
    const int size = static_cast<int>(text.size());
    const int length = FoldStringW(MAP_COMPOSITE, text.data(), size, nullptr, 0);
    std::wstring folded(length > 0 ? static_cast<std::size_t>(length) : 0, L'\0');
    if (!length || !FoldStringW(MAP_COMPOSITE, text.data(), size, folded.data(), length))
        folded.assign(text);
    CharLowerBuffW(folded.data(), static_cast<DWORD>(folded.size()));
    std::wstring key;
    for (const wchar_t ch : folded) {
        if (ch == L'ß') key += L"ss";
        else if (ch < 0x300 || ch > 0x36F) key += ch;
    }
    const auto first = key.find_first_not_of(L" \t");
    return first == key.npos ? std::wstring{} : key.substr(first, key.find_last_not_of(L" \t") - first + 1);
}

namespace {

constexpr UINT_PTR preview_timer = 1;
constexpr std::size_t replacement_limit = 20;

struct Picker {
    const wchar_t* title;
    const CardTargets& cards;
    const CardTargets& current;
    CardSelection mode;
    const AcceptCards& accept;
    std::filesystem::path previews;
    CardTargets selected{};
    std::vector<std::pair<HTREEITEM, const CardTarget*>> visible{};
    std::set<std::wstring, std::less<>> expanded{};
    const CardTarget* hovered = nullptr;
    const CardTarget* displayed = nullptr;
    ULONG_PTR graphics = 0;
    std::unique_ptr<Gdiplus::Image> image{};
    bool rendering = false;

    bool multiple() const { return mode != CardSelection::single; }
    bool has(const CardTarget* card) const {
        return std::find(selected.begin(), selected.end(), card) != selected.end();
    }

    ~Picker() {
        image.reset();
        if (graphics) Gdiplus::GdiplusShutdown(graphics);
    }
};

const CardTarget* card_at(HWND tree, HTREEITEM item) {
    if (!item) return nullptr;
    TVITEMW value{};
    value.mask = TVIF_PARAM;
    value.hItem = item;
    return TreeView_GetItem(tree, &value) ? reinterpret_cast<const CardTarget*>(value.lParam) : nullptr;
}

bool current_scope(HWND dialog) {
    return SendDlgItemMessageW(dialog, IDC_CARD_SCOPE, CB_GETCURSEL, 0, 0) == 0;
}

void preview(HWND dialog, Picker& picker, const CardTarget* card) {
    if (!card || picker.displayed == card) return;
    picker.displayed = card;
    SetDlgItemTextW(dialog, IDC_CARD_TITLE, card->title);
    std::wstring info = card->expansion;
    info += L"\r\n";
    info += card->types;
    if (*card->cost) {
        info += L"\r\nKosten  ";
        info += card->cost;
    }
    SetDlgItemTextW(dialog, IDC_CARD_INFO, info.c_str());
    SetDlgItemTextW(dialog, IDC_CARD_RULES, *card->rules ? card->rules : L"");
    picker.image.reset();
    if (picker.graphics) {
        const auto path = picker.previews / (std::string(card->id) + ".png");
        auto image = std::make_unique<Gdiplus::Image>(path.c_str());
        if (image->GetLastStatus() == Gdiplus::Ok && image->GetWidth() && image->GetHeight())
            picker.image = std::move(image);
    }
    InvalidateRect(GetDlgItem(dialog, IDC_CARD_IMAGE), nullptr, TRUE);
}

void selection_status(HWND dialog, const Picker& picker) {
    if (picker.mode == CardSelection::ordered) {
        SendDlgItemMessageW(dialog, IDC_CARD_SELECTION, LB_RESETCONTENT, 0, 0);
        for (const auto* card : picker.selected)
            SendDlgItemMessageW(dialog, IDC_CARD_SELECTION, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(card->title));
    }
    const auto status = std::to_wstring(picker.visible.size()) + L" Treffer · " +
        std::to_wstring(picker.selected.size()) +
        (picker.mode == CardSelection::ordered ? L" / 20 gewählt" : L" gewählt");
    SetDlgItemTextW(dialog, IDC_CARD_STATUS, status.c_str());
    EnableWindow(GetDlgItem(dialog, IDOK), !picker.selected.empty());
}

void toggle_card(HWND dialog, Picker& picker, HTREEITEM item) {
    const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
    const auto* card = card_at(tree, item);
    if (!card) return;
    const auto position = std::find(picker.selected.begin(), picker.selected.end(), card);
    const bool adding = position == picker.selected.end();
    if (adding && picker.mode == CardSelection::ordered && picker.selected.size() == replacement_limit) {
        SetDlgItemTextW(dialog, IDC_CARD_STATUS, L"Taifun erlaubt bis zu 20 Ersatzstapel.");
        return;
    }
    if (adding) picker.selected.push_back(card);
    else picker.selected.erase(position);
    TreeView_SetCheckState(tree, item, adding);
    selection_status(dialog, picker);
}

HTREEITEM insert_row(HWND tree, HTREEITEM parent, const wchar_t* title, const CardTarget* card,
                     bool multiple, bool checked) {
    TVINSERTSTRUCTW row{};
    row.hParent = parent;
    row.hInsertAfter = TVI_LAST;
    row.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
    row.item.pszText = const_cast<wchar_t*>(title);
    row.item.lParam = reinterpret_cast<LPARAM>(card);
    row.item.stateMask = TVIS_STATEIMAGEMASK;
    row.item.state = multiple && card ? INDEXTOSTATEIMAGEMASK(checked ? 2 : 1) : 0;
    const auto item = TreeView_InsertItem(tree, &row);
    // The control initializes the state image itself on insertion.
    row.item.hItem = item;
    row.item.mask = TVIF_STATE;
    if (item) TreeView_SetItem(tree, &row.item);
    return item;
}

void render_cards(HWND dialog, Picker& picker) {
    const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
    const bool only_current = current_scope(dialog);
    const auto* focused = card_at(tree, TreeView_GetSelection(tree));
    std::array<wchar_t, 256> search{};
    GetDlgItemTextW(dialog, IDC_CARD_QUERY, search.data(), static_cast<int>(search.size()));
    const auto query = fold_card_search(search.data());
    KillTimer(dialog, preview_timer);
    picker.hovered = nullptr;
    picker.rendering = true;
    SendMessageW(tree, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(tree);
    picker.visible.clear();
    if (!picker.multiple()) picker.selected.clear();
    HTREEITEM group = nullptr, focus = nullptr;
    std::wstring_view expansion;
    for (const auto* card : only_current ? picker.current : picker.cards) {
        if (std::wstring_view(card->search).find(query) == std::wstring_view::npos) continue;
        if (!group || expansion != card->expansion) {
            if (group && (only_current || !query.empty() || picker.expanded.find(expansion) != picker.expanded.end()))
                TreeView_Expand(tree, group, TVE_EXPAND);
            expansion = card->expansion;
            group = insert_row(tree, TVI_ROOT, card->expansion, nullptr, picker.multiple(), false);
        }
        const auto item = insert_row(tree, group, card->title, card, picker.multiple(), picker.has(card));
        if (item) picker.visible.emplace_back(item, card);
        if (card == focused) focus = item;
    }
    if (group && (only_current || !query.empty() || picker.expanded.find(expansion) != picker.expanded.end()))
        TreeView_Expand(tree, group, TVE_EXPAND);
    if (focus) TreeView_SelectItem(tree, focus);
    else if (picker.multiple() && !query.empty() && !picker.visible.empty())
        TreeView_SelectItem(tree, picker.visible.front().first);
    picker.rendering = false;
    SendMessageW(tree, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(tree, nullptr, TRUE);
    ShowWindow(GetDlgItem(dialog, IDC_CARD_ALL), only_current && picker.mode == CardSelection::multiple ? SW_SHOW : SW_HIDE);
    EnableWindow(GetDlgItem(dialog, IDC_CARD_ALL), !picker.visible.empty());
    preview(dialog, picker, card_at(tree, TreeView_GetSelection(tree)));
    selection_status(dialog, picker);
}

LRESULT CALLBACK tree_proc(HWND tree, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
    try {
        const auto dialog = GetParent(tree);
        auto& picker = *reinterpret_cast<Picker*>(GetWindowLongPtrW(dialog, DWLP_USER));
        if (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK) {
            TVHITTESTINFO hit{};
            hit.pt.x = static_cast<short>(LOWORD(lparam));
            hit.pt.y = static_cast<short>(HIWORD(lparam));
            const auto item = TreeView_HitTest(tree, &hit);
            const auto* card = card_at(tree, item);
            if (message == WM_MOUSEMOVE && card != picker.hovered) {
                picker.hovered = card;
                if (card) {
                    TRACKMOUSEEVENT tracking{sizeof tracking, TME_LEAVE, tree, 0};
                    TrackMouseEvent(&tracking);
                    SetTimer(dialog, preview_timer, 140, nullptr);
                } else KillTimer(dialog, preview_timer);
            } else if (message != WM_MOUSEMOVE && picker.multiple() && card && (hit.flags & TVHT_ONITEMSTATEICON)) {
                if (message == WM_LBUTTONDOWN) {
                    SetFocus(tree);
                    TreeView_SelectItem(tree, item);
                    toggle_card(dialog, picker, item);
                }
                return 0;
            }
        } else if (message == WM_MOUSELEAVE) {
            picker.hovered = nullptr;
            KillTimer(dialog, preview_timer);
        } else if (message == WM_KEYDOWN && wparam == VK_SPACE && picker.multiple()) {
            if (!(lparam & (1 << 30))) toggle_card(dialog, picker, TreeView_GetSelection(tree));
            return 0;
        }
    } catch (...) {
        return 0;
    }
    return DefSubclassProc(tree, message, wparam, lparam);
}

void change_order(HWND dialog, Picker& picker, unsigned command) {
    auto row = SendDlgItemMessageW(dialog, IDC_CARD_SELECTION, LB_GETCURSEL, 0, 0);
    if (row < 0 || static_cast<std::size_t>(row) >= picker.selected.size()) return;
    if (command == IDC_CARD_REMOVE) {
        const auto* card = picker.selected[row];
        picker.selected.erase(picker.selected.begin() + row);
        for (const auto& [item, visible_card] : picker.visible) {
            if (visible_card == card) TreeView_SetCheckState(GetDlgItem(dialog, IDC_CARD_TREE), item, FALSE);
        }
        if (row == static_cast<LRESULT>(picker.selected.size())) --row;
    } else {
        const auto next = row + (command == IDC_CARD_UP ? -1 : 1);
        if (next < 0 || static_cast<std::size_t>(next) >= picker.selected.size()) return;
        std::swap(picker.selected[row], picker.selected[next]);
        row = next;
    }
    selection_status(dialog, picker);
    SendDlgItemMessageW(dialog, IDC_CARD_SELECTION, LB_SETCURSEL, row, 0);
}

void paint_preview(const DRAWITEMSTRUCT& draw, const Picker& picker) {
    FillRect(draw.hDC, &draw.rcItem, GetSysColorBrush(COLOR_BTNFACE));
    if (!picker.image) return;
    const auto& rect = draw.rcItem;
    const auto scale = (std::min)(static_cast<double>(rect.right - rect.left) / picker.image->GetWidth(),
                                  static_cast<double>(rect.bottom - rect.top) / picker.image->GetHeight());
    const auto width = static_cast<INT>(picker.image->GetWidth() * scale);
    const auto height = static_cast<INT>(picker.image->GetHeight() * scale);
    Gdiplus::Graphics graphics(draw.hDC);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.DrawImage(picker.image.get(), (rect.left + rect.right - width) / 2,
                       (rect.top + rect.bottom - height) / 2, width, height);
}

INT_PTR CALLBACK picker_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    try {
        if (message == WM_INITDIALOG) {
            auto& picker = *reinterpret_cast<Picker*>(lparam);
            SetWindowLongPtrW(dialog, DWLP_USER, lparam);
            SetWindowTextW(dialog, picker.title);
            const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
            if (picker.multiple())
                SetWindowLongPtrW(tree, GWL_STYLE, GetWindowLongPtrW(tree, GWL_STYLE) | TVS_CHECKBOXES);
            SetWindowSubclass(tree, tree_proc, 1, 0);
            for (const auto* scope : {L"Aktuelles Königreich", L"Alle Karten"})
                SendDlgItemMessageW(dialog, IDC_CARD_SCOPE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(scope));
            const bool has_current = !picker.current.empty();
            SendDlgItemMessageW(dialog, IDC_CARD_SCOPE, CB_SETCURSEL, has_current ? 0 : 1, 0);
            EnableWindow(GetDlgItem(dialog, IDC_CARD_SCOPE), has_current);
            ShowWindow(GetDlgItem(dialog, IDC_CARD_REPEAT), picker.mode == CardSelection::multiple ? SW_SHOW : SW_HIDE);
            for (const auto id : {IDC_CARD_SELECTION, IDC_CARD_UP, IDC_CARD_DOWN, IDC_CARD_REMOVE})
                ShowWindow(GetDlgItem(dialog, id), picker.mode == CardSelection::ordered ? SW_SHOW : SW_HIDE);
            if (picker.mode == CardSelection::multiple && !has_current)
                SetDlgItemTextW(dialog, IDC_CARD_HINT,
                    L"Bei Zufallskönigreichen kann der Effektdialog alle gezogenen Stapel automatisch verdoppeln.");
            Gdiplus::GdiplusStartupInput input;
            if (Gdiplus::GdiplusStartup(&picker.graphics, &input, nullptr) != Gdiplus::Ok) picker.graphics = 0;
            render_cards(dialog, picker);
            return TRUE;
        }
        auto* state = reinterpret_cast<Picker*>(GetWindowLongPtrW(dialog, DWLP_USER));
        if (!state) return FALSE;
        auto& picker = *state;
        if (message == WM_TIMER && wparam == preview_timer) {
            KillTimer(dialog, preview_timer);
            preview(dialog, picker, picker.hovered);
            return TRUE;
        }
        if (message == WM_DRAWITEM && wparam == IDC_CARD_IMAGE) {
            paint_preview(*reinterpret_cast<const DRAWITEMSTRUCT*>(lparam), picker);
            return TRUE;
        }
        if (message == WM_DESTROY) {
            const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
            RemoveWindowSubclass(tree, tree_proc, 1);
            if (picker.multiple()) {
                const auto images = TreeView_SetImageList(tree, nullptr, TVSIL_STATE);
                if (images) ImageList_Destroy(images);
            }
            return FALSE;
        }
        if (message == WM_NOTIFY && wparam == IDC_CARD_TREE) {
            const auto& change = *reinterpret_cast<const NMTREEVIEWW*>(lparam);
            if (change.hdr.code == TVN_SELCHANGEDW) {
                const auto* card = card_at(change.hdr.hwndFrom, change.itemNew.hItem);
                if (!picker.multiple()) {
                    picker.selected.clear();
                    if (card) picker.selected.push_back(card);
                }
                if (!picker.rendering) {
                    preview(dialog, picker, card);
                    if (!picker.multiple()) selection_status(dialog, picker);
                }
            } else if (change.hdr.code == TVN_ITEMEXPANDEDW && !picker.rendering && !change.itemNew.lParam) {
                std::array<wchar_t, 128> name{};
                TVITEMW item{};
                item.hItem = change.itemNew.hItem;
                item.mask = TVIF_TEXT;
                item.pszText = name.data();
                item.cchTextMax = static_cast<int>(name.size());
                if (TreeView_GetItem(change.hdr.hwndFrom, &item)) {
                    if (change.action == TVE_EXPAND) picker.expanded.insert(name.data());
                    else picker.expanded.erase(name.data());
                }
            }
            return FALSE;
        }
        if (message != WM_COMMAND) return FALSE;
        const auto control = LOWORD(wparam);
        const auto event = HIWORD(wparam);
        if ((control == IDC_CARD_QUERY && event == EN_CHANGE) ||
            (control == IDC_CARD_SCOPE && event == CBN_SELCHANGE)) {
            render_cards(dialog, picker);
        } else if (control == IDC_CARD_ALL && current_scope(dialog) && picker.mode == CardSelection::multiple) {
            const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
            for (const auto& [item, card] : picker.visible) {
                if (!(TreeView_GetItemState(tree, TreeView_GetParent(tree, item), TVIS_EXPANDED) & TVIS_EXPANDED))
                    continue;
                if (!picker.has(card)) picker.selected.push_back(card);
                TreeView_SetCheckState(tree, item, TRUE);
            }
            selection_status(dialog, picker);
        } else if (control == IDC_CARD_UP || control == IDC_CARD_DOWN || control == IDC_CARD_REMOVE) {
            change_order(dialog, picker, control);
        } else if (control == IDOK) {
            if (picker.selected.empty()) return TRUE;
            const auto error = picker.accept(picker.selected, IsDlgButtonChecked(dialog, IDC_CARD_REPEAT) == BST_CHECKED);
            if (!error.empty()) SetDlgItemTextW(dialog, IDC_CARD_STATUS, error.c_str());
            else EndDialog(dialog, IDOK);
        } else if (control == IDCANCEL) {
            EndDialog(dialog, IDCANCEL);
        } else {
            return FALSE;
        }
        return TRUE;
    } catch (...) {
        EndDialog(dialog, -1);
        return TRUE;
    }
}

}

void pick_cards(HWND owner, const wchar_t* title, const CardTargets& cards, const CardTargets& current_kingdom,
                CardSelection mode, const AcceptCards& accept) {
    INITCOMMONCONTROLSEX controls{sizeof controls, ICC_TREEVIEW_CLASSES};
    HMODULE module{};
    if (!InitCommonControlsEx(&controls) ||
        !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(picker_proc), &module)) return;
    std::array<wchar_t, 32768> path{};
    const auto length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    std::filesystem::path previews;
    if (length && length < path.size()) previews = std::filesystem::path(path.data()).parent_path() / L"card-previews";
    Picker picker{title, cards, current_kingdom, mode, accept, std::move(previews)};
    if (DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_CARD_PICKER), owner, picker_proc,
                        reinterpret_cast<LPARAM>(&picker)) == -1) {
        MessageBoxW(owner, L"Die Kartenauswahl konnte nicht geöffnet werden.", title, MB_OK | MB_ICONWARNING);
    }
}

}
