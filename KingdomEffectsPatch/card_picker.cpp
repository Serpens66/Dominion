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
#include "dialog_skin.hpp"
#include "menu_labels.generated.hpp"

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

constexpr std::size_t replacement_limit = 20;

struct Picker {
    const wchar_t* title;
    const CardTargets& cards;
    const CardTargets& current;
    CardSelection mode;
    const AcceptCards& accept;
    CardTargets selected{};
    std::vector<std::pair<HTREEITEM, const CardTarget*>> visible{};
    std::set<std::wstring, std::less<>> expanded{};
    const CardTarget* displayed = nullptr;
    ULONG_PTR graphics = 0;
    ResourceImage image{};
    CardPreviewLoader loader;
    bool rendering = false;
    bool first_render = true;
    bool show_rules = false;
    std::unique_ptr<DialogSkin> skin{};
    HIMAGELIST expansion_images = nullptr;
    std::map<std::string, int, std::less<>> expansion_icons{};

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

std::string expansion_icon(const CardTarget& card) {
    return std::string("exp-") + (std::string_view(card.expansion_id) == "basic" ? "base" : card.expansion_id);
}

void layout_preview(HWND dialog, const Picker& picker) {
    const auto* card = picker.displayed;
    const bool can_switch = picker.image && card && *card->rules;
    const bool show_face = picker.image && !(can_switch && picker.show_rules);
    const int bottom = can_switch ? 204 : 222;
    ShowWindow(GetDlgItem(dialog, IDC_CARD_TITLE), card && !show_face ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dialog, IDC_CARD_INFO), card && !show_face ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dialog, IDC_CARD_RULES), show_face ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(dialog, IDC_CARD_IMAGE), show_face ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dialog, IDC_CARD_TEXT), can_switch ? SW_SHOW : SW_HIDE);
    SetDlgItemTextW(dialog, IDC_CARD_TEXT, show_face ? L"Kartentext lesen" : L"Karte anzeigen");
    picker.skin->place(IDC_CARD_IMAGE, {217, 29, 378, bottom});
    picker.skin->place(IDC_CARD_RULES, {217, card ? 84 : 29, 378, bottom});
    InvalidateRect(GetDlgItem(dialog, IDC_CARD_INFO), nullptr, TRUE);
    InvalidateRect(GetDlgItem(dialog, IDC_CARD_IMAGE), nullptr, TRUE);
}

void preview(HWND dialog, Picker& picker, const CardTarget* card) {
    if (card && picker.displayed == card) return;
    picker.displayed = card;
    SetDlgItemTextW(dialog, IDC_CARD_TITLE, card ? card->title : L"");
    SetDlgItemTextW(dialog, IDC_CARD_IMAGE, card ? card->title : L"");
    const auto info = card ? std::wstring(card->expansion) + L"\r\n" + card->types + L"\r\n" + card->cost : L"";
    SetDlgItemTextW(dialog, IDC_CARD_INFO, info.c_str());
    SetDlgItemTextW(dialog, IDC_CARD_RULES, card ? card->rules : picker.visible.empty()
        ? L"Keine passenden Karten." : L"Öffne eine Erweiterung für die Kartenvorschau.");
    picker.image.reset();
    picker.loader.request(card ? card->id : "");
    layout_preview(dialog, picker);
}

void layout_list(HWND dialog, const Picker& picker, bool only_current) {
    const bool scope = picker.mode == CardSelection::multiple && !picker.current.empty();
    const bool bulk = picker.mode == CardSelection::multiple;
    const bool ordered = picker.mode == CardSelection::ordered;
    const bool hint = bulk && picker.current.size() < 10;
    ShowWindow(GetDlgItem(dialog, IDC_CARD_SCOPE), scope ? SW_SHOW : SW_HIDE);
    picker.skin->place(IDC_CARD_QUERY, {8, 42, scope ? 109 : 210, 56});
    ShowWindow(GetDlgItem(dialog, IDC_CARD_ALL), bulk && only_current ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dialog, IDC_CARD_REPEAT), bulk ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dialog, IDC_CARD_HINT), hint ? SW_SHOW : SW_HIDE);
    for (const auto id : {IDC_CARD_SELECTION, IDC_CARD_UP, IDC_CARD_DOWN, IDC_CARD_REMOVE})
        ShowWindow(GetDlgItem(dialog, id), ordered ? SW_SHOW : SW_HIDE);
    const int bottom = ordered ? 166 : bulk ? (hint ? 184 : 203) : 222;
    picker.skin->place(IDC_CARD_TREE, {8, 59, 210, bottom});
    if (bulk) {
        const int actions = bottom + 3;
        picker.skin->place(IDC_CARD_ALL, {8, actions, 106, actions + 15});
        picker.skin->place(IDC_CARD_REPEAT, {only_current ? 108 : 8, actions, 210, actions + 15});
    }
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
                     bool multiple, bool checked, int image = I_IMAGENONE) {
    TVINSERTSTRUCTW row{};
    row.hParent = parent;
    row.hInsertAfter = TVI_LAST;
    row.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    row.item.pszText = const_cast<wchar_t*>(title);
    row.item.lParam = reinterpret_cast<LPARAM>(card);
    row.item.stateMask = TVIS_STATEIMAGEMASK;
    row.item.state = multiple && card ? INDEXTOSTATEIMAGEMASK(checked ? 2 : 1) : 0;
    row.item.iImage = row.item.iSelectedImage = image;
    const auto item = TreeView_InsertItem(tree, &row);
    // The control initializes the state image itself on insertion.
    row.item.hItem = item;
    row.item.mask = TVIF_STATE;
    if (item) TreeView_SetItem(tree, &row.item);
    return item;
}

const CardTarget* first_open_card(HWND tree, const Picker& picker) {
    for (const auto& [item, card] : picker.visible) {
        if (TreeView_GetItemState(tree, TreeView_GetParent(tree, item), TVIS_EXPANDED) & TVIS_EXPANDED)
            return card;
    }
    return nullptr;
}

void render_cards(HWND dialog, Picker& picker) {
    const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
    const bool only_current = current_scope(dialog);
    const auto* focused = card_at(tree, TreeView_GetSelection(tree));
    std::array<wchar_t, 256> search{};
    GetDlgItemTextW(dialog, IDC_CARD_QUERY, search.data(), static_cast<int>(search.size()));
    const auto query = fold_card_search(search.data());
    picker.rendering = true;
    SendMessageW(tree, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(tree);
    picker.visible.clear();
    if (!picker.multiple()) picker.selected.clear();
    HTREEITEM group = nullptr, focus = nullptr;
    std::wstring_view expansion;
    if (picker.first_render && !only_current && !picker.cards.empty())
        picker.expanded.insert(picker.cards.front()->expansion);
    picker.first_render = false;
    const auto expand_group = [&] {
        if (group && (only_current || !query.empty() || picker.expanded.find(expansion) != picker.expanded.end()))
            TreeView_Expand(tree, group, TVE_EXPAND);
    };
    for (const auto* card : only_current ? picker.current : picker.cards) {
        if (std::wstring_view(card->search).find(query) == std::wstring_view::npos) continue;
        if (!group || expansion != card->expansion) {
            expand_group();
            expansion = card->expansion;
            const auto key = expansion_icon(*card);
            auto [icon, added] = picker.expansion_icons.try_emplace(key, I_IMAGENONE);
            if (added) icon->second = picker.skin->add_icon(picker.expansion_images, key);
            group = insert_row(tree, TVI_ROOT, card->expansion, nullptr, picker.multiple(), false, icon->second);
        }
        const auto item = insert_row(tree, group, card->title, card, picker.multiple(), picker.has(card));
        if (item) picker.visible.emplace_back(item, card);
        if (card == focused) focus = item;
    }
    expand_group();
    if (focus) TreeView_SelectItem(tree, focus);
    picker.rendering = false;
    SendMessageW(tree, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(tree, nullptr, TRUE);
    layout_list(dialog, picker, only_current);
    EnableWindow(GetDlgItem(dialog, IDC_CARD_ALL), !picker.visible.empty());
    preview(dialog, picker, focus ? card_at(tree, focus) : first_open_card(tree, picker));
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
            if (message == WM_MOUSEMOVE) {
                if (card) preview(dialog, picker, card);
            } else if (picker.multiple() && card && (hit.flags & TVHT_ONITEMSTATEICON)) {
                if (message == WM_LBUTTONDOWN) {
                    SetFocus(tree);
                    TreeView_SelectItem(tree, item);
                    toggle_card(dialog, picker, item);
                }
                return 0;
            }
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
    SetDCBrushColor(draw.hDC, DialogSkin::panel);
    FillRect(draw.hDC, &draw.rcItem, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
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

void paint_card_info(const DRAWITEMSTRUCT& draw, const Picker& picker) {
    const auto dc = draw.hDC;
    SetDCBrushColor(dc, DialogSkin::panel);
    FillRect(dc, &draw.rcItem, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    if (!picker.displayed) return;
    const auto& card = *picker.displayed;
    const auto font = reinterpret_cast<HFONT>(SendMessageW(draw.hwndItem, WM_GETFONT, 0, 0));
    const auto previous = font ? SelectObject(dc, font) : nullptr;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, DialogSkin::ink);
    const auto height = (draw.rcItem.bottom - draw.rcItem.top) / 3;
    RECT row{draw.rcItem.left, draw.rcItem.top, draw.rcItem.right, draw.rcItem.top + height};
    const auto icon = [&](std::string_view key) {
        if (row.left + height <= row.right &&
            picker.skin->draw_icon(dc, key, {row.left, row.top, row.left + height, row.bottom}))
            row.left += height + 3;
    };
    const auto line = [&](const wchar_t* label) {
        DrawTextW(dc, label, -1, &row, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        row.left = draw.rcItem.left;
        OffsetRect(&row, 0, height);
    };
    icon(expansion_icon(card));
    line(card.expansion);
    std::string_view icons(card.type_icons);
    while (!icons.empty()) {
        const auto comma = icons.find(',');
        icon(icons.substr(0, comma));
        if (comma == icons.npos) break;
        icons.remove_prefix(comma + 1);
    }
    line(card.types);
    if (*card.cost) {
        if (card.coins > 0 || (!card.potions && !card.debt)) icon("coin");
        if (card.potions) icon("potion");
        if (card.debt) icon("debt");
        line(card.cost);
    }
    if (previous) SelectObject(dc, previous);
}

INT_PTR CALLBACK picker_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    try {
        if (message == WM_INITDIALOG) {
            auto& picker = *reinterpret_cast<Picker*>(lparam);
            SetWindowLongPtrW(dialog, DWLP_USER, lparam);
            const auto title = std::wstring(menu_text::choose_card) + L" · " + picker.title;
            SetWindowTextW(dialog, title.c_str());
            picker.skin = std::make_unique<DialogSkin>(dialog);
            SetDlgItemTextW(dialog, IDC_CARD_EXPANSIONS, menu_text::expansions);
            SetDlgItemTextW(dialog, IDOK, menu_text::done);
            SetDlgItemTextW(dialog, IDCANCEL, menu_text::cancel);
            SendDlgItemMessageW(dialog, IDC_CARD_QUERY, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"Kartenname suchen"));
            const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
            TreeView_SetBkColor(tree, DialogSkin::paper);
            TreeView_SetTextColor(tree, DialogSkin::ink);
            TreeView_SetLineColor(tree, RGB(147, 114, 72));
            const auto icon_size = picker.skin->units({0, 0, 10, 9}).right;
            picker.expansion_images = ImageList_Create(icon_size, icon_size, ILC_COLOR32, 16, 4);
            if (picker.expansion_images) TreeView_SetImageList(tree, picker.expansion_images, TVSIL_NORMAL);
            if (picker.multiple()) {
                SetWindowLongPtrW(tree, GWL_STYLE, GetWindowLongPtrW(tree, GWL_STYLE) | TVS_CHECKBOXES);
                if (const auto checks = picker.skin->checks()) {
                    const auto native_checks = TreeView_SetImageList(tree, checks, TVSIL_STATE);
                    if (native_checks) ImageList_Destroy(native_checks);
                }
            }
            SetWindowSubclass(tree, tree_proc, 1, 0);
            for (const auto* scope : {menu_text::kingdom, L"Alle Karten"})
                SendDlgItemMessageW(dialog, IDC_CARD_SCOPE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(scope));
            const bool has_current = picker.mode == CardSelection::multiple && !picker.current.empty();
            SendDlgItemMessageW(dialog, IDC_CARD_SCOPE, CB_SETCURSEL, has_current ? 0 : 1, 0);
            if (picker.mode == CardSelection::multiple && picker.current.size() < 10)
                SetDlgItemTextW(dialog, IDC_CARD_HINT, has_current
                    ? L"Offene Plätze lassen sich im Effektdialog automatisch verdoppeln."
                    : L"Zufällig gezogene Stapel lassen sich im Effektdialog automatisch verdoppeln.");
            Gdiplus::GdiplusStartupInput input;
            if (Gdiplus::GdiplusStartup(&picker.graphics, &input, nullptr) != Gdiplus::Ok) picker.graphics = 0;
            SetTimer(dialog, 7634, 50, nullptr);
            render_cards(dialog, picker);
            return TRUE;
        }
        auto* state = reinterpret_cast<Picker*>(GetWindowLongPtrW(dialog, DWLP_USER));
        if (!state) return FALSE;
        auto& picker = *state;
        if (message == WM_TIMER && wparam == 7634) {
            std::string id; ResourceImage image;
            if (picker.loader.take(id, image) && picker.displayed && id == picker.displayed->id) {
                picker.image = std::move(image);
                layout_preview(dialog, picker);
                InvalidateRect(dialog, nullptr, FALSE);
            }
            return TRUE;
        }
        if (message == WM_DRAWITEM && (wparam == IDC_CARD_IMAGE || wparam == IDC_CARD_INFO)) {
            const auto& draw = *reinterpret_cast<const DRAWITEMSTRUCT*>(lparam);
            if (wparam == IDC_CARD_IMAGE) paint_preview(draw, picker);
            else paint_card_info(draw, picker);
            SetWindowLongPtrW(dialog, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }
        if (picker.skin) {
            const auto handled = picker.skin->handle(message, wparam, lparam);
            if (handled) return handled;
        }
        if (message == WM_CLOSE) {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
        if (message == WM_DESTROY) {
            KillTimer(dialog, 7634);
            const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
            RemoveWindowSubclass(tree, tree_proc, 1);
            if (picker.multiple()) {
                const auto images = TreeView_SetImageList(tree, nullptr, TVSIL_STATE);
                if (images) ImageList_Destroy(images);
            }
            if (picker.expansion_images) {
                TreeView_SetImageList(tree, nullptr, TVSIL_NORMAL);
                ImageList_Destroy(picker.expansion_images);
                picker.expansion_images = nullptr;
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
                    preview(dialog, picker, card ? card : first_open_card(change.hdr.hwndFrom, picker));
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
                    if (!card_at(change.hdr.hwndFrom, TreeView_GetSelection(change.hdr.hwndFrom)))
                        preview(dialog, picker, first_open_card(change.hdr.hwndFrom, picker));
                }
            }
            return FALSE;
        }
        if (message != WM_COMMAND) return FALSE;
        const auto control = LOWORD(wparam);
        const auto event = HIWORD(wparam);
        if (control == IDC_CARD_TEXT) {
            picker.show_rules = !picker.show_rules;
            layout_preview(dialog, picker);
        } else if ((control == IDC_CARD_QUERY && event == EN_CHANGE) ||
            (control == IDC_CARD_SCOPE && event == CBN_SELCHANGE)) {
            SetTimer(dialog, 7634, 50, nullptr);
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
    Picker picker{title, cards, current_kingdom, mode, accept};
    if (DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_CARD_PICKER), owner, picker_proc,
                        reinterpret_cast<LPARAM>(&picker)) == -1) {
        MessageBoxW(owner, L"Die Kartenauswahl konnte nicht geöffnet werden.", title, MB_OK | MB_ICONWARNING);
    }
}

}
