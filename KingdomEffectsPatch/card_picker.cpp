#include "ui_language.hpp"
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
#include "list_reorder.hpp"
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
struct ExpansionInfo {const char* id;const wchar_t *de,*en,*description_de,*description_en,*rules_de,*rules_en;};
#include "expansion_catalog.generated.hpp"
const ExpansionInfo* expansion_info(std::string_view id){for(const auto& info:expansion_catalog)if(info.id==id)return &info;return nullptr;}


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
    const ExpansionInfo* expansion=nullptr;
    std::map<HTREEITEM,const ExpansionInfo*> groups{};
    ULONG_PTR graphics = 0;
    ResourceImage image{};
    CardPreviewLoader loader;
    bool rendering = false;
    std::unique_ptr<DialogSkin> skin{};
    HIMAGELIST expansion_images = nullptr;
    HIMAGELIST checks_images = nullptr;
    HWND checks_tree = nullptr;
    std::map<std::string, int, std::less<>> expansion_icons{};

    bool multiple() const { return mode != CardSelection::single; }
    bool has(const CardTarget* card) const {
        return std::find(selected.begin(), selected.end(), card) != selected.end();
    }

    void release_checks() noexcept {
        if (checks_images) {
            if (checks_tree && IsWindow(checks_tree) &&
                TreeView_GetImageList(checks_tree, TVSIL_STATE) == checks_images)
                TreeView_SetImageList(checks_tree, nullptr, TVSIL_STATE);
            ImageList_Destroy(checks_images);
            checks_images = nullptr;
        }
        checks_tree = nullptr;
    }

    ~Picker() {
        release_checks();
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

void layout_preview(HWND, const Picker& picker) {
    // Geometry is fixed before the first request, independent of loading state.
    picker.skin->place(IDC_CARD_RULES, {217, 84, 378, 126});
    const auto extra=picker.skin->logical_height()-247;
    picker.skin->place(IDC_CARD_IMAGE, {217, 129, 378, 222+extra});
    picker.skin->place(IDC_CARD_STATUS,{8,226+extra,264,243+extra});
    picker.skin->place(IDOK,{271,227+extra,323,243+extra});
    picker.skin->place(IDCANCEL,{326,227+extra,378,243+extra});
}

void preview(HWND dialog, Picker& picker, const CardTarget* card) {
    if (card && picker.displayed == card) return;
    const bool was_expansion=picker.expansion!=nullptr;
    picker.displayed = card;picker.expansion=nullptr;picker.image.reset();
    if(was_expansion){layout_preview(dialog,picker);ShowWindow(GetDlgItem(dialog,IDC_CARD_IMAGE),SW_SHOW);}
    SetDlgItemTextW(dialog, IDC_CARD_TITLE, card ? card_title(*card) : L"");
    SetDlgItemTextW(dialog, IDC_CARD_IMAGE, card ? card_title(*card) : L"");
    const auto info = card ? std::wstring(card_expansion(*card)) + L"\r\n" + card_types(*card) + L"\r\n" + card_cost(*card) : L"";
    SetDlgItemTextW(dialog, IDC_CARD_INFO, info.c_str());
    SetDlgItemTextW(dialog, IDC_CARD_RULES, card ? card_rules(*card) : picker.visible.empty()
        ? kingdom::ui::tr(L"Keine passenden Karten.") : kingdom::ui::tr(L"Öffne eine Erweiterung. Zeige auf einen Kartennamen oder wähle ihn mit den Pfeiltasten."));
    picker.image.reset();
    picker.loader.request(card ? card->id : "");
    InvalidateRect(GetDlgItem(dialog, IDC_CARD_INFO), nullptr, FALSE);
    InvalidateRect(GetDlgItem(dialog, IDC_CARD_IMAGE), nullptr, FALSE);
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
    for (const auto id : {IDC_CARD_SELECTION, IDC_CARD_ADD, IDC_CARD_REMOVE})
        ShowWindow(GetDlgItem(dialog, id), ordered ? SW_SHOW : SW_HIDE);
    const auto extra=picker.skin->logical_height()-247;
    const int bottom = (ordered ? 148 : bulk ? (hint ? 184 : 203) : 222)+extra;
    if(ordered) {
        picker.skin->place(IDC_CARD_ADD,{8,152+extra,210,168+extra});
        picker.skin->place(IDC_CARD_SELECTION,{8,171+extra,157,222+extra});
        picker.skin->place(IDC_CARD_REMOVE,{161,171+extra,210,187+extra});
    }
    picker.skin->place(IDC_CARD_HINT,{8,208+extra,210,223+extra});
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
        for (std::size_t i=0;i<picker.selected.size();++i) {
            const auto label=std::to_wstring(i+1)+L". "+card_title(*picker.selected[i]);
            SendDlgItemMessageW(dialog, IDC_CARD_SELECTION, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
        const auto* candidate=card_at(GetDlgItem(dialog,IDC_CARD_TREE),TreeView_GetSelection(GetDlgItem(dialog,IDC_CARD_TREE)));
        EnableWindow(GetDlgItem(dialog,IDC_CARD_ADD),candidate && !picker.has(candidate) && picker.selected.size()<replacement_limit);
    }
    const auto status = std::to_wstring(picker.visible.size()) + kingdom::ui::tr(L" Treffer · ") +
        std::to_wstring(picker.selected.size()) +
        (picker.mode == CardSelection::ordered ? kingdom::ui::tr(L" / 20 gewählt") : kingdom::ui::tr(L" gewählt"));
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
        SetDlgItemTextW(dialog, IDC_CARD_STATUS, kingdom::ui::tr(L"Taifun erlaubt bis zu 20 Ersatzstapel."));
        return;
    }
    if (adding) picker.selected.push_back(card);
    else picker.selected.erase(position);
    if(picker.mode==CardSelection::multiple) TreeView_SetCheckState(tree, item, adding);
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
    // State images are supplied by the picker, including state 0 for groups.
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
    picker.rendering = true;
    SendMessageW(tree, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(tree);
    picker.visible.clear();
    picker.groups.clear();
    if (!picker.multiple()) picker.selected.clear();
    HTREEITEM group = nullptr, focus = nullptr;
    std::wstring_view expansion;
    const auto expand_group = [&] {
        if (group && (!query.empty() || picker.expanded.find(expansion) != picker.expanded.end()))
            TreeView_Expand(tree, group, TVE_EXPAND);
    };
    auto sorted=only_current?picker.current:picker.cards;
    std::stable_sort(sorted.begin(),sorted.end(),[](const auto* a,const auto* b){
        const auto ae=fold_card_search(card_expansion(*a)),be=fold_card_search(card_expansion(*b));
        return ae==be?fold_card_search(card_title(*a))<fold_card_search(card_title(*b)):ae<be;});
    for (const auto* card : sorted) {
        if (std::wstring_view(card->search).find(query) == std::wstring_view::npos) continue;
        if (!group || expansion != card_expansion(*card)) {
            expand_group();
            expansion = card_expansion(*card);
            const auto key = expansion_icon(*card);
            auto [icon, added] = picker.expansion_icons.try_emplace(key, I_IMAGENONE);
            if (added) icon->second = picker.skin->add_icon(picker.expansion_images, key);
            group = insert_row(tree, TVI_ROOT, card_expansion(*card), nullptr, false, false, icon->second);
            picker.groups[group]=expansion_info(card->expansion_id);
        }
        const auto item = insert_row(tree, group, card_title(*card), card, picker.mode==CardSelection::multiple, picker.has(card));
        if (item) picker.visible.emplace_back(item, card);
        if (card == focused) focus = item;
    }
    expand_group();
    if (focus && !(TreeView_GetItemState(tree,TreeView_GetParent(tree,focus),TVIS_EXPANDED)&TVIS_EXPANDED)) focus=nullptr;
    if (focus) TreeView_SelectItem(tree, focus);
    picker.rendering = false;
    SendMessageW(tree, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(tree, nullptr, TRUE);
    layout_list(dialog, picker, only_current);
    EnableWindow(GetDlgItem(dialog, IDC_CARD_ALL), !picker.visible.empty());
    preview(dialog, picker, focus ? card_at(tree, focus) : nullptr);
    selection_status(dialog, picker);
}

void preview_expansion(HWND dialog,Picker& picker,const ExpansionInfo* info) {
    if(!info || picker.expansion==info)return;
    picker.expansion=info;picker.displayed=nullptr;picker.image.reset();picker.loader.request("");
    SetDlgItemTextW(dialog,IDC_CARD_TITLE,ui::pick(info->de,info->en));
    SetDlgItemTextW(dialog,IDC_CARD_INFO,L"");
    const auto* description=ui::pick(info->description_de,info->description_en);
    const auto* rules=ui::pick(info->rules_de,info->rules_en);
    std::wstring text=description?description:L"";
    if(rules && *rules) {
        if(!text.empty())text+=L"\r\n\r\n";
        text+=rules;
    }
    SetDlgItemTextW(dialog,IDC_CARD_RULES,text.empty()?ui::tr(L"Für diese Erweiterung sind keine zusätzlichen Regeln im Katalog enthalten."):text.c_str());
    const auto extra=picker.skin->logical_height()-247;
    picker.skin->place(IDC_CARD_RULES,{217,84,378,222+extra});
    ShowWindow(GetDlgItem(dialog,IDC_CARD_IMAGE),SW_HIDE);
    InvalidateRect(GetDlgItem(dialog,IDC_CARD_INFO),nullptr,FALSE);
}
void add_selected_pile(HWND dialog,Picker& picker) {
    const auto tree=GetDlgItem(dialog,IDC_CARD_TREE);const auto item=TreeView_GetSelection(tree);
    const auto* card=card_at(tree,item);
    if(card && !picker.has(card)) toggle_card(dialog,picker,item);
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
                RECT label{};
                if ((hit.flags&TVHT_ONITEMLABEL) && TreeView_GetItemRect(tree,item,&label,TRUE) && PtInRect(&label,hit.pt)) {
                    if(card)preview(dialog,picker,card);
                    else if(picker.groups.count(item))preview_expansion(dialog,picker,picker.groups[item]);
                }
            } else if(message==WM_LBUTTONDBLCLK && card && picker.mode==CardSelection::ordered && (hit.flags&TVHT_ONITEMLABEL)) {
                TreeView_SelectItem(tree,item);add_selected_pile(dialog,picker);return 0;
            } else if (picker.mode==CardSelection::multiple && card && (hit.flags & TVHT_ONITEMSTATEICON)) {
                if (message == WM_LBUTTONDOWN) {
                    SetFocus(tree);
                    TreeView_SelectItem(tree, item);
                    toggle_card(dialog, picker, item);
                }
                return 0;
            }
        } else if (message == WM_KEYDOWN && wparam == VK_SPACE && picker.multiple()) {
            if (!(lparam & (1 << 30))) {if(picker.mode==CardSelection::ordered)add_selected_pile(dialog,picker);else toggle_card(dialog, picker, TreeView_GetSelection(tree));}
            return 0;
        } else if (message == WM_CHAR && wparam == L' ' && picker.multiple()) {
            // WM_KEYDOWN already handled Space. Letting the native tree search
            // for a name starting with a space can trigger the Windows beep.
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
            if (visible_card == card && picker.mode == CardSelection::multiple)
                TreeView_SetCheckState(GetDlgItem(dialog, IDC_CARD_TREE), item, FALSE);
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
    const auto& rect = draw.rcItem;
    const int area_width = rect.right - rect.left, area_height = rect.bottom - rect.top;
    if (area_width <= 0 || area_height <= 0) return;
    const auto buffer = CreateCompatibleDC(draw.hDC);
    const auto bitmap = CreateCompatibleBitmap(draw.hDC, area_width, area_height);
    if (!buffer || !bitmap) { if (buffer) DeleteDC(buffer); if (bitmap) DeleteObject(bitmap); return; }
    const auto previous = SelectObject(buffer, bitmap);
    RECT local{0,0,area_width,area_height};
    SetDCBrushColor(buffer, DialogSkin::panel);
    FillRect(buffer, &local, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    if (picker.image) {
    const auto scale = (std::min)(static_cast<double>(rect.right - rect.left) / picker.image->GetWidth(),
                                  static_cast<double>(rect.bottom - rect.top) / picker.image->GetHeight());
    const auto width = static_cast<INT>(picker.image->GetWidth() * scale);
    const auto height = static_cast<INT>(picker.image->GetHeight() * scale);
    Gdiplus::Graphics graphics(buffer);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.DrawImage(picker.image.get(), (area_width - width) / 2,
                       (area_height - height) / 2, width, height);
    }
    BitBlt(draw.hDC, rect.left, rect.top, area_width, area_height, buffer, 0, 0, SRCCOPY);
    SelectObject(buffer, previous); DeleteObject(bitmap); DeleteDC(buffer);
}

void paint_card_info(const DRAWITEMSTRUCT& draw, const Picker& picker) {
    const auto dc = draw.hDC;
    SetDCBrushColor(dc, DialogSkin::panel);
    FillRect(dc, &draw.rcItem, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    if(picker.expansion){const auto side=draw.rcItem.bottom-draw.rcItem.top;const std::string key=std::string("exp-")+(std::string_view(picker.expansion->id)=="basic"?"base":picker.expansion->id);
        picker.skin->draw_icon(dc,key,{draw.rcItem.left,draw.rcItem.top,draw.rcItem.left+side,draw.rcItem.bottom});return;}
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
    line(card_expansion(card));
    std::string_view icons(card.type_icons);
    while (!icons.empty()) {
        const auto comma = icons.find(',');
        icon(icons.substr(0, comma));
        if (comma == icons.npos) break;
        icons.remove_prefix(comma + 1);
    }
    line(card_types(card));
    if (*card.cost) {
        if (card.coins > 0 || (!card.potions && !card.debt)) icon("coin");
        if (card.potions) icon("potion");
        if (card.debt) icon("debt");
        line(card_cost(card));
    }
    if (previous) SelectObject(dc, previous);
}

INT_PTR CALLBACK picker_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    try {
        if (message == WM_INITDIALOG) {
            auto& picker = *reinterpret_cast<Picker*>(lparam);
            SetWindowLongPtrW(dialog, DWLP_USER, lparam);
            const auto title = std::wstring(menu_text::choose_card()) + L" · " + picker.title;
            SetWindowTextW(dialog, title.c_str());
            picker.skin = std::make_unique<DialogSkin>(dialog,true);
            SetDlgItemTextW(dialog, IDC_CARD_EXPANSIONS, menu_text::expansions());
            SetDlgItemTextW(dialog, IDOK, menu_text::done());
            SetDlgItemTextW(dialog, IDCANCEL, menu_text::cancel());
            SendDlgItemMessageW(dialog, IDC_CARD_QUERY, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(kingdom::ui::tr(L"Kartenname suchen")));
            const auto tree = GetDlgItem(dialog, IDC_CARD_TREE);
            TreeView_SetBkColor(tree, DialogSkin::paper);
            TreeView_SetTextColor(tree, DialogSkin::ink);
            TreeView_SetLineColor(tree, RGB(147, 114, 72));
            const auto icon_size = picker.skin->units({0, 0, 10, 9}).right;
            picker.expansion_images = ImageList_Create(icon_size, icon_size, ILC_COLOR32, 16, 4);
            if (picker.expansion_images) TreeView_SetImageList(tree, picker.expansion_images, TVSIL_NORMAL);
            if (picker.mode==CardSelection::multiple) {
                // Do not enable TVS_CHECKBOXES: native automatic state images
                // would compete with our explicit mouse/keyboard state handling.
                picker.checks_images = picker.skin->checks();
                if (!picker.checks_images) {
                    EndDialog(dialog, -1);
                    return TRUE;
                }
                picker.checks_tree = tree;
                TreeView_SetImageList(tree, picker.checks_images, TVSIL_STATE);
                if (TreeView_GetImageList(tree, TVSIL_STATE) != picker.checks_images) {
                    EndDialog(dialog, -1);
                    return TRUE;
                }
            }
            SetWindowSubclass(tree, tree_proc, 1, 0);
            ListReorder::attach(GetDlgItem(dialog,IDC_CARD_SELECTION),[dialog,&picker](int from,int to){move_entry(picker.selected,from,to);selection_status(dialog,picker);SendDlgItemMessageW(dialog,IDC_CARD_SELECTION,LB_SETCURSEL,to,0);},
                [dialog]{SendMessageW(dialog,WM_COMMAND,IDC_CARD_REMOVE,0);});
            for (const auto* scope : {menu_text::kingdom(), kingdom::ui::tr(L"Alle Karten")})
                SendDlgItemMessageW(dialog, IDC_CARD_SCOPE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(scope));
            const bool has_current = picker.mode == CardSelection::multiple && !picker.current.empty();
            SendDlgItemMessageW(dialog, IDC_CARD_SCOPE, CB_SETCURSEL, has_current ? 0 : 1, 0);
            if (picker.mode == CardSelection::multiple && picker.current.size() < 10)
                SetDlgItemTextW(dialog, IDC_CARD_HINT, has_current
                    ? kingdom::ui::tr(L"Offene Plätze lassen sich im Effektdialog automatisch verdoppeln.")
                    : kingdom::ui::tr(L"Zufällig gezogene Stapel lassen sich im Effektdialog automatisch verdoppeln."));
            Gdiplus::GdiplusStartupInput input;
            if (Gdiplus::GdiplusStartup(&picker.graphics, &input, nullptr) != Gdiplus::Ok) picker.graphics = 0;
            SetTimer(dialog, 7634, 50, nullptr);
            layout_preview(dialog, picker);
            render_cards(dialog, picker);
            return TRUE;
        }
        auto* state = reinterpret_cast<Picker*>(GetWindowLongPtrW(dialog, DWLP_USER));
        if (!state) return FALSE;
        auto& picker = *state;
        if (message == WM_TIMER && wparam == 7634) {
            std::string id; ResourceImage image;ui::Language language;
            if (picker.loader.take(id, image,&language) && language==ui::language() && picker.displayed && id == picker.displayed->id) {
                picker.image = std::move(image);
                InvalidateRect(GetDlgItem(dialog, IDC_CARD_IMAGE), nullptr, FALSE);
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
            picker.release_checks();
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
                    if(card) preview(dialog, picker, card);
                    else if(picker.groups.count(change.itemNew.hItem))preview_expansion(dialog,picker,picker.groups[change.itemNew.hItem]);
                    selection_status(dialog, picker);
                }
            } else if (change.hdr.code == TVN_ITEMEXPANDEDW && !picker.rendering && !change.itemNew.lParam) {
                std::array<wchar_t, 128> name{};
                TVITEMW item{};
                item.hItem = change.itemNew.hItem;
                item.mask = TVIF_TEXT;
                item.pszText = name.data();
                item.cchTextMax = static_cast<int>(name.size());
                if (TreeView_GetItem(change.hdr.hwndFrom, &item)) {
                    if(GetWindowTextLengthW(GetDlgItem(dialog,IDC_CARD_QUERY))==0) {
                        if (change.action == TVE_EXPAND) picker.expanded.insert(name.data());
                        else picker.expanded.erase(name.data());
                    }
                }
            }
            return FALSE;
        }
        if (message != WM_COMMAND) return FALSE;
        const auto control = LOWORD(wparam);
        const auto event = HIWORD(wparam);
        if ((control == IDC_CARD_QUERY && event == EN_CHANGE) ||
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
        } else if(control==IDC_CARD_ADD && picker.mode==CardSelection::ordered) {
            add_selected_pile(dialog,picker);
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
        MessageBoxW(owner, kingdom::ui::tr(L"Die Kartenauswahl konnte nicht geöffnet werden."), title, MB_OK | MB_ICONWARNING);
    }
}

}
