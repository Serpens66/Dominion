// Actual native dialog controls on a separate desktop; no foreground input.
#include "effect_editor.cpp"
#include <cstdio>
#include <stdexcept>

namespace {
std::string failure;
bool checked = false;
UINT_PTR timer = 0;
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

BOOL CALLBACK inspect(HWND dialog, LPARAM) {
    if (!GetDlgItem(dialog, IDC_CATALOG)) return TRUE;
    KillTimer(nullptr, timer);
    try {
        const auto list = GetDlgItem(dialog, IDC_CATALOG);
        check(SendMessageW(list, LB_GETCOUNT, 0, 0) == 89, "Expected 89 visible effects.");
        for (LRESULT row = 0; row < 89; ++row) {
            check(SendMessageW(list, LB_GETITEMHEIGHT, row, 0) > 0, "Invalid row height.");
        }
        for (const auto query : {L"twist_mirror_loot", L"stamp_two_for_one", L"start_trash_curses"}) {
            SetDlgItemTextW(dialog, IDC_QUERY, query);
            check(SendMessageW(list, LB_GETCOUNT, 0, 0) == 1, "ID search mismatch.");
        }
        SetDlgItemTextW(dialog, IDC_QUERY, L"Ungetestet");
        check(SendMessageW(list, LB_GETCOUNT, 0, 0) == 27, "Untested effect labels missing.");
        SendMessageW(list, LB_SETCURSEL, 0, 0);
        SendMessageW(dialog, WM_COMMAND, MAKEWPARAM(IDC_CATALOG, LBN_SELCHANGE), reinterpret_cast<LPARAM>(list));
        check(IsWindowEnabled(GetDlgItem(dialog, IDC_ADD)) || IsWindowEnabled(GetDlgItem(dialog, IDC_TARGET)),
              "Untested effect cannot be chosen.");
        for (const auto query : {L"Zugbeginn", L"Spielaufbau"}) {
            SetDlgItemTextW(dialog, IDC_QUERY, query);
            check(SendMessageW(list, LB_GETCOUNT, 0, 0) > 0, "Timing search mismatch.");
        }
        SetDlgItemTextW(dialog, IDC_QUERY, L"no-such-effect-123456");
        check(SendMessageW(list, LB_GETCOUNT, 0, 0) == 0, "Empty search mismatch.");
        check(!IsWindowEnabled(GetDlgItem(dialog, IDC_ADD)), "Empty selection can be added.");
        SetDlgItemTextW(dialog, IDC_QUERY, L"");
        check(SendMessageW(list, LB_GETCOUNT, 0, 0) == 89, "Search reset mismatch.");
        checked = true;
    } catch (const std::exception& error) { failure = error.what(); }
    PostMessageW(dialog, WM_COMMAND, IDCANCEL, 0);
    return FALSE;
}
void CALLBACK inspect_dialog(HWND, UINT, UINT_PTR, DWORD) { EnumThreadWindows(GetCurrentThreadId(), inspect, 0); }
}

int main() {
    const auto name = L"DominionEditorReleaseTest_" + std::to_wstring(GetCurrentProcessId());
    const auto previous = GetThreadDesktop(GetCurrentThreadId());
    const auto desktop = CreateDesktopW(name.c_str(), nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    if (!desktop || !SetThreadDesktop(desktop)) {
        std::fputs("Cannot create an isolated desktop; no UI test was run.\n", stderr);
        if (desktop) CloseDesktop(desktop);
        return 1;
    }
    timer = SetTimer(nullptr, 0, 150, inspect_dialog);
    bool automatic = false;
    bool applied = false;
    const kingdom::EffectCommands effects{"start_double_pile(village)"};
    const bool saved = kingdom::edit_effects(nullptr, "-k village, market", effects, automatic,
        [&](const kingdom::EffectCommands&) { applied = true; return true; });
    KillTimer(nullptr, timer);
    SetThreadDesktop(previous);
    CloseDesktop(desktop);
    if (!checked || !failure.empty() || saved || applied || automatic) {
        std::fprintf(stderr, "Editor test failed: %s\n", failure.c_str());
        return 1;
    }
    std::puts("PASS: real native F8 dialog, catalog labels, ID/timing search, empty result and Cancel.");
    return 0;
}
