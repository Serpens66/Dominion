#include "ui_language.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "effect_editor.hpp"
#include "target_access_runtime.hpp"
#include "effect_editor_ids.h"
#include "card_picker.hpp"
#include "dialog_skin.hpp"
#include "list_reorder.hpp"
#include "menu_labels.generated.hpp"
#include "effect_table.hpp"

namespace kingdom {
namespace {

enum class TargetKind {
    none, card, pile, kingdom_pile, event, landmark, way, prophecy, project, pile_list, duration, heirloom_parent
};

constexpr unsigned kind_mask(TargetKind kind) {
    return 1u << static_cast<unsigned>(kind);
}

struct EffectDefinition {
    const char* id;
    const wchar_t* title;
    const wchar_t* description;
    const wchar_t* timing;
    const wchar_t* status;
    const wchar_t* verification;
    const wchar_t* expansion_reference;
    unsigned timing_mask;
    TargetKind first;
    TargetKind second;
    const wchar_t *title_en,*description_en,*timing_en,*status_en,*expansion_reference_en;
    const wchar_t *requirement_de,*requirement_en,*requirement_detail_de,*requirement_detail_en;
};
const wchar_t* effect_title(const EffectDefinition& effect) {return ui::pick(effect.title,effect.title_en);}
const wchar_t* effect_description(const EffectDefinition& effect) {return ui::pick(effect.description,effect.description_en);}
const wchar_t* effect_timing(const EffectDefinition& effect) {return ui::pick(effect.timing,effect.timing_en);}
const wchar_t* effect_status(const EffectDefinition& effect) {return ui::pick(effect.status,effect.status_en);}
const wchar_t* effect_reference(const EffectDefinition& effect) {return ui::pick(effect.expansion_reference,effect.expansion_reference_en);}
const wchar_t* effect_requirement(const EffectDefinition& effect) {return ui::pick(effect.requirement_de,effect.requirement_en);}
const wchar_t* effect_requirement_detail(const EffectDefinition& effect) {return ui::pick(effect.requirement_detail_de,effect.requirement_detail_en);}

#include "effect_catalog.generated.hpp"

struct EffectPresentation {
    std::array<std::wstring,3> cells;
    std::vector<std::wstring> search;
    int height=0;
};

struct Draft {
    EffectCommands effects;
    bool automatic;
    const std::function<bool(const EffectCommands&)>& apply;
    CardTargets fixed;
    EditorContext context;
    const std::wstring* apply_error = nullptr;
    const EffectDefinition* selected = nullptr;
    const CardTarget* first = nullptr;
    const CardTarget* second = nullptr;
    std::unique_ptr<DialogSkin> skin{};
    std::vector<EffectPresentation> catalog;
    bool catalog_english=false;
    HFONT height_font=nullptr;
    std::array<int,3> height_widths{};
    POINT height_padding{};
    std::unique_ptr<EffectCellMeasurer> measurer;
};

std::wstring widen(std::string_view value) {
    return {value.begin(), value.end()}; // All native IDs and effect arguments are ASCII.
}

std::wstring input(HWND dialog, int control) {
    std::array<wchar_t, 256> buffer{};
    GetDlgItemTextW(dialog, control, buffer.data(), static_cast<int>(buffer.size()));
    const std::wstring value(buffer.data());
    const auto first = value.find_first_not_of(L" \t");
    if (first == value.npos) return {};
    return value.substr(first, value.find_last_not_of(L" \t") - first + 1);
}

const wchar_t* target_label(TargetKind kind) {
    switch (kind) {
        case TargetKind::card: return kingdom::ui::tr(L"Karte");
        case TargetKind::pile: return kingdom::ui::tr(L"Stapel");
        case TargetKind::kingdom_pile: return kingdom::ui::tr(L"Königreichstapel");
        case TargetKind::pile_list: return kingdom::ui::tr(L"Ersatzstapel (bis zu 20)");
        case TargetKind::event: return kingdom::ui::tr(L"Ereignis");
        case TargetKind::landmark: return kingdom::ui::tr(L"Landmarke");
        case TargetKind::way: return kingdom::ui::tr(L"Weg");
        case TargetKind::prophecy: return kingdom::ui::tr(L"Prophezeiung");
        case TargetKind::project: return kingdom::ui::tr(L"Projekt");
        case TargetKind::duration: return kingdom::ui::tr(L"Dauerkarte");
        case TargetKind::heirloom_parent: return kingdom::ui::tr(L"Königreichkarte mit Erbstück");
        default: return L"";
    }
}

std::string_view canonical_card(std::string_view id) {
    for (const auto& [alias, canonical] : target_aliases) {
        if (id == alias) return canonical;
    }
    return id;
}

CardTargets fixed_targets(std::string_view text) {
    const auto ids = fixed_kingdom_ids(text);
    CardTargets cards;
    for (const auto& target : target_catalog) {
        if (!(target.kinds & kind_mask(TargetKind::kingdom_pile))) continue;
        if (std::any_of(ids.begin(), ids.end(), [&](const auto& id) { return canonical_card(id) == target.id; }))
            cards.push_back(&target);
    }
    return cards;
}

bool multiplier(const EffectDefinition& effect) {
    const std::string_view id(effect.id);
    return id == "start_double_pile" || id == "start_triple_pile";
}

std::wstring active_label(std::string_view command) {
    const auto opening = command.find('(');
    const auto id = command.substr(0, opening);
    const auto* effect = std::find_if(std::begin(effect_catalog), std::end(effect_catalog),
        [id](const auto& entry) { return entry.id == id; });
    if (effect == std::end(effect_catalog)) return widen(command);
    std::wstring label(effect_title(*effect));
    if (opening == command.npos) return label;
    auto arguments = command.substr(opening + 1, command.size() - opening - 2);
    label += L" · ";
    while (!arguments.empty()) {
        const auto comma = arguments.find(',');
        const auto argument = canonical_card(trimmed(arguments.substr(0, comma)));
        const auto* target = std::find_if(std::begin(target_catalog), std::end(target_catalog),
            [argument](const auto& entry) { return entry.id == argument; });
        label += target == std::end(target_catalog) ? widen(argument) : std::wstring(card_title(*target));
        if (comma == arguments.npos) break;
        label += L", ";
        arguments.remove_prefix(comma + 1);
    }
    return label;
}

void set_list_width(HWND list, LRESULT count) {
    const auto dc = GetDC(list);
    if (!dc) return;
    const auto font = reinterpret_cast<HFONT>(SendMessageW(list, WM_GETFONT, 0, 0));
    const auto previous = font ? SelectObject(dc, font) : nullptr;
    LONG width = 0;
    std::wstring text;
    for (LRESULT row = 0; row < count; ++row) {
        const auto length = SendMessageW(list, LB_GETTEXTLEN, row, 0);
        if (length <= 0) continue;
        text.resize(static_cast<std::size_t>(length) + 1);
        SendMessageW(list, LB_GETTEXT, row, reinterpret_cast<LPARAM>(text.data()));
        SIZE extent{};
        if (GetTextExtentPoint32W(dc, text.data(), static_cast<int>(length), &extent))
            width = (std::max)(width, extent.cx + 8);
    }
    if (previous) SelectObject(dc, previous);
    ReleaseDC(list, dc);
    SendMessageW(list, LB_SETHORIZONTALEXTENT, width, 0);
}

std::array<RECT, 3> table_cells(HWND dialog, RECT row) {
    RECT client{};
    GetClientRect(GetDlgItem(dialog, IDC_CATALOG), &client);
    const LONG first = row.left + client.right * 30 / 100;
    const LONG second = row.left + client.right * 73 / 100;
    return {RECT{row.left, row.top, first, row.bottom},
            RECT{first, row.top, second, row.bottom},
            RECT{second, row.top, row.left + client.right, row.bottom}};
}

std::wstring table_name(const EffectDefinition& effect) {
    std::wstring name = std::wstring(effect_title(effect)) + L" (" + widen(effect.id) + L")";
    if (effect.first != TargetKind::none) name += L" · " + std::wstring(target_label(effect.first));
    if (*effect.verification) name += L" · " + std::wstring(ui::tr(effect.verification));
    return name;
}

std::array<std::wstring, 3> table_text(const EffectDefinition& effect) {
    auto name = table_name(effect);
    // Allow the native text renderer to wrap long IDs at their underscores.
    for (std::size_t at = 0; (at = name.find(L'_', at)) != name.npos; at += 2)
        name.insert(at + 1, 1, L'\u200B');
    auto description=std::wstring(effect_description(effect))+L"\r\n"+effect_requirement(effect);
    return {std::move(name), std::move(description), effect_timing(effect)};
}

constexpr UINT table_text_flags = DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;

void prepare_catalog(Draft& draft) {
    if(draft.catalog.size()==std::size(effect_catalog) && draft.catalog_english==ui::english())return;
    draft.catalog.clear();draft.catalog.reserve(std::size(effect_catalog));draft.catalog_english=ui::english();
    for(const auto& effect:effect_catalog){
        EffectPresentation entry;entry.cells=table_text(effect);
        for(const auto* text:{effect.title,effect.description,effect.timing,effect.expansion_reference,effect.verification,
            effect.title_en,effect.description_en,effect.timing_en,effect.requirement_de,effect.requirement_en,
            effect.requirement_detail_de,effect.requirement_detail_en,ui::tr(effect.verification)})entry.search.push_back(fold_card_search(text));
        entry.search.push_back(fold_card_search(widen(effect.id)));
        draft.catalog.push_back(std::move(entry));
    }
}

void measure_rows(HWND dialog,Draft& draft) {
    const auto list=GetDlgItem(dialog,IDC_CATALOG);
    const auto font=reinterpret_cast<HFONT>(SendMessageW(list,WM_GETFONT,0,0));
    const auto padding=draft.skin->units({0,0,4,3});
    SendMessageW(list,effect_table_padding,padding.right,padding.bottom);
    if(!draft.measurer)draft.measurer=std::make_unique<EffectCellMeasurer>(list);
    RECT client{};GetClientRect(list,&client);const auto cells=table_cells(dialog,client);
    const std::array<int,3> widths{cells[0].right-cells[0].left-1,cells[1].right-cells[1].left-1,cells[2].right-cells[2].left-1};
    if(font!=draft.height_font || widths!=draft.height_widths || padding.right!=draft.height_padding.x || padding.bottom!=draft.height_padding.y){
        for(auto& entry:draft.catalog)entry.height=0;
        draft.height_font=font;draft.height_widths=widths;draft.height_padding={padding.right,padding.bottom};
    }
    const auto count=SendMessageW(list,LB_GETCOUNT,0,0);
    for(LRESULT row=0;row<count;++row){
        const auto* effect=reinterpret_cast<const EffectDefinition*>(SendMessageW(list,LB_GETITEMDATA,row,0));
        auto& entry=draft.catalog.at(std::size_t(effect-std::begin(effect_catalog)));
        if(!entry.height){
            for(std::size_t column=0;column<3;++column)
                entry.height=(std::max)(entry.height,draft.measurer->measure(font,entry.cells[column],widths[column],padding.right,padding.bottom)+1);
        }
        SendMessageW(list,LB_SETITEMHEIGHT,row,entry.height);
    }
    InvalidateRect(GetDlgItem(dialog,IDC_CATALOG_HEADER),nullptr,FALSE);
}

void paint_table(HWND dialog, const Draft& draft, const DRAWITEMSTRUCT& draw) {
    const bool header = draw.CtlID == IDC_CATALOG_HEADER;
    if (!header && draw.itemID == static_cast<UINT>(-1)) return;
    const bool selected = !header && (draw.itemState & ODS_SELECTED);
    const auto background = header ? RGB(213, 185, 131) : selected ? RGB(231, 205, 148) : DialogSkin::paper;
    SetDCBrushColor(draw.hDC, background);
    FillRect(draw.hDC, &draw.rcItem, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    const auto font_window = GetDlgItem(dialog, header ? IDC_EFFECT_LABEL : IDC_CATALOG);
    const auto previous = SelectObject(draw.hDC, reinterpret_cast<HFONT>(SendMessageW(font_window, WM_GETFONT, 0, 0)));
    SetTextColor(draw.hDC, DialogSkin::ink);
    SetBkMode(draw.hDC, TRANSPARENT);
    const auto text = header ? std::array<std::wstring, 3>{kingdom::ui::tr(L"Effekt"), kingdom::ui::tr(L"Wirkung"), kingdom::ui::tr(L"Wann/wie")} :
        table_text(*reinterpret_cast<const EffectDefinition*>(SendDlgItemMessageW(dialog, IDC_CATALOG,
                                                                                  LB_GETITEMDATA, draw.itemID, 0)));
    const auto cells = table_cells(dialog, draw.rcItem);
    const auto padding = draft.skin->units({0, 0, 4, 3});
    const auto pen = CreatePen(PS_SOLID, 1, RGB(198, 174, 126));
    const auto previous_pen = SelectObject(draw.hDC, pen);
    for (std::size_t column = 0; column < cells.size(); ++column) {
        auto bounds = cells[column];
        const int saved = SaveDC(draw.hDC);
        IntersectClipRect(draw.hDC, bounds.left, bounds.top, bounds.right, bounds.bottom);
        InflateRect(&bounds, -padding.right, -padding.bottom);
        if(header) DrawTextW(draw.hDC, text[column].c_str(), -1, &bounds, table_text_flags);
        RestoreDC(draw.hDC, saved);
        if (column) {
            MoveToEx(draw.hDC, cells[column].left, draw.rcItem.top, nullptr);
            LineTo(draw.hDC, cells[column].left, draw.rcItem.bottom);
        }
    }
    MoveToEx(draw.hDC, draw.rcItem.left, draw.rcItem.bottom - 1, nullptr);
    LineTo(draw.hDC, draw.rcItem.right, draw.rcItem.bottom - 1);
    SelectObject(draw.hDC, previous_pen);
    DeleteObject(pen);
    if ((draw.itemState & ODS_FOCUS) && !(draw.itemState & ODS_NOFOCUSRECT)) {
        RECT focus = draw.rcItem;
        InflateRect(&focus, -2, -2);
        DrawFocusRect(draw.hDC, &focus);
    }
    SelectObject(draw.hDC, previous);
}

void update_active(HWND dialog, const Draft& draft, LRESULT selection = -1) {
    const auto list = GetDlgItem(dialog, IDC_ACTIVE);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (const auto& command : draft.effects) {
        const auto label = active_label(command);
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    set_list_width(list, SendMessageW(list, LB_GETCOUNT, 0, 0));
    if (selection >= 0 && !draft.effects.empty()) {
        SendDlgItemMessageW(dialog, IDC_ACTIVE, LB_SETCURSEL,
                            (std::min)(selection, static_cast<LRESULT>(draft.effects.size() - 1)), 0);
    }
    const auto status = std::to_wstring(draft.effects.size()) + kingdom::ui::tr(L" / 32 Effekte");
    SetDlgItemTextW(dialog, IDC_EFFECT_COUNT, status.c_str());
}

void update_targets(HWND dialog, const Draft& draft) {
    const auto* effect = draft.selected;
    const bool needs_first = effect && effect->first != TargetKind::none;
    const bool needs_second = effect && effect->second != TargetKind::none;
    const bool multiple = effect && (multiplier(*effect) || effect->first == TargetKind::pile_list);
    ShowWindow(GetDlgItem(dialog, IDC_TARGET), needs_first ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dialog, IDC_SECOND_TARGET), needs_second ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dialog, IDC_ADD), effect && (!needs_first || needs_second) ? SW_SHOW : SW_HIDE);
    EnableWindow(GetDlgItem(dialog, IDC_ADD), effect && (!needs_second || (draft.first && draft.second)));
    if (needs_first) {
        draft.skin->place(IDC_TARGET, {8, 298, 135, 314});
        SetDlgItemTextW(dialog, IDC_TARGET, draft.first ? card_title(*draft.first) :
                       multiple ? kingdom::ui::tr(L"Ziele wählen…") : menu_text::choose_card());
    }
    if (needs_second)
        SetDlgItemTextW(dialog, IDC_SECOND_TARGET, draft.second ? card_title(*draft.second) : kingdom::ui::tr(L"Zweites Ziel…"));
    if (effect && (!needs_first || needs_second))
        draft.skin->place(IDC_ADD, needs_second ? RECT{270, 298, 397, 314} : RECT{8, 298, 135, 314});
}

void select_effect(HWND dialog, Draft& draft) {
    const auto row = SendDlgItemMessageW(dialog, IDC_CATALOG, LB_GETCURSEL, 0, 0);
    const auto* selection = row < 0 ? nullptr : reinterpret_cast<const EffectDefinition*>(
        SendDlgItemMessageW(dialog, IDC_CATALOG, LB_GETITEMDATA, row, 0));
    if (selection != draft.selected) {
        draft.selected = selection;
        draft.first = draft.second = nullptr;
        SetDlgItemTextW(dialog, IDC_STATUS, L"");
    }
    const auto evidence=selection ? std::wstring(effect_status(*selection))+L"\r\n"+effect_requirement_detail(*selection)+ui::tr(L"\r\nErweiterungsbezug im Spielcode: ")+
        (*effect_reference(*selection)?effect_reference(*selection):ui::tr(L"keine Vorgabe")) : std::wstring{};
    SetDlgItemTextW(dialog, IDC_EFFECT_EVIDENCE,evidence.c_str());
    update_targets(dialog, draft);
}

void filter_catalog(HWND dialog, Draft& draft) {
    const auto filter = fold_card_search(input(dialog, IDC_QUERY));
    const auto category = SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_GETCURSEL, 0, 0);
    const std::array<std::string_view, 4> prefixes{"", "twist_", "stamp_", "start_"};
    const auto prefix = category >= 0 && category < 4 ? prefixes[category] : prefixes[0];
    const unsigned timing = category >= 4 && category < 4 + std::size(timing_filters) ? 1u << (category - 4) : 0;
    prepare_catalog(draft);
    std::vector<effect_table_detail::Row> rows;rows.reserve(std::size(effect_catalog));
    LRESULT selection=0;
    for(const auto& effect:effect_catalog){
        const std::string_view id(effect.id);
        if(id.substr(0,prefix.size())!=prefix || (timing && !(effect.timing_mask&timing)))continue;
        const auto& entry=draft.catalog[std::size_t(&effect-std::begin(effect_catalog))];
        if(!std::any_of(entry.search.begin(),entry.search.end(),[&](const auto& text){return text.find(filter)!=text.npos;}))continue;
        if(&effect==draft.selected)selection=LRESULT(rows.size());
        rows.push_back({table_name(effect),reinterpret_cast<LPARAM>(&effect),entry.height?entry.height:24,entry.cells});
    }
    SendDlgItemMessageW(dialog,IDC_CATALOG,WM_SETREDRAW,FALSE,0);
    SendDlgItemMessageW(dialog,IDC_CATALOG,effect_table_replace_rows,0,reinterpret_cast<LPARAM>(&rows));
    measure_rows(dialog, draft);
    SendDlgItemMessageW(dialog, IDC_CATALOG, WM_SETREDRAW, TRUE, 0);
    SendDlgItemMessageW(dialog, IDC_CATALOG, LB_SETCURSEL, selection, 0);
    select_effect(dialog, draft);
}

std::string effect_command(const EffectDefinition& effect, const CardTargets& targets) {
    std::string command(effect.id);
    if (targets.empty()) return command;
    char separator = '(';
    for (const auto* target : targets) {
        command += separator;
        command += target->id;
        separator = ',';
    }
    return command + ')';
}

std::wstring add_commands(HWND dialog, Draft& draft, const EffectCommands& commands, bool skip_existing) {
    if (const auto failure = check_target_access(commands)) return target_access_message(failure);
    const auto previous = draft.effects.size();
    const auto result = append_editor_effects(draft.effects, commands, skip_existing);
    if (const auto* known = known_failure_message(result.error)) return ui::tr(known);
    if (result.error == Error::capacity)
        return result.missing_slots == 1 ? kingdom::ui::tr(L"Für diese Auswahl fehlt ein Effektplatz.") :
               kingdom::ui::tr(L"Für diese Auswahl fehlen ") + std::to_wstring(result.missing_slots) + kingdom::ui::tr(L" Effektplätze.");
    if (result.error == Error::shared_target) return kingdom::ui::tr(L"Platz für Mehr und Taifun sind jeweils einmal möglich.");
    if (result.error != Error::none) return kingdom::ui::tr(L"Diese Effektliste kann das Spiel nicht übernehmen.");
    const auto added = draft.effects.size() - previous;
    const auto skipped = commands.size() - added;
    if (added) update_active(dialog, draft, static_cast<LRESULT>(draft.effects.size() - 1));
    const auto status = std::to_wstring(added) + kingdom::ui::tr(L" hinzugefügt") +
        (skipped ? L" · " + std::to_wstring(skipped) + kingdom::ui::tr(L" bereits vorhanden") : L"");
    SetDlgItemTextW(dialog, IDC_STATUS, status.c_str());
    return {};
}

void choose_targets(HWND dialog, Draft& draft, bool second) {
    if (!draft.selected) return;
    const auto& effect = *draft.selected;
    const auto kind = second ? effect.second : effect.first;
    if (kind == TargetKind::none) return;
    const auto mode = kind == TargetKind::pile_list ? CardSelection::ordered :
                      multiplier(effect) ? CardSelection::multiple : CardSelection::single;
    const auto mask = kind_mask(kind == TargetKind::pile_list ? TargetKind::kingdom_pile : kind);
    CardTargets cards, current;
    bool unknown_access=false;
    for (const auto& card : target_catalog) {
        if (!(card.kinds & mask)) continue;
        const auto access=target_card_access(card.id);
        if (access==CardAccess::allowed) cards.push_back(&card);
        else if (access==CardAccess::unknown) unknown_access=true;
    }
    if (mode == CardSelection::multiple) {
        for (const auto* card : draft.fixed)
            if ((card->kinds & mask) && target_card_access(card->id) == CardAccess::allowed) current.push_back(card);
    }
    if (cards.empty()) {
        const auto message=unknown_access ? target_access_message({CardAccess::unknown,effect.id,"",false}) :
            std::wstring(ui::pick(L"Für diese Zielart sind derzeit keine Karten für dein Konto zum Spielen freigegeben.", L"No cards of this target type are currently available to play on your account."));
        SetDlgItemTextW(dialog,IDC_STATUS,message.c_str());
        return;
    }
    const auto title = std::wstring(effect_title(effect)) + L" · " + target_label(kind);
    pick_cards(dialog, title.c_str(), cards, current, mode, [&](const CardTargets& targets, bool repeat) {
        if (effect.second != TargetKind::none) {
            (second ? draft.second : draft.first) = targets.front();
            update_targets(dialog, draft);
            return std::wstring{};
        }
        EffectCommands commands;
        if (mode == CardSelection::multiple) {
            for (const auto* card : targets) commands.push_back(effect_command(effect, {card}));
        } else commands.push_back(effect_command(effect, targets));
        return add_commands(dialog, draft, commands, mode == CardSelection::multiple && !repeat);
    });
}

void add_effect(HWND dialog, Draft& draft) {
    if (!draft.selected) return;
    CardTargets targets;
    if (draft.selected->second != TargetKind::none) {
        if (!draft.first || !draft.second) return;
        targets = {draft.first, draft.second};
    }
    const auto message = add_commands(dialog, draft, {effect_command(*draft.selected, targets)}, false);
    if (!message.empty()) SetDlgItemTextW(dialog, IDC_STATUS, message.c_str());
}

void refresh_language(HWND dialog,Draft& draft) {
    const auto active=SendDlgItemMessageW(dialog,IDC_ACTIVE,LB_GETCURSEL,0,0);
    const auto previous_category=SendDlgItemMessageW(dialog,IDC_CATEGORY,CB_GETCURSEL,0,0);
    draft.skin->language_changed();
    SetDlgItemTextW(dialog,IDC_STATUS,L"");
    SendDlgItemMessageW(dialog,IDC_CATEGORY,CB_RESETCONTENT,0,0);
    const bool online = draft.context == EditorContext::online;
    const auto title = std::wstring(menu_text::kingdom()) + (online ? kingdom::ui::tr(L" – Online – Wirkung serverabhängig") : L"");
    SetWindowTextW(dialog, title.c_str());
    SetDlgItemTextW(dialog, IDC_EFFECT_LABEL, menu_text::features());
    SetDlgItemTextW(dialog, IDOK, menu_text::done());
    SetDlgItemTextW(dialog, IDCANCEL, menu_text::cancel());
    SendDlgItemMessageW(dialog, IDC_QUERY, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(kingdom::ui::tr(L"Name oder Effekt-ID")));
    for (const auto* category : {kingdom::ui::tr(L"Alle Kategorien"), kingdom::ui::tr(L"Effektart: Spielvariationen"), kingdom::ui::tr(L"Effektart: Stempel"), kingdom::ui::tr(L"Effektart: Spielaufbau")}) {
        SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(category));
    }
    for (const auto* label : timing_filters) {
        const auto name = std::wstring(kingdom::ui::tr(L"Wann/wie: ")) + ui::tr(label);
        SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
    }
    const auto category = GetDlgItem(dialog, IDC_CATEGORY);
    const auto dc = GetDC(category);
    const auto old_font = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(category, WM_GETFONT, 0, 0)));
    LONG width = 0;
    for (LRESULT row = 0; row < SendMessageW(category, CB_GETCOUNT, 0, 0); ++row) {
        std::array<wchar_t, 256> label{}; SendMessageW(category, CB_GETLBTEXT, row, reinterpret_cast<LPARAM>(label.data()));
        SIZE extent{}; GetTextExtentPoint32W(dc, label.data(), lstrlenW(label.data()), &extent);
        width = (std::max)(width, extent.cx + 2 * GetSystemMetrics(SM_CXVSCROLL));
    }
    SelectObject(dc, old_font); ReleaseDC(category, dc);
    SendMessageW(category, CB_SETDROPPEDWIDTH, width, 0);
    SendMessageW(category, CB_SETMINVISIBLE, 14, 0);
    SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_SETCURSEL, (std::max)(LRESULT(0),previous_category), 0);
    if(online) SetDlgItemTextW(dialog,IDC_AUTOMATIC,ui::tr(L"Automatische Verdopplung nur lokal"));
    filter_catalog(dialog,draft);update_active(dialog,draft,active);update_targets(dialog,draft);
}

INT_PTR CALLBACK dialog_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    try {
        if (message == WM_INITDIALOG) {
            auto& draft = *reinterpret_cast<Draft*>(lparam);
            SetWindowLongPtrW(dialog, DWLP_USER, lparam);
            draft.skin = std::make_unique<DialogSkin>(dialog);
            for(const auto* name:{L"Deutsch",L"English"}) SendDlgItemMessageW(dialog,IDC_LANGUAGE,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
            SendDlgItemMessageW(dialog,IDC_LANGUAGE,CB_SETCURSEL,ui::english()?1:0,0);
            const bool online=draft.context==EditorContext::online;
            refresh_language(dialog,draft);
            ListReorder::attach(GetDlgItem(dialog,IDC_ACTIVE),[dialog,&draft](int from,int to){move_entry(draft.effects,from,to);update_active(dialog,draft,to);},
                [dialog]{SendMessageW(dialog,WM_COMMAND,IDC_REMOVE,0);});
            CheckDlgButton(dialog, IDC_AUTOMATIC, !online && draft.automatic ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(GetDlgItem(dialog, IDC_AUTOMATIC), !online);
            if (online) SetDlgItemTextW(dialog, IDC_AUTOMATIC, kingdom::ui::tr(L"Automatische Verdopplung nur lokal"));
            return TRUE;
        }
        const auto state = GetWindowLongPtrW(dialog, DWLP_USER);
        if (!state) return FALSE;
        auto& draft = *reinterpret_cast<Draft*>(state);
        if (message == effect_table_layout && draft.skin) {
            if(wparam){draft.height_font=nullptr;draft.measurer.reset();}
            prepare_catalog(draft);
            SendDlgItemMessageW(dialog, IDC_CATALOG, WM_SETREDRAW, FALSE, 0);
            measure_rows(dialog, draft);
            SendDlgItemMessageW(dialog, IDC_CATALOG, WM_SETREDRAW, TRUE, 0);
            return TRUE;
        }
        if (message == WM_DRAWITEM && (wparam == IDC_CATALOG || wparam == IDC_CATALOG_HEADER)) {
            paint_table(dialog, draft, *reinterpret_cast<const DRAWITEMSTRUCT*>(lparam));
            SetWindowLongPtrW(dialog, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }
        if (draft.skin) {
            const auto handled = draft.skin->handle(message, wparam, lparam);
            if (handled) return handled;
        }
        if (message == WM_CLOSE) {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
        if (message != WM_COMMAND) return FALSE;
        const auto control = LOWORD(wparam);
        const auto event = HIWORD(wparam);
        if(control==IDC_LANGUAGE && event==CBN_SELCHANGE) {
            const auto language=SendDlgItemMessageW(dialog,IDC_LANGUAGE,CB_GETCURSEL,0,0)==1?ui::Language::en:ui::Language::de;
            const bool saved=ui::set_language(language);refresh_language(dialog,draft);
            if(!saved) SetDlgItemTextW(dialog,IDC_STATUS,ui::tr(L"Die Sprache konnte nicht gespeichert werden."));
        } else if ((control == IDC_QUERY && event == EN_CHANGE) ||
            (control == IDC_CATEGORY && event == CBN_SELCHANGE)) {
            filter_catalog(dialog, draft);
        } else if (control == IDC_CATALOG && event == LBN_SELCHANGE) {
            select_effect(dialog, draft);
        } else if (control == IDC_ADD) {
            add_effect(dialog, draft);
        } else if (control == IDC_TARGET || control == IDC_SECOND_TARGET) {
            choose_targets(dialog, draft, control == IDC_SECOND_TARGET);
        } else if (control == IDC_UP || control == IDC_DOWN || control == IDC_REMOVE) {
            auto row = SendDlgItemMessageW(dialog, IDC_ACTIVE, LB_GETCURSEL, 0, 0);
            if (row < 0) return TRUE;
            if (control == IDC_REMOVE) {
                draft.effects.erase(draft.effects.begin() + row);
            } else {
                const auto next = row + (control == IDC_UP ? -1 : 1);
                if (next < 0 || next >= static_cast<LRESULT>(draft.effects.size())) return TRUE;
                std::swap(draft.effects[row], draft.effects[next]);
                row = next;
            }
            update_active(dialog, draft, row);
            SetDlgItemTextW(dialog, IDC_STATUS, L"");
        } else if (control == IDOK) {
            const auto error = validate_text(effect_command_text(draft.effects), 4095, true);
            if (const auto* known = known_failure_message(error)) {
                SetDlgItemTextW(dialog, IDC_STATUS, ui::tr(known));
                return TRUE;
            }
            if (!draft.apply(draft.effects)) {
                  SetDlgItemTextW(dialog, IDC_STATUS, draft.apply_error && !draft.apply_error->empty()
                      ? draft.apply_error->c_str() : kingdom::ui::tr(L"Dominion hat die Effektliste abgelehnt. Bitte prüfe die Ziele."));
                return TRUE;
            }
            if (draft.context == EditorContext::local)
                draft.automatic = IsDlgButtonChecked(dialog, IDC_AUTOMATIC) == BST_CHECKED;
            EndDialog(dialog, IDOK);
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

std::wstring target_access_message(const TargetAccessFailure& failure) {
    std::wstring effect(failure.effect_id.begin(), failure.effect_id.end());
    for (const auto& definition : effect_catalog) if (failure.effect_id == definition.id) { effect = effect_title(definition) + std::wstring(L" (") + effect + L")"; break; }
    const auto canonical = canonical_access_id(failure.target_id);
    std::wstring card(failure.target_id.begin(), failure.target_id.end()), expansion;
    for (const auto& target : target_catalog) if (canonical == target.id) { card = card_title(target); expansion = card_expansion(target); break; }
    const auto prefix = ui::pick(L"Effekt ", L"Effect ") + effect + (card.empty() ? L"" : L" · " + card) + L": ";
    if (failure.state == CardAccess::denied)
        return prefix + ui::pick(L"Diese Zielkarte ist für dein Konto derzeit nicht zum Spielen freigegeben", L"This target card is not currently available to play on your account") +
            (expansion.empty() ? L"" : L" (" + expansion + L")") + ui::pick(L". Entferne oder ersetze sie. Die bisherige Auswahl bleibt erhalten.", L". Remove or replace it. Your previous selection is preserved.");
    return prefix + ui::pick(L"Die Spielfreigabe dieser Zielkarte konnte nicht zuverlässig geprüft werden. Öffne die normale Sammlung oder Königreich-Auswahl erneut und versuche es dann noch einmal. Die bisherige Auswahl bleibt erhalten.", L"Play access for this target card could not be verified. Reopen the normal Collection or Kingdom selection and try again. Your previous selection is preserved.");
}

bool edit_effects(void* owner, std::string_view kingdom_text, const EffectCommands& effects, bool& automatic,
                  const std::function<bool(const EffectCommands&)>& apply, EditorContext context, const std::wstring* apply_error) {
    if (context == EditorContext::none) return false;
    HMODULE module{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(dialog_proc), &module)) return false;
    if (!register_effect_table(module)) return false;
    ui::load_language();
    Draft draft{effects, automatic, apply, fixed_targets(kingdom_text), context, apply_error};
    const auto result = DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_EFFECT_EDITOR),
                                       static_cast<HWND>(owner), dialog_proc, reinterpret_cast<LPARAM>(&draft));
    if (result == -1) {
        MessageBoxW(static_cast<HWND>(owner), kingdom::ui::tr(L"Der Effektdialog konnte nicht geöffnet werden."),
                    kingdom::ui::tr(L"Dominion – Königreich-Effekte"), MB_OK | MB_ICONWARNING);
    }
    if (result != IDOK) return false;
    automatic = draft.automatic;
    return true;
}

}
