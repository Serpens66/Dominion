// Actual native dialog controls on a separate desktop; no foreground input.
#define KINGDOM_UI_TEST
#define UNICODE
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
bool ordered_checked=false;
bool missing_art=false;
bool online_checked = false;
bool online_cancel = false;
UINT_PTR timer = 0;
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

void save_dialog(HWND dialog,const wchar_t* name) {
    ShowWindow(dialog,SW_SHOWNOACTIVATE);
    const auto directory=std::filesystem::path(L"KingdomEffectsPatch/out/staging/ui-check");std::filesystem::create_directories(directory);
    RECT rect{};GetClientRect(dialog,&rect);const auto dc=GetDC(dialog),buffer=CreateCompatibleDC(dc);
    const auto bitmap=CreateCompatibleBitmap(dc,rect.right,rect.bottom);const auto old=SelectObject(buffer,bitmap);
    SendMessageW(dialog,WM_PRINT,reinterpret_cast<WPARAM>(buffer),PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND|PRF_CHECKVISIBLE);Gdiplus::Bitmap image(bitmap,nullptr);
    const CLSID png{0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0,0,0xf8,0x1e,0xf3,0x2e}};
    image.Save((directory/name).c_str(),&png,nullptr);SelectObject(buffer,old);DeleteObject(bitmap);DeleteDC(buffer);ReleaseDC(dialog,dc);
}
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
        check(SUCCEEDED(counted) && children>=89 && readable,"Accessible rows missing.");
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
        SetDlgItemTextW(dialog, IDC_QUERY, L"Alchemisten");
        check(SendMessageW(list,LB_GETCOUNT,0,0)>0,"Expansion reference search failed.");
        SetDlgItemTextW(dialog, IDC_QUERY, L"Ungetestet");
        const auto untested=std::count_if(std::begin(kingdom::effect_catalog),std::end(kingdom::effect_catalog),[](const auto& effect) {
            return kingdom::fold_card_search(effect.verification).find(L"ungetestet")!=std::wstring::npos;
        });
        check(SendMessageW(list, LB_GETCOUNT, 0, 0) == untested, "Untested effect labels missing.");
        SendMessageW(list,LB_SETSEL,TRUE,-1); SendMessageW(list,WM_COPY,0,0);
        const auto copied=kingdom::effect_table_detail::test_clipboard;
        check(copied.find(L"Effekt\tWirkung\tWann/wie\r\n")==0 && copied.find(L"Ungetestet")!=copied.npos,"Filtered table copy failed.");
        check(std::count(copied.begin(),copied.end(),L'\n')>=untested,"Copy did not include all filtered rows.");
        check(copied.find(L"twist_big_ideas")==copied.npos && copied.find(L'\u200B')==copied.npos,"Copy includes hidden rows or layout characters.");
        auto& table=*reinterpret_cast<kingdom::effect_table_detail::Model*>(GetWindowLongPtrW(list,GWLP_USERDATA));
        table.select(list,0,true);
        const auto existing_cell=table.controls[0][0];RECT before_cell{},after_cell{};GetWindowRect(existing_cell,&before_cell);
        const auto before_lines=SendMessageW(existing_cell,EM_GETLINECOUNT,0,0);
        const auto text_editor=table.open_cell(list,0,0);
        GetWindowRect(text_editor,&after_cell);check(text_editor==existing_cell && EqualRect(&before_cell,&after_cell) && before_lines==SendMessageW(text_editor,EM_GETLINECOUNT,0,0),"Click changes text layout."); check(text_editor!=nullptr,"Selectable cell unavailable.");
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
        auto& draft=*reinterpret_cast<kingdom::Draft*>(GetWindowLongPtrW(dialog,DWLP_USER));
        save_dialog(dialog,L"f8-de.png");
        const auto saved_effects=draft.effects;const auto* saved_selection=draft.selected;
        SendDlgItemMessageW(dialog,IDC_LANGUAGE,CB_SETCURSEL,1,0);SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(IDC_LANGUAGE,CBN_SELCHANGE),0);
        check(kingdom::ui::english() && draft.effects==saved_effects && draft.selected==saved_selection,"Language change lost draft or selection.");
        for(const auto& effect:kingdom::effect_catalog){check(*effect.title_en && *effect.description_en && *effect.timing_en && *effect.status_en && *effect.requirement_en,"Missing English effect data.");}
        check(kingdom::table_text(kingdom::effect_catalog[0])[1].find(L"Base game sufficient")!=std::wstring::npos,"English prerequisite missing.");
        for(const auto query:{L"Große Ideen",L"Big Ideas",L"twist_big_ideas"}){SetDlgItemTextW(dialog,IDC_QUERY,query);check(SendMessageW(list,LB_GETCOUNT,0,0)==1,"Bilingual search mismatch.");}
        SetDlgItemTextW(dialog,IDC_QUERY,L"");check_scroll_geometry(dialog);save_dialog(dialog,L"f8-en.png");
        SendDlgItemMessageW(dialog,IDC_LANGUAGE,CB_SETCURSEL,0,0);SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(IDC_LANGUAGE,CBN_SELCHANGE),0);
        draft.effects={"twist_big_ideas","twist_busy_busy","twist_big_ideas"};kingdom::update_active(dialog,draft,0);
        const auto active=GetDlgItem(dialog,IDC_ACTIVE);
        RECT row0{},row2{};SendMessageW(active,LB_GETITEMRECT,0,reinterpret_cast<LPARAM>(&row0));SendMessageW(active,LB_GETITEMRECT,2,reinterpret_cast<LPARAM>(&row2));
        DWORD_PTR reorder_data=0;check(GetWindowSubclass(active,kingdom::ListReorder::proc,9071,&reorder_data)!=FALSE,"Missing reorder subclass.");
        auto& reorder=*reinterpret_cast<kingdom::ListReorder*>(reorder_data);
        SendMessageW(active,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(10,row0.top+2));
        const int down_source=reorder.source;
        SendMessageW(active,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(10,row2.bottom-1));
        const int move_source=reorder.source,move_at=reorder.insertion;const bool was_dragging=reorder.dragging;
        SendMessageW(active,WM_LBUTTONUP,0,MAKELPARAM(10,row2.bottom-1));
        if(draft.effects[0]!="twist_busy_busy")std::fprintf(stderr,"Drag trace: row0 %ld/%ld row2 %ld/%ld down %d move %d insertion %d dragging %d\n",row0.top,row0.bottom,row2.top,row2.bottom,down_source,move_source,move_at,int(was_dragging));
        check(draft.effects[0]=="twist_busy_busy" && draft.effects[2]=="twist_big_ideas","Drag failed with identical effects.");
        const auto drag_saved=draft.effects;SendMessageW(active,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(10,2));SendMessageW(active,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(10,row2.bottom-1));SendMessageW(active,WM_KEYDOWN,VK_ESCAPE,0);SendMessageW(active,WM_LBUTTONUP,0,MAKELPARAM(10,row2.bottom-1));check(draft.effects==drag_saved,"Escape did not cancel drag.");
        SendMessageW(active,LB_SETCURSEL,0,0);SendMessageW(active,WM_SYSKEYDOWN,VK_DOWN,1L<<29);check(draft.effects[1]=="twist_busy_busy","Alt+Down failed.");
        SetFocus(active);SendMessageW(active,WM_KEYDOWN,VK_DELETE,0);check(draft.effects.size()==2,"Delete did not remove selected effect.");
        draft.effects=saved_effects;kingdom::update_active(dialog,draft);
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
        auto& picker=*reinterpret_cast<kingdom::Picker*>(GetWindowLongPtrW(dialog,DWLP_USER));
        if (!picker_ticks++) {
            const auto tree=GetDlgItem(dialog,IDC_CARD_TREE);
            for(auto group=TreeView_GetRoot(tree);group;group=TreeView_GetNextSibling(tree,group))
                check(!(TreeView_GetItemState(tree,group,TVIS_EXPANDED)&TVIS_EXPANDED),"An expansion opens automatically.");
            check(!picker.displayed && !picker.image && picker.loader.counts().second==0,"Initial card preview should be empty.");
            const auto group=TreeView_GetRoot(tree); TreeView_Expand(tree,group,TVE_EXPAND);
            check(!picker.expanded.empty(),"Manual expansion not recorded.");
            check(!picker.displayed,"Expanding a group changes preview.");
            const auto child=TreeView_GetChild(tree,group); RECT label{},bounds{};TreeView_GetItemRect(tree,child,&label,TRUE);GetClientRect(tree,&bounds);
            SendMessageW(tree,WM_MOUSEMOVE,0,MAKELPARAM(bounds.right-4,(label.top+label.bottom)/2));
            check(!picker.displayed,"Empty row space changes preview.");
            SetDlgItemTextW(dialog,IDC_CARD_QUERY,L"miser");SetDlgItemTextW(dialog,IDC_CARD_QUERY,L"");
            check(TreeView_GetItemState(tree,TreeView_GetRoot(tree),TVIS_EXPANDED)&TVIS_EXPANDED,"Search loses manual expansion state.");
            TreeView_Expand(tree,TreeView_GetRoot(tree),TVE_COLLAPSE);
            for (const auto query : {L"village", L"market", L"no-such-card", L"miser"}) SetDlgItemTextW(dialog, IDC_CARD_QUERY, query);
            const auto row=TreeView_GetChild(tree,TreeView_GetRoot(tree));TreeView_GetItemRect(tree,row,&label,TRUE);
            SendMessageW(tree,WM_MOUSEMOVE,0,MAKELPARAM(label.left+3,(label.top+label.bottom)/2));
            check(picker.displayed && std::string_view(picker.displayed->id)=="miser","Text hover did not select preview.");
        }
        // Text and image controls keep the same geometry before/after loading.
        check(IsWindowVisible(GetDlgItem(dialog, IDC_CARD_RULES)), "Rules hidden while loading.");
        if ((!missing_art && !picker.image || missing_art && picker.loader.counts().second==0) && picker_ticks<200) return FALSE;
        check(missing_art ? !picker.image : bool(picker.image),"Async image/fallback state incorrect.");
        check(IsWindowVisible(GetDlgItem(dialog, IDC_CARD_IMAGE)), "Image area hidden.");
        wchar_t name[128]{}; GetDlgItemTextW(dialog, IDC_CARD_IMAGE, name, 128);
        check(std::wstring_view(name)==kingdom::ui::pick(L"Geizhals",L"Miser"), "Stale preview published after navigation.");
        check(IsWindowVisible(GetDlgItem(dialog, IDC_CARD_TITLE)) && IsWindowVisible(GetDlgItem(dialog, IDC_CARD_INFO)), "Text displaced by image.");
        RECT before{},after{}; GetWindowRect(GetDlgItem(dialog,IDC_CARD_IMAGE),&before);
        if(picker.skin->logical_height()>=385) check(before.bottom-before.top>=(before.right-before.left)*1.5,"Card area is too short to use column width.");
        const auto counts=picker.loader.counts();
        for(int i=0;i<100;++i) kingdom::preview(dialog,picker,picker.displayed);
        GetWindowRect(GetDlgItem(dialog,IDC_CARD_IMAGE),&after);
        check(EqualRect(&before,&after) && counts==picker.loader.counts(),"Repeated hover rebuilds the preview.");
        RECT window{},client{};GetWindowRect(dialog,&window);
        SetWindowPos(dialog,nullptr,0,0,window.right-window.left,520,SWP_NOMOVE|SWP_NOZORDER);
        kingdom::layout_preview(dialog,picker);kingdom::layout_list(dialog,picker,kingdom::current_scope(dialog));
        GetClientRect(dialog,&client);
        for(const auto control:{IDOK,IDCANCEL,IDC_CARD_TREE,IDC_CARD_IMAGE}) {
            RECT rect{};GetWindowRect(GetDlgItem(dialog,control),&rect);MapWindowPoints(nullptr,dialog,reinterpret_cast<POINT*>(&rect),2);
            check(rect.top>=0 && rect.bottom<=client.bottom && rect.bottom>rect.top,"Limited-height layout clips controls.");
        }
        check(kingdom::resource_cache_bytes()<=96u*1024u*1024u,"Shared cache limit exceeded.");
        SetDlgItemTextW(dialog, IDC_CARD_QUERY, L"city_quarter");
        picker_checked=true;
    } catch (const std::exception& error) { failure=error.what(); }
    KillTimer(nullptr,timer); PostMessageW(dialog,WM_COMMAND,IDCANCEL,0); return FALSE;
}
BOOL CALLBACK inspect_ordered(HWND dialog,LPARAM) {
    if(!GetDlgItem(dialog,IDC_CARD_TREE))return TRUE;
    KillTimer(nullptr,timer);
    try {
        auto& picker=*reinterpret_cast<kingdom::Picker*>(GetWindowLongPtrW(dialog,DWLP_USER));const auto tree=GetDlgItem(dialog,IDC_CARD_TREE);
        check(picker.mode==kingdom::CardSelection::ordered && !(GetWindowLongPtrW(tree,GWL_STYLE)&TVS_CHECKBOXES),"Typhoon has unwanted checkboxes.");
        check(!IsWindowEnabled(GetDlgItem(dialog,IDOK)),"Empty Typhoon can be accepted.");
        const auto group=TreeView_GetRoot(tree);RECT label{};TreeView_GetItemRect(tree,group,&label,TRUE);
        SendMessageW(tree,WM_MOUSEMOVE,0,MAKELPARAM(label.left+3,(label.top+label.bottom)/2));
        check(picker.expansion && !picker.displayed && !IsWindowVisible(GetDlgItem(dialog,IDC_CARD_IMAGE)),"Expansion preview missing.");
        SendMessageW(dialog,WM_COMMAND,IDC_CARD_TEXT,0);check(picker.expansion_rules,"Expansion rules not reachable.");
        save_dialog(dialog,kingdom::ui::english()?L"expansion-en.png":L"expansion-de.png");
        TreeView_SelectItem(tree,group);check(picker.expansion,"Keyboard expansion preview missing.");
        // Hover cannot choose the Add button's target.
        TreeView_Expand(tree,group,TVE_EXPAND);const auto first=TreeView_GetChild(tree,group),second=TreeView_GetNextSibling(tree,first);
        TreeView_SelectItem(tree,first);const auto* chosen=kingdom::card_at(tree,first);kingdom::preview(dialog,picker,kingdom::card_at(tree,second));
        SendMessageW(dialog,WM_COMMAND,IDC_CARD_ADD,0);check(picker.selected.size()==1 && picker.selected.front()==chosen,"Hover replaced the chosen target.");
        SendMessageW(dialog,WM_COMMAND,IDC_CARD_ADD,0);check(picker.selected.size()==1,"Duplicate Typhoon pile accepted.");
        TreeView_SelectItem(tree,second);SendMessageW(tree,WM_KEYDOWN,VK_SPACE,0);check(picker.selected.size()==2,"Space does not add selected pile.");
        SetDlgItemTextW(dialog,IDC_CARD_QUERY,L"");
        for(const auto& entry:picker.visible){if(picker.has(entry.second))continue;TreeView_SelectItem(tree,entry.first);kingdom::add_selected_pile(dialog,picker);if(picker.selected.size()==20)break;}
        check(picker.selected.size()==20 && !IsWindowEnabled(GetDlgItem(dialog,IDC_CARD_ADD)),"Typhoon capacity or Add state incorrect.");
        const auto saved=picker.selected;for(const auto& entry:picker.visible)if(!picker.has(entry.second)){TreeView_SelectItem(tree,entry.first);kingdom::add_selected_pile(dialog,picker);break;}
        check(picker.selected==saved,"21st Typhoon pile changed draft.");
        const auto list=GetDlgItem(dialog,IDC_CARD_SELECTION);SendMessageW(list,LB_SETTOPINDEX,0,0);RECT r0{},r2{};
        SendMessageW(list,LB_GETITEMRECT,0,reinterpret_cast<LPARAM>(&r0));SendMessageW(list,LB_GETITEMRECT,1,reinterpret_cast<LPARAM>(&r2));
        SendMessageW(list,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(8,r0.top+2));SendMessageW(list,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(8,r2.bottom-1));SendMessageW(list,WM_LBUTTONUP,0,MAKELPARAM(8,r2.bottom-1));
        check(picker.selected[1]==saved[0] && picker.selected[0]==saved[1],"Typhoon drag reorder failed.");
        SendMessageW(list,LB_SETCURSEL,1,0);SendMessageW(list,WM_SYSKEYDOWN,VK_UP,1L<<29);check(picker.selected==saved,"Typhoon Alt+Up failed.");
        RECT client{};GetClientRect(list,&client);
        const auto outside_before=picker.selected;
        SendMessageW(list,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(8,r0.top+2));SendMessageW(list,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(8,client.bottom+10));SendMessageW(list,WM_LBUTTONUP,0,MAKELPARAM(8,client.bottom+10));
        check(picker.selected==outside_before,"Drop outside changed order.");
        SendMessageW(list,LB_SETTOPINDEX,0,0);SendMessageW(list,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(8,2));SendMessageW(list,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(8,client.bottom-2));
        for(int i=0;i<4;++i)SendMessageW(list,WM_TIMER,9071,0);
        check(SendMessageW(list,LB_GETTOPINDEX,0,0)>0,"Drag edge did not scroll.");
        check(SendMessageW(list,WM_GETDLGCODE,VK_ESCAPE,0)&DLGC_WANTALLKEYS,"Dialog would consume drag Escape.");
        SendMessageW(list,WM_KEYDOWN,VK_ESCAPE,0);check(picker.selected==outside_before,"Edge-scroll Escape changed order.");
        SendMessageW(list,LB_SETCURSEL,0,0);SetFocus(tree);SendMessageW(list,WM_KEYDOWN,VK_DELETE,0);check(picker.selected.size()==20,"Unfocused list consumed Delete.");
        SetFocus(list);SendMessageW(list,WM_KEYDOWN,VK_DELETE,0);check(picker.selected.size()==19,"Typhoon Delete failed.");
        ordered_checked=true;
    }catch(const std::exception& error){failure=error.what();}
    PostMessageW(dialog,WM_COMMAND,IDCANCEL,0);return FALSE;
}
void CALLBACK inspect_ordered_dialog(HWND,UINT,UINT_PTR,DWORD){EnumThreadWindows(GetCurrentThreadId(),inspect_ordered,0);}
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
        if(failure.empty()) for(WORD points:{WORD(11),WORD(14)}) {
            const auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(IDD_CARD_PICKER),RT_DIALOG);
            const auto size=SizeofResource(nullptr,resource); const auto data=LockResource(LoadResource(nullptr,resource));
            std::vector<WORD> layout((size+1)/2);std::memcpy(layout.data(),data,size);
            std::size_t at=13;
            for(int field=0;field<3;++field) {if(layout[at]==0xffff)at+=2;else {while(layout[at])++at;++at;}}
            layout[at]=points;
            picker_checked=false;picker_ticks=0;timer=SetTimer(nullptr,0,50,inspect_card_dialog);
            // The callback must outlive the modal picker (Picker stores a reference).
            const kingdom::AcceptCards accept=[](const kingdom::CardTargets&,bool){return std::wstring{};};
            kingdom::Picker stable{L"Skalierungsprüfung",cards,{},kingdom::CardSelection::multiple,accept};
            DialogBoxIndirectParamW(GetModuleHandleW(nullptr),reinterpret_cast<const DLGTEMPLATE*>(layout.data()),nullptr,kingdom::picker_proc,reinterpret_cast<LPARAM>(&stable));
            KillTimer(nullptr,timer);if(!picker_checked && failure.empty())failure="Scaled picker check did not run.";
        }
        if(failure.empty()) for(const auto language:{kingdom::ui::Language::de,kingdom::ui::Language::en}){
            kingdom::ui::selected=language;ordered_checked=false;timer=SetTimer(nullptr,0,100,inspect_ordered_dialog);
            kingdom::pick_cards(nullptr,kingdom::ui::pick(L"Taifun",L"Typhoon"),cards,{},kingdom::CardSelection::ordered,[](const kingdom::CardTargets&,bool){return std::wstring{};});
            KillTimer(nullptr,timer);if(!ordered_checked && failure.empty())failure="Ordered picker test did not run.";
        }
        kingdom::ui::selected=kingdom::ui::Language::de;
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
    if(failure.empty()){
        const auto directory=std::filesystem::path(L"KingdomEffectsPatch/out/staging/ui-check");std::filesystem::create_directories(directory);
        kingdom::ui::test_preferences=directory/L"editor.ini";
        check(kingdom::ui::set_language(kingdom::ui::Language::en),"Language preference save failed.");kingdom::ui::selected=kingdom::ui::Language::de;kingdom::ui::load_language();check(kingdom::ui::english(),"Language preference did not survive reload.");
        std::filesystem::remove(kingdom::ui::test_preferences);kingdom::ui::test_preferences.clear();kingdom::ui::selected=kingdom::ui::Language::de;
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
