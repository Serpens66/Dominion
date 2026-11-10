#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "effect_editor.hpp"
#include "effect_editor_ids.h"

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
    TargetKind first;
    TargetKind second;
};

struct TargetDefinition {
    const char* id;
    const wchar_t* title;
    unsigned kinds;
};

#include "effect_catalog.generated.hpp"

struct Draft {
    EffectCommands effects;
    bool automatic;
    const std::function<bool(const EffectCommands&)>& apply;
    const EffectDefinition* selected = nullptr;
    std::vector<const TargetDefinition*> replacements;
};

std::wstring widen(std::string_view value) {
    return {value.begin(), value.end()}; // All native IDs and effect arguments are ASCII.
}

std::wstring lowercase(std::wstring value) {
    if (!value.empty()) CharLowerBuffW(value.data(), static_cast<DWORD>(value.size()));
    return value;
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

std::wstring target_name(const TargetDefinition& target) {
    return std::wstring(target.title) + L" (" + widen(target.id) + L")";
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
        const auto argument = trimmed(arguments.substr(0, comma));
        const auto* target = std::find_if(std::begin(target_catalog), std::end(target_catalog),
            [argument](const auto& entry) { return entry.id == argument; });
        label += target == std::end(target_catalog) ? widen(argument) : std::wstring(target->title);
        if (comma == arguments.npos) break;
        label += L", ";
        arguments.remove_prefix(comma + 1);
    }
    return label;
}

void update_active(HWND dialog, const Draft& draft, LRESULT selection = -1) {
    SendDlgItemMessageW(dialog, IDC_ACTIVE, LB_RESETCONTENT, 0, 0);
    for (const auto& command : draft.effects) {
        const auto label = active_label(command);
        SendDlgItemMessageW(dialog, IDC_ACTIVE, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    if (selection >= 0 && !draft.effects.empty()) {
        SendDlgItemMessageW(dialog, IDC_ACTIVE, LB_SETCURSEL,
                            (std::min)(selection, static_cast<LRESULT>(draft.effects.size() - 1)), 0);
    }
    const auto status = std::to_wstring(draft.effects.size()) + L" / 32 Effektplätze belegt";
    SetDlgItemTextW(dialog, IDC_STATUS, status.c_str());
}

void populate_targets(HWND dialog, int control, int label, TargetKind kind) {
    SendDlgItemMessageW(dialog, control, CB_RESETCONTENT, 0, 0);
    SetDlgItemTextW(dialog, label, target_label(kind));
    ShowWindow(GetDlgItem(dialog, label), kind == TargetKind::none ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(dialog, control), kind == TargetKind::none ? SW_HIDE : SW_SHOW);
    if (kind == TargetKind::none) return;
    const auto mask = kind_mask(kind == TargetKind::pile_list ? TargetKind::kingdom_pile : kind);
    for (const auto& target : target_catalog) {
        if (!(target.kinds & mask)) continue;
        const auto name = target_name(target);
        const auto row = SendDlgItemMessageW(dialog, control, CB_ADDSTRING, 0,
                                             reinterpret_cast<LPARAM>(name.c_str()));
        if (row >= 0) SendDlgItemMessageW(dialog, control, CB_SETITEMDATA, row,
                                          reinterpret_cast<LPARAM>(&target));
    }
    SendDlgItemMessageW(dialog, control, CB_LIMITTEXT, 240, 0);
}

void select_effect(HWND dialog, Draft& draft) {
    const auto row = SendDlgItemMessageW(dialog, IDC_CATALOG, LB_GETCURSEL, 0, 0);
    draft.selected = row < 0 ? nullptr : reinterpret_cast<const EffectDefinition*>(
        SendDlgItemMessageW(dialog, IDC_CATALOG, LB_GETITEMDATA, row, 0));
    const auto* effect = draft.selected;
    SetDlgItemTextW(dialog, IDC_DESCRIPTION, effect ? effect->description : L"");
    populate_targets(dialog, IDC_TARGET, IDC_TARGET_LABEL, effect ? effect->first : TargetKind::none);
    populate_targets(dialog, IDC_SECOND_TARGET, IDC_SECOND_LABEL, effect ? effect->second : TargetKind::none);
    draft.replacements.clear();
    SendDlgItemMessageW(dialog, IDC_MULTI_TARGETS, LB_RESETCONTENT, 0, 0);
    const bool multiple = effect && effect->first == TargetKind::pile_list;
    for (const auto id : {IDC_MULTI_ADD, IDC_MULTI_TARGETS, IDC_MULTI_REMOVE}) {
        ShowWindow(GetDlgItem(dialog, id), multiple ? SW_SHOW : SW_HIDE);
    }
    EnableWindow(GetDlgItem(dialog, IDC_ADD), effect != nullptr);
}

void filter_catalog(HWND dialog, Draft& draft) {
    const auto filter = lowercase(input(dialog, IDC_QUERY));
    const auto category = SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_GETCURSEL, 0, 0);
    const std::array<std::string_view, 4> prefixes{"", "twist_", "stamp_", "start_"};
    const auto prefix = category >= 0 ? prefixes[category] : prefixes[0];
    SendDlgItemMessageW(dialog, IDC_CATALOG, LB_RESETCONTENT, 0, 0);
    for (const auto& effect : effect_catalog) {
        const std::string_view id(effect.id);
        if (id.substr(0, prefix.size()) != prefix) continue;
        if (lowercase(effect.title).find(filter) == filter.npos &&
            lowercase(widen(id)).find(filter) == filter.npos &&
            lowercase(effect.description).find(filter) == filter.npos) continue;
        std::wstring name(effect.title);
        if (effect.first != TargetKind::none) name += L" · " + std::wstring(target_label(effect.first));
        const auto row = SendDlgItemMessageW(dialog, IDC_CATALOG, LB_ADDSTRING, 0,
                                             reinterpret_cast<LPARAM>(name.c_str()));
        if (row >= 0) SendDlgItemMessageW(dialog, IDC_CATALOG, LB_SETITEMDATA, row,
                                          reinterpret_cast<LPARAM>(&effect));
    }
    SendDlgItemMessageW(dialog, IDC_CATALOG, LB_SETCURSEL, 0, 0);
    select_effect(dialog, draft);
}

const TargetDefinition* chosen_target(HWND dialog, int control, TargetKind kind) {
    const auto row = SendDlgItemMessageW(dialog, control, CB_GETCURSEL, 0, 0);
    if (row >= 0) return reinterpret_cast<const TargetDefinition*>(
        SendDlgItemMessageW(dialog, control, CB_GETITEMDATA, row, 0));
    const auto typed = lowercase(input(dialog, control));
    if (typed.empty()) return nullptr;
    const auto mask = kind_mask(kind == TargetKind::pile_list ? TargetKind::kingdom_pile : kind);
    const TargetDefinition* match = nullptr;
    for (const auto& target : target_catalog) {
        if (!(target.kinds & mask)) continue;
        if (lowercase(widen(target.id)) == typed || lowercase(target_name(target)) == typed) return &target;
        if (lowercase(target.title) == typed) {
            if (match) return nullptr; // Ambiguous localized title. Pick a row or enter the ID.
            match = &target;
        }
    }
    return match;
}

void add_effect(HWND dialog, Draft& draft) {
    if (!draft.selected) return;
    if (draft.effects.size() == maximum_effects) {
        SetDlgItemTextW(dialog, IDC_STATUS, L"Alle 32 Effektplätze sind belegt.");
        return;
    }
    const auto& effect = *draft.selected;
    std::string command(effect.id);
    if (effect.first == TargetKind::pile_list) {
        if (draft.replacements.empty()) {
            SetDlgItemTextW(dialog, IDC_STATUS, L"Merke mindestens einen Ersatzstapel vor.");
            return;
        }
        char separator = '(';
        for (const auto* target : draft.replacements) {
            command += separator;
            command += target->id;
            separator = ',';
        }
        command += ')';
    } else if (effect.first != TargetKind::none) {
        const auto* first = chosen_target(dialog, IDC_TARGET, effect.first);
        const auto* second = chosen_target(dialog, IDC_SECOND_TARGET, effect.second);
        if (!first || (effect.second != TargetKind::none && !second)) {
            SetDlgItemTextW(dialog, IDC_STATUS, L"Bitte wähle die passenden Ziele aus der Liste.");
            return;
        }
        command += '(';
        command += first->id;
        if (second) {
            command += ',';
            command += second->id;
        }
        command += ')';
    }
    draft.effects.push_back(std::move(command));
    const auto error = validate_text(effect_command_text(draft.effects), 4095, true);
    if (error != Error::none) {
        draft.effects.pop_back();
        const auto* known = known_failure_message(error);
        SetDlgItemTextW(dialog, IDC_STATUS, known ? known : error == Error::shared_target
            ? L"Platz für Mehr und Taifun dürfen jeweils nur einmal vorkommen."
            : L"Diese Effektliste kann das Spiel nicht übernehmen.");
        return;
    }
    update_active(dialog, draft, static_cast<LRESULT>(draft.effects.size() - 1));
}

void edit_replacements(HWND dialog, Draft& draft, bool add) {
    if (add) {
        const auto* target = chosen_target(dialog, IDC_TARGET, TargetKind::pile_list);
        if (!target || draft.replacements.size() == 20 ||
            std::find(draft.replacements.begin(), draft.replacements.end(), target) != draft.replacements.end()) {
            SetDlgItemTextW(dialog, IDC_STATUS, L"Wähle einen neuen Ersatzstapel. Maximal 20 sind möglich.");
            return;
        }
        draft.replacements.push_back(target);
        SendDlgItemMessageW(dialog, IDC_MULTI_TARGETS, LB_ADDSTRING, 0,
                            reinterpret_cast<LPARAM>(target->title));
        SetDlgItemTextW(dialog, IDC_TARGET, L"");
    } else {
        const auto row = SendDlgItemMessageW(dialog, IDC_MULTI_TARGETS, LB_GETCURSEL, 0, 0);
        if (row < 0) return;
        draft.replacements.erase(draft.replacements.begin() + row);
        SendDlgItemMessageW(dialog, IDC_MULTI_TARGETS, LB_DELETESTRING, row, 0);
    }
    const auto status = std::to_wstring(draft.replacements.size()) + L" / 20 Ersatzstapel vorgemerkt";
    SetDlgItemTextW(dialog, IDC_STATUS, status.c_str());
}

INT_PTR CALLBACK dialog_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    try {
        if (message == WM_INITDIALOG) {
            auto& draft = *reinterpret_cast<Draft*>(lparam);
            SetWindowLongPtrW(dialog, DWLP_USER, lparam);
            for (const auto* category : {L"Alle Kategorien", L"Spielvariationen", L"Stempel", L"Spielaufbau"}) {
                SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(category));
            }
            SendDlgItemMessageW(dialog, IDC_CATEGORY, CB_SETCURSEL, 0, 0);
            CheckDlgButton(dialog, IDC_AUTOMATIC, draft.automatic ? BST_CHECKED : BST_UNCHECKED);
            filter_catalog(dialog, draft);
            update_active(dialog, draft);
            return TRUE;
        }
        if (message != WM_COMMAND) return FALSE;
        const auto state = GetWindowLongPtrW(dialog, DWLP_USER);
        if (!state) return FALSE;
        auto& draft = *reinterpret_cast<Draft*>(state);
        const auto control = LOWORD(wparam);
        const auto event = HIWORD(wparam);
        if ((control == IDC_QUERY && event == EN_CHANGE) ||
            (control == IDC_CATEGORY && event == CBN_SELCHANGE)) {
            filter_catalog(dialog, draft);
        } else if (control == IDC_CATALOG && event == LBN_SELCHANGE) {
            select_effect(dialog, draft);
        } else if (control == IDC_ADD) {
            add_effect(dialog, draft);
        } else if (control == IDC_MULTI_ADD || control == IDC_MULTI_REMOVE) {
            edit_replacements(dialog, draft, control == IDC_MULTI_ADD);
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

bool edit_effects(void* owner, const EffectCommands& effects, bool& automatic,
                  const std::function<bool(const EffectCommands&)>& apply) {
    HMODULE module{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(dialog_proc), &module)) return false;
    Draft draft{effects, automatic, apply};
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
