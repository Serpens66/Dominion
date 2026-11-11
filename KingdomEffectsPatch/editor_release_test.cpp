// Actual native dialog controls on a separate desktop; no foreground input.
#include "effect_editor.cpp"
#include <cstdio>
#include <stdexcept>

namespace {
std::string failure;
bool checked = false;
bool picker_checked = false;
unsigned picker_ticks = 0;
bool online_checked = false;
bool online_cancel = false;
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
BOOL CALLBACK inspect_online(HWND dialog, LPARAM) {
    if (!GetDlgItem(dialog, IDC_CATALOG)) return TRUE;
    KillTimer(nullptr, timer);
    try {
        wchar_t caption[256]{}; GetWindowTextW(dialog,caption,256);
        check(std::wstring_view(caption).find(L"Online – Wirkung serverabhängig") != std::wstring_view::npos, "Online context missing from title.");
        check(!IsWindowEnabled(GetDlgItem(dialog,IDC_AUTOMATIC)), "Online automation is enabled.");
        check(IsDlgButtonChecked(dialog,IDC_AUTOMATIC)==BST_UNCHECKED, "Local automation shown as active online.");
        auto& draft=*reinterpret_cast<kingdom::Draft*>(GetWindowLongPtrW(dialog,DWLP_USER));
        const kingdom::EffectCommands pair{"start_double_pile(village)","start_double_pile(market)"};
        check(kingdom::add_commands(dialog,draft,pair,true).empty(), "Online multi-target append failed.");
        check(draft.effects==pair, "Online selection mismatch.");
        check(kingdom::add_commands(dialog,draft,{pair.front()},false).empty() && draft.effects.size()==3, "Online repeat failed.");
        while (draft.effects.size()<32) check(kingdom::add_commands(dialog,draft,{pair.front()},false).empty(), "Online capacity fill failed.");
        const auto saved=draft.effects;
        check(!kingdom::add_commands(dialog,draft,{pair.front()},false).empty() && draft.effects==saved, "Online capacity rejection lost draft.");
        online_checked=true;
    } catch (const std::exception& error) { failure=error.what(); }
    PostMessageW(dialog,WM_COMMAND,online_cancel?IDCANCEL:IDOK,0);
    return FALSE;
}
void CALLBACK inspect_online_dialog(HWND, UINT, UINT_PTR, DWORD) { EnumThreadWindows(GetCurrentThreadId(),inspect_online,0); }
BOOL CALLBACK inspect_picker(HWND dialog, LPARAM) {
    if (!GetDlgItem(dialog, IDC_CARD_TREE)) return TRUE;
    try {
        if (!picker_ticks++) {
            for (const auto query : {L"village", L"market", L"no-such-card", L"miser"}) SetDlgItemTextW(dialog, IDC_CARD_QUERY, query);
        }
        if (!IsWindowVisible(GetDlgItem(dialog, IDC_CARD_IMAGE)) && picker_ticks<200) return FALSE;
        check(IsWindowVisible(GetDlgItem(dialog, IDC_CARD_IMAGE)), "Async image did not arrive.");
        wchar_t name[128]{}; GetDlgItemTextW(dialog, IDC_CARD_IMAGE, name, 128);
        check(std::wstring_view(name)==L"Geizhals", "Stale preview published after navigation.");
        SendMessageW(dialog, WM_COMMAND, IDC_CARD_TEXT, 0);
        check(IsWindowVisible(GetDlgItem(dialog, IDC_CARD_RULES)), "Rules switch failed.");
        SetDlgItemTextW(dialog, IDC_CARD_QUERY, L"city_quarter");
        picker_checked=true;
    } catch (const std::exception& error) { failure=error.what(); }
    KillTimer(nullptr,timer); PostMessageW(dialog,WM_COMMAND,IDCANCEL,0); return FALSE;
}
void CALLBACK inspect_card_dialog(HWND, UINT, UINT_PTR, DWORD) { EnumThreadWindows(GetCurrentThreadId(),inspect_picker,0); }
}

int main() {
    wchar_t package[32768]{};
    if (GetEnvironmentVariableW(L"DOMINION_RESOURCE_TEST_PACKAGE",package,32768)) {
        if (!kingdom::set_resource_package_for_test(package)) return 1;
    }
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
    // Each profile-backed online configuration supplies its own apply callback.
    std::array<kingdom::EffectCommands,3> configurations{effects,{}, {"twist_big_ideas"}};
    const auto local_before=configurations[0];
    bool local_automatic=true;
    for (std::size_t target=1;target<=2;++target) {
        online_checked=false; online_cancel=false; timer=SetTimer(nullptr,0,150,inspect_online_dialog);
        const auto other=configurations[3-target];
        check(kingdom::edit_effects(nullptr,"-k village, market",{},local_automatic,
            [&](const kingdom::EffectCommands& value) { configurations[target]=value; return true; },kingdom::EditorContext::online), "Online accept failed.");
        check(online_checked && local_automatic && configurations[target].size()==32 && configurations[3-target]==other && configurations[0]==local_before, "Online change affected another configuration or automation.");
    }
    online_cancel=true; online_checked=false; timer=SetTimer(nullptr,0,150,inspect_online_dialog);
    check(!kingdom::edit_effects(nullptr,"-k village, market",{},local_automatic,
        [&](const kingdom::EffectCommands&) { applied=true; return true; },kingdom::EditorContext::online) && online_checked && !applied && local_automatic, "Online cancel changed the saved selection.");
    if (*package) {
        kingdom::CardTargets cards;
        for (const auto& card:kingdom::target_catalog) cards.push_back(&card);
        timer=SetTimer(nullptr,0,50,inspect_card_dialog);
        kingdom::pick_cards(nullptr,L"Ressourcentest",cards,{},kingdom::CardSelection::multiple,
            [&](const kingdom::CardTargets&,bool) { applied=true; return std::wstring{}; });
        KillTimer(nullptr,timer);
        if (!picker_checked) failure="Picker check did not run.";
    }
    SetThreadDesktop(previous);
    CloseDesktop(desktop);
    if (!checked || !failure.empty() || saved || applied || automatic) {
        std::fprintf(stderr, "Editor test failed: %s\n", failure.c_str());
        return 1;
    }
    std::puts("PASS: native local/online dialogs, isolated apply callbacks, automation preserved, 32-entry limit, search, preview and Cancel.");
    return 0;
}
