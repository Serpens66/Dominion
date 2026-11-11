// Actual native dialog controls on a separate desktop; no foreground input.
#define EFFECT_TABLE_TEST_CLIPBOARD
#include "effect_editor.cpp"
#include "card_picker.cpp"
#include <cstdio>
#include <stdexcept>
#pragma comment(lib, "oleaut32.lib")

namespace {
std::string failure;
bool checked = false;
bool picker_checked = false;
unsigned picker_ticks = 0;
bool missing_art=false;
bool online_checked = false;
bool online_cancel = false;
UINT_PTR timer = 0;
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

void check_scroll_geometry(HWND dialog) {
    const auto list=GetDlgItem(dialog,IDC_CATALOG);
    const auto count=SendMessageW(list,LB_GETCOUNT,0,0);
    std::vector<LRESULT> heights;
    int total=0;
    for (LRESULT row=0;row<count;++row) { const auto h=SendMessageW(list,LB_GETITEMHEIGHT,row,0); check(h>0,"Unmeasured row."); heights.push_back(h); total+=int(h); }
    SCROLLINFO before{sizeof before,SIF_ALL}; GetScrollInfo(list,SB_VERT,&before);
    check(before.nMax==(std::max)(0,total-1),"Initial range does not contain every row.");
    for (int step=0;step<200;++step) {
        SendMessageW(list,WM_VSCROLL,SB_PAGEDOWN,0);
        SCROLLINFO after{sizeof after,SIF_ALL}; GetScrollInfo(list,SB_VERT,&after);
        check(after.nMax==before.nMax && after.nPage==before.nPage,"Range grows during scrolling.");
        if (after.nPos==after.nMax-int(after.nPage)+1) break;
    }
    SendMessageW(list,WM_VSCROLL,SB_BOTTOM,0);
    RECT last{},client{}; GetClientRect(list,&client);
    SendMessageW(list,LB_GETITEMRECT,count-1,reinterpret_cast<LPARAM>(&last));
    check(last.bottom<=client.bottom && last.bottom>0,"Last row is not reachable.");
    for (LRESULT row=0;row<count;++row) check(SendMessageW(list,LB_GETITEMHEIGHT,row,0)==heights[row],"Row changes after scrolling.");
    SendMessageW(list,WM_KEYDOWN,VK_HOME,0); check(SendMessageW(list,LB_GETCURSEL,0,0)==0,"Home navigation failed.");
    SendMessageW(list,WM_KEYDOWN,VK_END,0); check(SendMessageW(list,LB_GETCURSEL,0,0)==count-1,"End navigation failed.");
    SendMessageW(list,WM_KEYDOWN,VK_HOME,0);
}

BOOL CALLBACK inspect(HWND dialog, LPARAM) {
    if (!GetDlgItem(dialog, IDC_CATALOG)) return TRUE;
    KillTimer(nullptr, timer);
    try {
        const auto list = GetDlgItem(dialog, IDC_CATALOG);
        check(SendMessageW(list, LB_GETCOUNT, 0, 0) == 89, "Expected 89 visible effects.");
        IAccessible* accessible=nullptr;
        check(SUCCEEDED(AccessibleObjectFromWindow(list,static_cast<DWORD>(OBJID_CLIENT),__uuidof(IAccessible),reinterpret_cast<void**>(&accessible))),"Table accessibility unavailable.");
        LONG children=0; const auto counted=accessible->get_accChildCount(&children);
        VARIANT first{}; first.vt=VT_I4; first.lVal=1; BSTR name=nullptr;
        const auto named=accessible->get_accName(first,&name);
        const bool readable=SUCCEEDED(named) && name && SysStringLen(name)>0;
        if (name) SysFreeString(name); accessible->Release();
        check(SUCCEEDED(counted) && children==89 && readable,"Accessible rows missing.");
        check_scroll_geometry(dialog);
        const auto font=reinterpret_cast<HFONT>(SendMessageW(list,WM_GETFONT,0,0));
        LOGFONTW original{}; check(GetObjectW(font,sizeof original,&original)!=0,"Missing table font.");
        for (const auto percent:{125,150}) {
            auto enlarged=original; enlarged.lfHeight=MulDiv(original.lfHeight,percent,100);
            const auto temporary=CreateFontIndirectW(&enlarged);
            SendMessageW(list,WM_SETFONT,reinterpret_cast<WPARAM>(temporary),0); check_scroll_geometry(dialog);
            SendMessageW(list,WM_SETFONT,reinterpret_cast<WPARAM>(font),0); DeleteObject(temporary);
        }
        RECT original_bounds{}; GetWindowRect(list,&original_bounds); MapWindowPoints(nullptr,dialog,reinterpret_cast<POINT*>(&original_bounds),2);
        SetWindowPos(list,nullptr,0,0,(original_bounds.right-original_bounds.left)/2,120,SWP_NOMOVE|SWP_NOZORDER);
        check_scroll_geometry(dialog);
        SendMessageW(list,LB_SETITEMHEIGHT,0,700);
        SendMessageW(list,WM_VSCROLL,SB_TOP,0); SendMessageW(list,WM_VSCROLL,SB_PAGEDOWN,0);
        RECT partial{}; SendMessageW(list,LB_GETITEMRECT,0,reinterpret_cast<LPARAM>(&partial));
        check(partial.top<0 && partial.bottom>0,"Cannot scroll within a tall row.");
        SetWindowPos(list,nullptr,0,0,original_bounds.right-original_bounds.left,original_bounds.bottom-original_bounds.top,SWP_NOMOVE|SWP_NOZORDER);
        check_scroll_geometry(dialog);
        check(SendDlgItemMessageW(dialog,IDC_CATEGORY,CB_GETCOUNT,0,0)==17,"Missing trigger filters.");
        const auto combo=GetDlgItem(dialog,IDC_CATEGORY);
        const auto dc=GetDC(combo); const auto previous_font=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(combo,WM_GETFONT,0,0)));
        for (int index=0;index<17;++index) {
            wchar_t label[256]{}; SendMessageW(combo,CB_GETLBTEXT,index,reinterpret_cast<LPARAM>(label));
            SIZE width{}; GetTextExtentPoint32W(dc,label,lstrlenW(label),&width);
            check(SendMessageW(combo,CB_GETDROPPEDWIDTH,0,0)>width.cx,"Dropdown clips a filter label.");
        }
        SelectObject(dc,previous_font); ReleaseDC(combo,dc);
        for (int category=0;category<17;++category) {
            SendDlgItemMessageW(dialog,IDC_CATEGORY,CB_SETCURSEL,category,0);
            SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(IDC_CATEGORY,CBN_SELCHANGE),0);
            check(SendMessageW(list,LB_GETCOUNT,0,0)>0,"Empty documented category.");
            for (LRESULT row=0;row<SendMessageW(list,LB_GETCOUNT,0,0);++row) {
                const auto* effect=reinterpret_cast<const kingdom::EffectDefinition*>(SendMessageW(list,LB_GETITEMDATA,row,0));
                if (category>=4) check((effect->timing_mask&(1u<<(category-4)))!=0,"Incorrect trigger filter result.");
            }
            check_scroll_geometry(dialog);
        }
        SendDlgItemMessageW(dialog,IDC_CATEGORY,CB_SETCURSEL,5,0);
        SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(IDC_CATEGORY,CBN_SELCHANGE),0);
        SetDlgItemTextW(dialog,IDC_QUERY,L"twist_big_ideas");
        check(SendMessageW(list,LB_GETCOUNT,0,0)==1,"Search and trigger filter not combined.");
        SetDlgItemTextW(dialog,IDC_QUERY,L"start_double_pile");
        check(SendMessageW(list,LB_GETCOUNT,0,0)==0,"Combined filter should be empty.");
        SendDlgItemMessageW(dialog,IDC_CATEGORY,CB_SETCURSEL,0,0);
        SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(IDC_CATEGORY,CBN_SELCHANGE),0);
        SetDlgItemTextW(dialog,IDC_QUERY,L"");
        for (LRESULT row = 0; row < 89; ++row) {
            check(SendMessageW(list, LB_GETITEMHEIGHT, row, 0) > 0, "Invalid row height.");
        }
        for (const auto query : {L"twist_mirror_loot", L"stamp_two_for_one", L"start_trash_curses"}) {
            SetDlgItemTextW(dialog, IDC_QUERY, query);
            check(SendMessageW(list, LB_GETCOUNT, 0, 0) == 1, "ID search mismatch.");
        }
        SetDlgItemTextW(dialog, IDC_QUERY, L"Ungetestet");
        const auto untested=std::count_if(std::begin(kingdom::effect_catalog),std::end(kingdom::effect_catalog),[](const auto& effect) {
            return kingdom::fold_card_search(effect.verification).find(L"ungetestet")!=std::wstring::npos;
        });
        check(SendMessageW(list, LB_GETCOUNT, 0, 0) == untested, "Untested effect labels missing.");
        SendMessageW(list,LB_SETSEL,TRUE,-1); SendMessageW(list,WM_COPY,0,0);
        const auto copied=kingdom::effect_table_detail::test_clipboard;
        check(copied.find(L"Effekt\tWirkung\tWann/wie\r\n")==0 && copied.find(L"Ungetestet")!=copied.npos,"Filtered table copy failed.");
        check(std::count(copied.begin(),copied.end(),L'\n')==untested,"Copy did not include all filtered rows.");
        check(copied.find(L"twist_big_ideas")==copied.npos && copied.find(L'\u200B')==copied.npos,"Copy includes hidden rows or layout characters.");
        auto& table=*reinterpret_cast<kingdom::effect_table_detail::Model*>(GetWindowLongPtrW(list,GWLP_USERDATA));
        table.select(list,0,true);
        const auto text_editor=table.open_cell(list,0,0); check(text_editor!=nullptr,"Selectable cell unavailable.");
        SendMessageW(text_editor,EM_SETSEL,0,-1); SendMessageW(text_editor,WM_COPY,0,0);
        check(kingdom::effect_table_detail::test_clipboard==kingdom::effect_table_detail::plain_copy(table.rows[0].cells[0]),"Selected text copy failed.");
        SendMessageW(text_editor,EM_SETSEL,2,7); SendMessageW(text_editor,WM_COPY,0,0);
        check(kingdom::effect_table_detail::test_clipboard==kingdom::effect_table_detail::plain_copy(table.rows[0].cells[0].substr(2,5)),"Partial text copy failed.");
        check((GetWindowLongPtrW(text_editor,GWL_STYLE)&ES_READONLY)!=0,"Table text must remain read-only.");
        SendMessageW(text_editor,WM_KEYDOWN,VK_ESCAPE,0); check(!table.editor,"Escape did not end text selection.");
        SendMessageW(list,WM_COPY,0,0); check(kingdom::effect_table_detail::test_clipboard.find(L'\t')!=std::wstring::npos,"Single effect row copy failed.");
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
    if (!IsWindowVisible(dialog)) return FALSE;
    try {
        if (!picker_ticks++) {
            for (const auto query : {L"village", L"market", L"no-such-card", L"miser"}) SetDlgItemTextW(dialog, IDC_CARD_QUERY, query);
        }
        auto& picker=*reinterpret_cast<kingdom::Picker*>(GetWindowLongPtrW(dialog,DWLP_USER));
        // Text and image controls keep the same geometry before/after loading.
        check(IsWindowVisible(GetDlgItem(dialog, IDC_CARD_RULES)), "Rules hidden while loading.");
        if ((!missing_art && !picker.image || missing_art && picker.loader.counts().second==0) && picker_ticks<200) return FALSE;
        check(missing_art ? !picker.image : bool(picker.image),"Async image/fallback state incorrect.");
        check(IsWindowVisible(GetDlgItem(dialog, IDC_CARD_IMAGE)), "Image area hidden.");
        wchar_t name[128]{}; GetDlgItemTextW(dialog, IDC_CARD_IMAGE, name, 128);
        check(std::wstring_view(name)==L"Geizhals", "Stale preview published after navigation.");
        check(IsWindowVisible(GetDlgItem(dialog, IDC_CARD_TITLE)) && IsWindowVisible(GetDlgItem(dialog, IDC_CARD_INFO)), "Text displaced by image.");
        RECT before{},after{}; GetWindowRect(GetDlgItem(dialog,IDC_CARD_IMAGE),&before);
        const auto counts=picker.loader.counts();
        for(int i=0;i<100;++i) kingdom::preview(dialog,picker,picker.displayed);
        GetWindowRect(GetDlgItem(dialog,IDC_CARD_IMAGE),&after);
        check(EqualRect(&before,&after) && counts==picker.loader.counts(),"Repeated hover rebuilds the preview.");
        check(kingdom::resource_cache_bytes()<=96u*1024u*1024u,"Shared cache limit exceeded.");
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
        if (!picker_checked && failure.empty()) failure="Picker check did not run.";
        if (failure.empty()) {
            check(!kingdom::set_resource_package_for_test(std::filesystem::path(package).parent_path()/L"missing-test-resources.xpack"),"Missing resource fixture unexpectedly exists.");
            missing_art=true; picker_checked=false; picker_ticks=0;
            timer=SetTimer(nullptr,0,50,inspect_card_dialog);
            kingdom::pick_cards(nullptr,L"Text ohne Spielgrafiken",cards,{},kingdom::CardSelection::multiple,
                [&](const kingdom::CardTargets&,bool) { applied=true; return std::wstring{}; });
            KillTimer(nullptr,timer);
            if(!picker_checked && failure.empty()) failure="Missing-art check did not run.";
        }
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
