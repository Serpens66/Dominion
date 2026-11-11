#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "effect_editor.hpp"
#include "effect_editor_ids.h"
#include "card_picker.hpp"
#include "dialog_skin.hpp"
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
    unsigned timing_mask;
    TargetKind first;
    TargetKind second;
};

#include "effect_catalog.generated.hpp"

struct Draft {
    EffectCommands effects;
    bool automatic;
    const std::function<bool(const EffectCommands&)>& apply;
    CardTargets fixed;
    EditorContext context;
    const EffectDefinition* selected = nullptr;
    const CardTarget* first = nullptr;
    const CardTarget* second = nullptr;
    std::unique_ptr<DialogSkin> skin{};
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
        case TargetKind::card: return L"Karte";
        case TargetKind::pile: return L"Stapel";
        case TargetKind::kingdom_pile: return L"Königreichstapel";
        case TargetKind::pile_list: return L"Ersatzstapel (bis zu 20)";
        case TargetKind::event: return L"Ereignis";
        case TargetKind::landmark: return L"Landmarke";
        case TargetKind::way: return L"Weg";
        case TargetKind::prophecy: return L"Prophezeiung";
        case TargetKind::project: return L"Projekt";
        case TargetKind::duration: return L"Dauerkarte";
        case TargetKind::heirloom_parent: return L"Königreichkarte mit Erbstück";
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
    std::wstring label(effect->title);
    if (opening == command.npos) return label;
    auto arguments = command.substr(opening + 1, command.size() - opening - 2);
    label += L" · ";
    while (!arguments.empty()) {
        const auto comma = arguments.find(',');
        const auto argument = canonical_card(trimmed(arguments.substr(0, comma)));
        const auto* target = std::find_if(std::begin(target_catalog), std::end(target_catalog),
            [argument](const auto& entry) { return entry.id == argument; });
        label += target == std::end(target_catalog) ? widen(argument) : std::wstring(target->title);
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
    std::wstring name = std::wstring(effect.title) + L" (" + widen(effect.id) + L")";
    if (effect.first != TargetKind::none) name += L" · " + std::wstring(target_label(effect.first));
    if (*effect.verification) name += L" · " + std::wstring(effect.verification);
    return name;
}

std::array<std::wstring, 3> table_text(const EffectDefinition& effect) {
    auto name = table_name(effect);
    // Allow the native text renderer to wrap long IDs at their underscores.
    for (std::size_t at = 0; (at = name.find(L'_', at)) != name.npos; at += 2)
        name.insert(at + 1, 1, L'\u200B');
    return {std::move(name), effect.description, effect.timing};
}

constexpr UINT table_text_flags = DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;

void measure_rows(HWND dialog, const Draft& draft) {
    const auto list = GetDlgItem(dialog, IDC_CATALOG);
    const auto dc = GetDC(list);
    if (!dc) return;
    const auto font = reinterpret_cast<HFONT>(SendMessageW(list, WM_GETFONT, 0, 0));
    const auto previous = SelectObject(dc, font);
    const auto padding = draft.skin->units({0, 0, 4, 3});
    SendMessageW(list,effect_table_padding,padding.right,padding.bottom);
    // Updating row heights can make the vertical scrollbar appear.
    LONG previous_width = -1;
    for (int pass = 0; pass < 2; ++pass) {
        RECT client{};
        GetClientRect(list, &client);
        if (client.right == previous_width) break;
        previous_width = client.right;
        const auto cells = table_cells(dialog, client);
        const auto count = SendMessageW(list, LB_GETCOUNT, 0, 0);
        for (LRESULT row = 0; row < count; ++row) {
            const auto* effect = reinterpret_cast<const EffectDefinition*>(SendMessageW(list, LB_GETITEMDATA, row, 0));
            const auto text = table_text(*effect);
            LONG height = 0;
            for (std::size_t column = 0; column < cells.size(); ++column) {
                RECT bounds{0, 0, cells[column].right - cells[column].left - 2 * padding.right, 0};
                DrawTextW(dc, text[column].c_str(), -1, &bounds, table_text_flags | DT_CALCRECT);
                height = (std::max)(height, bounds.bottom);
            }
            SendMessageW(list, LB_SETITEMHEIGHT, row, height + 2 * padding.bottom + 1);
        }
    }
    SelectObject(dc, previous);
    ReleaseDC(list, dc);
    InvalidateRect(GetDlgItem(dialog, IDC_CATALOG_HEADER), nullptr, TRUE);
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
    const auto text = header ? std::array<std::wstring, 3>{L"Effekt", L"Wirkung", L"Wann/wie"} :
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
        DrawTextW(draw.hDC, text[column].c_str(), -1, &bounds, table_text_flags);
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
    const auto status = std::to_wstring(draft.effects.size()) + L" / 32 Effekte";
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
        SetDlgItemTextW(dialog, IDC_TARGET, draft.first ? draft.first->title :
                       multiple ? L"Ziele wählen…" : menu_text::choose_card);
    }
    if (needs_second)
        SetDlgItemTextW(dialog, IDC_SECOND_TARGET, draft.second ? draft.second->title : L"Zweites Ziel…");
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
    SetDlgItemTextW(dialog, IDC_EFFECT_EVIDENCE, selection ? selection->status : L"");
    update_targets(dialog, draft);
}

void filter_catalog(HWND dialog, Draft& draft) {
    const auto filter = fold_card_search(input(dialog, IDC_QUERY));
    const auto category = SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_GETCURSEL, 0, 0);
    const std::array<std::string_view, 4> prefixes{"", "twist_", "stamp_", "start_"};
    const auto prefix = category >= 0 && category < 4 ? prefixes[category] : prefixes[0];
    const unsigned timing = category >= 4 && category < 4 + std::size(timing_filters) ? 1u << (category - 4) : 0;
    SendDlgItemMessageW(dialog, IDC_CATALOG, WM_SETREDRAW, FALSE, 0);
    SendDlgItemMessageW(dialog, IDC_CATALOG, LB_RESETCONTENT, 0, 0);
    LRESULT selection = 0;
    for (const auto& effect : effect_catalog) {
        const std::string_view id(effect.id);
        if (id.substr(0, prefix.size()) != prefix || (timing && !(effect.timing_mask & timing))) continue;
        if (fold_card_search(effect.title).find(filter) == filter.npos &&
            fold_card_search(widen(id)).find(filter) == filter.npos &&
            fold_card_search(effect.description).find(filter) == filter.npos &&
            fold_card_search(effect.timing).find(filter) == filter.npos &&
            fold_card_search(effect.verification).find(filter) == filter.npos) continue;
        const auto name = table_name(effect);
        const auto row = SendDlgItemMessageW(dialog, IDC_CATALOG, LB_ADDSTRING, 0,
                                             reinterpret_cast<LPARAM>(name.c_str()));
        if (row >= 0) SendDlgItemMessageW(dialog, IDC_CATALOG, LB_SETITEMDATA, row,
                                          reinterpret_cast<LPARAM>(&effect));
        if (row >= 0) {
            const auto cells=table_text(effect);
            SendDlgItemMessageW(dialog,IDC_CATALOG,effect_table_cells,row,reinterpret_cast<LPARAM>(&cells));
        }
        if (&effect == draft.selected && row >= 0) selection = row;
    }
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
    const auto previous = draft.effects.size();
    const auto result = append_editor_effects(draft.effects, commands, skip_existing);
    if (const auto* known = known_failure_message(result.error)) return known;
    if (result.error == Error::capacity)
        return result.missing_slots == 1 ? L"Für diese Auswahl fehlt ein Effektplatz." :
               L"Für diese Auswahl fehlen " + std::to_wstring(result.missing_slots) + L" Effektplätze.";
    if (result.error == Error::shared_target) return L"Platz für Mehr und Taifun sind jeweils einmal möglich.";
    if (result.error != Error::none) return L"Diese Effektliste kann das Spiel nicht übernehmen.";
    const auto added = draft.effects.size() - previous;
    const auto skipped = commands.size() - added;
    if (added) update_active(dialog, draft, static_cast<LRESULT>(draft.effects.size() - 1));
    const auto status = std::to_wstring(added) + L" hinzugefügt" +
        (skipped ? L" · " + std::to_wstring(skipped) + L" bereits vorhanden" : L"");
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
    for (const auto& card : target_catalog) {
        if (card.kinds & mask) cards.push_back(&card);
    }
    if (mode == CardSelection::multiple) {
        for (const auto* card : draft.fixed)
            if (card->kinds & mask) current.push_back(card);
    }
    const auto title = std::wstring(effect.title) + L" · " + target_label(kind);
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

INT_PTR CALLBACK dialog_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    try {
        if (message == WM_INITDIALOG) {
            auto& draft = *reinterpret_cast<Draft*>(lparam);
            SetWindowLongPtrW(dialog, DWLP_USER, lparam);
            const bool online = draft.context == EditorContext::online;
            const auto title = std::wstring(menu_text::kingdom) + (online ? L" – Online – Wirkung serverabhängig" : L"");
            SetWindowTextW(dialog, title.c_str());
            draft.skin = std::make_unique<DialogSkin>(dialog);
            SetDlgItemTextW(dialog, IDC_EFFECT_LABEL, menu_text::features);
            SetDlgItemTextW(dialog, IDOK, menu_text::done);
            SetDlgItemTextW(dialog, IDCANCEL, menu_text::cancel);
            SendDlgItemMessageW(dialog, IDC_QUERY, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"Name oder Effekt-ID"));
            for (const auto* category : {L"Alle Kategorien", L"Effektart: Spielvariationen", L"Effektart: Stempel", L"Effektart: Spielaufbau"}) {
                SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(category));
            }
            for (const auto* label : timing_filters) {
                const auto name = std::wstring(L"Wann/wie: ") + label;
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
            SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_SETCURSEL, 0, 0);
            CheckDlgButton(dialog, IDC_AUTOMATIC, !online && draft.automatic ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(GetDlgItem(dialog, IDC_AUTOMATIC), !online);
            if (online) SetDlgItemTextW(dialog, IDC_AUTOMATIC, L"Automatische Verdopplung nur lokal");
            filter_catalog(dialog, draft);
            update_active(dialog, draft);
            return TRUE;
        }
        const auto state = GetWindowLongPtrW(dialog, DWLP_USER);
        if (!state) return FALSE;
        auto& draft = *reinterpret_cast<Draft*>(state);
        if (message == effect_table_layout && draft.skin) {
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
        if ((control == IDC_QUERY && event == EN_CHANGE) ||
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
                SetDlgItemTextW(dialog, IDC_STATUS, known);
                return TRUE;
            }
            if (!draft.apply(draft.effects)) {
                SetDlgItemTextW(dialog, IDC_STATUS, L"Dominion hat die Effektliste abgelehnt. Bitte prüfe die Ziele.");
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

bool edit_effects(void* owner, std::string_view kingdom_text, const EffectCommands& effects, bool& automatic,
                  const std::function<bool(const EffectCommands&)>& apply, EditorContext context) {
    if (context == EditorContext::none) return false;
    HMODULE module{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(dialog_proc), &module)) return false;
    if (!register_effect_table(module)) return false;
    Draft draft{effects, automatic, apply, fixed_targets(kingdom_text), context};
    const auto result = DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_EFFECT_EDITOR),
                                       static_cast<HWND>(owner), dialog_proc, reinterpret_cast<LPARAM>(&draft));
    if (result == -1) {
        MessageBoxW(static_cast<HWND>(owner), L"Der Effektdialog konnte nicht geöffnet werden.",
                    L"Dominion – Königreich-Effekte", MB_OK | MB_ICONWARNING);
    }
    if (result != IDOK) return false;
    automatic = draft.automatic;
    return true;
}

}
