#include "ui_language.hpp"
#pragma once
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <oleacc.h>
#pragma comment(lib, "oleacc.lib")
#include <algorithm>
#include <array>
#include <cstring>
#include <new>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace kingdom {
// A pixel-scrolled owner-drawn table. The editor's private LB_* interface and
// selection notifications stay intact; rows are not limited to 255 pixels.
constexpr UINT effect_table_layout = WM_APP + 83;
constexpr UINT effect_table_cells = WM_APP + 84;
constexpr UINT effect_table_padding = WM_APP + 85;
constexpr UINT effect_table_close_text = WM_APP + 86;
constexpr UINT effect_table_select_cell = WM_APP + 87;
constexpr UINT effect_table_replace_rows = WM_APP + 88;
namespace effect_table_detail {
#ifdef EFFECT_TABLE_PERF_COUNTERS
struct PerformanceCounters { unsigned created=0,destroyed=0,measurements=0,measurement_windows=0; };
inline PerformanceCounters performance;
#endif
inline std::wstring plain_copy(std::wstring text) {
    text.erase(std::remove(text.begin(),text.end(),L'\u200B'),text.end()); return text;
}
#ifdef EFFECT_TABLE_TEST_CLIPBOARD
inline std::wstring test_clipboard;
#endif
inline bool copy_text(HWND owner,std::wstring text) {
    text=plain_copy(std::move(text));
#ifdef EFFECT_TABLE_TEST_CLIPBOARD
    (void)owner; test_clipboard=std::move(text); return true;
#else
    const auto bytes=(text.size()+1)*sizeof(wchar_t);
    const auto memory=GlobalAlloc(GMEM_MOVEABLE,bytes); if (!memory) return false;
    auto* destination=GlobalLock(memory); if (!destination) { GlobalFree(memory); return false; }
    std::memcpy(destination,text.c_str(),bytes); GlobalUnlock(memory);
    if (!OpenClipboard(owner)) { GlobalFree(memory); return false; }
    const bool copied=EmptyClipboard() && SetClipboardData(CF_UNICODETEXT,memory)!=nullptr;
    CloseClipboard(); if (!copied) GlobalFree(memory); return copied;
#endif
}
inline std::wstring selected_text(HWND editor) {
    DWORD first=0,last=0; SendMessageW(editor,EM_GETSEL,reinterpret_cast<WPARAM>(&first),reinterpret_cast<LPARAM>(&last));
    const int length=GetWindowTextLengthW(editor); std::wstring text(std::size_t(length)+1,L'\0');
    GetWindowTextW(editor,text.data(),length+1); text.resize(length);
    return text.substr((std::min)(std::size_t(first),text.size()),(std::min)(std::size_t(last),text.size())-(std::min)(std::size_t(first),text.size()));
}
inline LRESULT CALLBACK text_proc(HWND editor,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data) {
    const auto table=reinterpret_cast<HWND>(data);
    if(message==WM_GETDLGCODE && wparam==VK_ESCAPE) return DefSubclassProc(editor,message,wparam,lparam)|DLGC_WANTMESSAGE;
    // The key handler already copies sanitized text; do not let EDIT repeat
    // Ctrl+A/C through WM_CHAR using its unsanitized default clipboard path.
    if (message==WM_CHAR && (wparam==1 || wparam==3)) return 0;
    if (message==WM_COPY || (message==WM_KEYDOWN && ((wparam=='C' && (GetKeyState(VK_CONTROL)&0x8000)) || (wparam==VK_INSERT && (GetKeyState(VK_CONTROL)&0x8000))))) {
        auto text=selected_text(editor); if (!text.empty()) copy_text(table,std::move(text)); return 0;
    }
    if (message==WM_KEYDOWN && wparam=='A' && (GetKeyState(VK_CONTROL)&0x8000)) { SendMessageW(editor,EM_SETSEL,0,-1); return 0; }
    if (message==WM_KEYDOWN && wparam==VK_ESCAPE) { SendMessageW(table,effect_table_close_text,reinterpret_cast<WPARAM>(editor),0); return 0; }
    if (message==WM_MOUSEWHEEL || message==WM_CONTEXTMENU) return SendMessageW(table,message,message==WM_CONTEXTMENU?reinterpret_cast<WPARAM>(editor):wparam,lparam);
    if(message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN || message==WM_SETFOCUS) SendMessageW(table,effect_table_select_cell,reinterpret_cast<WPARAM>(editor),0);
    if (message==WM_NCDESTROY) RemoveWindowSubclass(editor,text_proc,id);
    return DefSubclassProc(editor,message,wparam,lparam);
}
struct Row { std::wstring label; LPARAM data=0; int height=24; std::array<std::wstring,3> cells{}; };
struct Model {
    std::vector<Row> rows;
    std::vector<int> offsets{0};
    HFONT font=nullptr;
    int selected=-1, position=0, wheel=0;
    bool redraw=true;
    bool all_marked=false;
    HWND editor=nullptr;
    std::vector<std::array<HWND,3>> controls;
    struct StoredCells { std::array<HWND,3> windows{}; std::array<std::wstring,3> text{}; };
    std::map<LPARAM,StoredCells> retained;
    int padding_x=7,padding_y=5;
    std::wstring search;
    ULONGLONG typed_at=0;
    ~Model() { destroy_cells(); }
    void close_editor() { if(editor && IsWindow(editor))SendMessageW(editor,EM_SETSEL,WPARAM(-1),0);editor=nullptr; }
    void destroy_cells(){
        editor=nullptr;
        for(const auto& entry:retained)for(auto cell:entry.second.windows)if(IsWindow(cell)){
            DestroyWindow(cell);
#ifdef EFFECT_TABLE_PERF_COUNTERS
            ++performance.destroyed;
#endif
        }
        retained.clear();controls.clear();
    }
    bool owns(HWND cell) const {for(const auto& row:controls)for(auto value:row)if(value && value==cell)return true;return false;}
    void replace_rows(HWND table,const std::vector<Row>& next){
        close_editor();
        for(const auto& row:controls)for(auto cell:row)if(cell && (GetWindowLongPtrW(cell,GWL_STYLE)&WS_VISIBLE))ShowWindow(cell,SW_HIDE);
        rows=next;controls.assign(rows.size(),{});offsets.assign(1,0);
        position=0;selected=-1;all_marked=false;wheel=0;search.clear();
        if(redraw)rebuild(table);
    }
    void layout_cells(HWND table){
        if(!redraw)return;
        if(controls.size()!=rows.size())controls.resize(rows.size());
        RECT client{};GetClientRect(table,&client);const int boundaries[]{0,client.right*30/100,client.right*73/100,client.right};
        for(std::size_t row=0;row<rows.size();++row){
            const bool visible=offsets[row]<position+client.bottom && offsets[row+1]>position;
            if(!visible){for(auto cell:controls[row])if(cell && (GetWindowLongPtrW(cell,GWL_STYLE)&WS_VISIBLE))ShowWindow(cell,SW_HIDE);continue;}
            // Production rows carry their stable EffectDefinition pointer. The
            // negative fallback keeps the legacy LB_* interface usable too.
            auto& row_cells=retained[rows[row].data?rows[row].data:-LPARAM(row+1)];
            for(int column=0;column<3;++column){
                auto& cell=row_cells.windows[column];
                if(!cell){
                    cell=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,
                        0,0,1,1,table,nullptr,reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(table,GWLP_HINSTANCE)),nullptr);
                    if(!cell)continue;
#ifdef EFFECT_TABLE_PERF_COUNTERS
                    ++performance.created;
#endif
                    SetWindowSubclass(cell,text_proc,1,reinterpret_cast<DWORD_PTR>(table));
                    SendMessageW(cell,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,0);
                }
                controls[row][column]=cell;
                if(row_cells.text[column]!=rows[row].cells[column]){
                    row_cells.text[column]=rows[row].cells[column];SetWindowTextW(cell,row_cells.text[column].c_str());
                }
                if(reinterpret_cast<HFONT>(SendMessageW(cell,WM_GETFONT,0,0))!=font)SendMessageW(cell,WM_SETFONT,reinterpret_cast<WPARAM>(font),FALSE);
                const int width=boundaries[column+1]-boundaries[column]-1,height=rows[row].height-1;
                SetWindowPos(cell,nullptr,boundaries[column],offsets[row]-position,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
                RECT area{padding_x,padding_y,width-padding_x,height-padding_y};SendMessageW(cell,EM_SETRECTNP,0,reinterpret_cast<LPARAM>(&area));
                if(!(GetWindowLongPtrW(cell,GWL_STYLE)&WS_VISIBLE))ShowWindow(cell,SW_SHOWNA);
                InvalidateRect(cell,nullptr,FALSE);
            }
        }
    }
    std::wstring exported(bool all) const {
        std::wstring text=all?kingdom::ui::tr(L"Effekt\tWirkung\tWann/wie\r\n"):L"";
        for (int row=0;row<int(rows.size());++row) {
            if (!all && row!=selected) continue;
            if (!text.empty() && text.back()!=L'\n') text+=L"\r\n";
            for (std::size_t column=0;column<3;++column) {
                if (column) text+=L'\t';
                auto value=rows[row].cells[column];
                text+=value;
            }
        }
        return plain_copy(std::move(text));
    }
    HWND open_cell(HWND table,int row,int column) {
        if(row<0 || row>=int(rows.size()) || column<0 || column>=3)return nullptr;
        if(row+1>=int(offsets.size()))return nullptr;
        if(offsets[row]<position || offsets[row]>=position+page(table))scroll(table,offsets[row]);
        layout_cells(table);
        editor=controls[row][column];if(editor)SetFocus(editor);return editor;
    }
    int page(HWND window) const { RECT r{}; GetClientRect(window,&r); return (std::max)(1,int(r.bottom)); }
    int limit(HWND window) const { return (std::max)(0,offsets.back()-page(window)); }
    void scroll(HWND window,int value) {
        close_editor();
        position=std::clamp(value,0,limit(window));
        SCROLLINFO info{sizeof info,SIF_POS}; info.nPos=position;
        SetScrollInfo(window,SB_VERT,&info,TRUE);layout_cells(window); if (redraw) InvalidateRect(window,nullptr,FALSE);
    }
    void rebuild(HWND window) {
        close_editor();
        offsets.assign(1,0);
        for (const auto& row:rows) offsets.push_back(offsets.back()+row.height);
        position=std::clamp(position,0,limit(window));
        SCROLLINFO info{sizeof info,SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL};
        info.nMin=0; info.nMax=(std::max)(0,offsets.back()-1); info.nPage=UINT(page(window)); info.nPos=position;
        SetScrollInfo(window,SB_VERT,&info,TRUE);layout_cells(window);
        if (redraw) InvalidateRect(window,nullptr,FALSE);
    }
    int at(int pixel) const {
        if (rows.empty() || pixel<0 || pixel>=offsets.back()) return -1;
        return int(std::upper_bound(offsets.begin(),offsets.end(),pixel)-offsets.begin())-1;
    }
    void select(HWND window,int row,bool notify,bool ensure_visible=true) {
        if (row<0 || row>=int(rows.size())) row=-1;
        const bool changed=selected!=row; selected=row; all_marked=false;
        if (ensure_visible && row>=0 && row+1<int(offsets.size())) {
            if (offsets[row]<position) scroll(window,offsets[row]);
            else if (offsets[row+1]>position+page(window))
                scroll(window,rows[row].height>page(window)?offsets[row]:offsets[row+1]-page(window));
        }
        if(redraw){InvalidateRect(window,nullptr,FALSE);for(const auto& cells:controls)for(auto cell:cells)if(cell)InvalidateRect(cell,nullptr,FALSE);}
        if (notify && changed) {
            SendMessageW(GetParent(window),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(window),LBN_SELCHANGE),reinterpret_cast<LPARAM>(window));
            NotifyWinEvent(EVENT_OBJECT_SELECTION,window,OBJID_CLIENT,row+1);
        }
    }
};
inline LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    if (message==WM_NCCREATE) {
        auto* model=new(std::nothrow) Model;
        if (!model) return FALSE;
        SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(model)); return TRUE;
    }
    auto* model=reinterpret_cast<Model*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if (!model) return DefWindowProcW(window,message,wparam,lparam);
    auto& state=*model;
    const auto valid=[&] { return wparam<state.rows.size(); };
    switch (message) {
        case effect_table_replace_rows:
            if(!lparam)return LB_ERR;state.replace_rows(window,*reinterpret_cast<const std::vector<Row>*>(lparam));return 0;
        case effect_table_cells:
            if (!valid() || !lparam) return LB_ERR;
            state.rows[wparam].cells=*reinterpret_cast<const std::array<std::wstring,3>*>(lparam); return 0;
        case effect_table_padding: state.padding_x=int(wparam); state.padding_y=int(lparam); return 0;
        case effect_table_select_cell:
            state.editor=reinterpret_cast<HWND>(wparam);
            for(int row=0;row<int(state.controls.size());++row)for(auto cell:state.controls[row])if(cell==state.editor){state.select(window,row,true,false);return 0;}
            return 0;
        case effect_table_close_text:
            if (state.editor==reinterpret_cast<HWND>(wparam)){state.close_editor();SetFocus(window);} return 0;
        case WM_CTLCOLOREDIT: case WM_CTLCOLORSTATIC:
            if (state.owns(reinterpret_cast<HWND>(lparam))) {
                bool selected=state.all_marked;
                if(state.selected>=0 && state.selected<int(state.controls.size()))for(auto cell:state.controls[state.selected])selected|=cell==reinterpret_cast<HWND>(lparam);
                const auto dc=reinterpret_cast<HDC>(wparam); const auto color=selected?RGB(231,205,148):RGB(248,238,212);
                SetTextColor(dc,RGB(56,35,20)); SetBkColor(dc,color); SetDCBrushColor(dc,color);
                return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
            }
            break;
        case WM_COPY: {
            if (state.rows.empty()) return 0;
            auto text=state.exported(state.all_marked); if (!text.empty()) copy_text(window,std::move(text)); return 0;
        }
        case WM_CONTEXTMENU: {
            POINT point{GET_X_LPARAM(lparam),GET_Y_LPARAM(lparam)};
            const bool text_source=state.editor && reinterpret_cast<HWND>(wparam)==state.editor;
            if (point.x==-1 && point.y==-1) { point={0,0}; ClientToScreen(window,&point); }
            else if (!text_source) {
                auto hit=point; ScreenToClient(window,&hit); state.close_editor();
                const auto row=state.at(hit.y+state.position); if (row>=0) state.select(window,row,true,false);
            }
            const auto menu=CreatePopupMenu(); if (!menu) return 0;
            const bool has_text=text_source && !selected_text(state.editor).empty();
            AppendMenuW(menu,MF_STRING|(has_text?0:MF_GRAYED),1,kingdom::ui::tr(L"Markierten Text kopieren\tStrg+C"));
            AppendMenuW(menu,MF_STRING|(state.selected>=0?0:MF_GRAYED),2,kingdom::ui::tr(L"Effektzeile kopieren"));
            AppendMenuW(menu,MF_STRING|(!state.rows.empty()?0:MF_GRAYED),3,kingdom::ui::tr(L"Alle angezeigten Effekte kopieren"));
            const auto command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,window,nullptr);
            DestroyMenu(menu);
            if (command==1 && state.editor) SendMessageW(state.editor,WM_COPY,0,0);
            else if (command==2 || command==3) copy_text(window,state.exported(command==3));
            return 0;
        }
        case WM_GETOBJECT: {
            if (lparam!=OBJID_CLIENT) break;
            IAccessible* accessible=nullptr;
            if (FAILED(CreateStdAccessibleProxyW(window,L"LISTBOX",OBJID_CLIENT,__uuidof(IAccessible),reinterpret_cast<void**>(&accessible)))) return 0;
            const auto result=LresultFromObject(__uuidof(IAccessible),wparam,accessible); accessible->Release(); return result;
        }
        case WM_NCDESTROY: SetWindowLongPtrW(window,GWLP_USERDATA,0); delete model; return DefWindowProcW(window,message,wparam,lparam);
        case WM_SETREDRAW:
            state.redraw=wparam!=0;
            if (state.redraw) { state.rebuild(window); NotifyWinEvent(EVENT_OBJECT_REORDER,window,OBJID_CLIENT,CHILDID_SELF); }
            return 0;
        case WM_GETFONT: return reinterpret_cast<LRESULT>(state.font);
        case WM_SETFONT:
            if(state.font==reinterpret_cast<HFONT>(wparam))return 0;
            state.font=reinterpret_cast<HFONT>(wparam);
            for(const auto& entry:state.retained)for(auto cell:entry.second.windows)if(cell)SendMessageW(cell,WM_SETFONT,wparam,FALSE);
            SendMessageW(GetParent(window),effect_table_layout,1,0);
            return 0;
        case WM_SIZE:
            if (!state.rows.empty()) SendMessageW(GetParent(window),effect_table_layout,0,0);
            else state.rebuild(window);
            return 0;
        case LB_RESETCONTENT: state.destroy_cells(); state.all_marked=false; state.rows.clear(); state.offsets.assign(1,0); state.position=0; state.selected=-1;
            if (state.redraw) state.rebuild(window); return 0;
        case LB_ADDSTRING: {
            Row row; row.label=reinterpret_cast<const wchar_t*>(lparam); row.cells[0]=row.label; state.rows.push_back(std::move(row));
            if (state.redraw) state.rebuild(window); return LRESULT(state.rows.size()-1);
        }
        case LB_GETCOUNT: return LRESULT(state.rows.size());
        case LB_SETITEMDATA: if (!valid()) return LB_ERR; state.rows[wparam].data=lparam; return 0;
        case LB_GETITEMDATA: return valid()?state.rows[wparam].data:LB_ERR;
        case LB_SETITEMHEIGHT: if (!valid() || lparam<=0 || lparam>100000) return LB_ERR;
            state.rows[wparam].height=int(lparam); if (state.redraw) state.rebuild(window); return 0;
        case LB_GETITEMHEIGHT: return valid()?state.rows[wparam].height:LB_ERR;
        case LB_SETCURSEL: state.select(window,int(wparam),false); return state.selected;
        case LB_GETCURSEL: return state.selected<0?LB_ERR:state.selected;
        case LB_GETSEL: return valid()?(state.all_marked || int(wparam)==state.selected):LB_ERR;
        case LB_GETSELCOUNT: return state.all_marked?LRESULT(state.rows.size()):state.selected>=0?1:0;
        case LB_GETSELITEMS:
            if (!wparam || !lparam) return 0;
            if (state.all_marked) { const auto count=(std::min)(std::size_t(wparam),state.rows.size()); for (std::size_t i=0;i<count;++i) reinterpret_cast<int*>(lparam)[i]=int(i); return LRESULT(count); }
            if (state.selected<0) return 0; *reinterpret_cast<int*>(lparam)=state.selected; return 1;
        case LB_SETSEL:
            if (lparam==-1) { state.close_editor(); state.all_marked=wparam!=0; InvalidateRect(window,nullptr,FALSE); }
            else if (wparam) state.select(window,int(lparam),true);
            return 0;
        case LB_GETCARETINDEX: return (std::max)(0,state.selected);
        case LB_SETCARETINDEX: state.select(window,int(wparam),false); return 0;
        case LB_ITEMFROMPOINT: {
            RECT rect{}; GetClientRect(window,&rect); POINT point{GET_X_LPARAM(lparam),GET_Y_LPARAM(lparam)};
            const auto row=state.at(point.y+state.position);
            return MAKELRESULT((std::max)(0,row),!PtInRect(&rect,point) || row<0);
        }
        case LB_GETTEXTLEN: return valid()?LRESULT(state.rows[wparam].label.size()):LB_ERR;
        case LB_GETTEXT:
            if (!valid() || !lparam) return LB_ERR;
            std::copy(state.rows[wparam].label.begin(),state.rows[wparam].label.end(),reinterpret_cast<wchar_t*>(lparam));
            reinterpret_cast<wchar_t*>(lparam)[state.rows[wparam].label.size()]=0; return LRESULT(state.rows[wparam].label.size());
        case LB_GETTOPINDEX: return (std::max)(0,state.at(state.position));
        case LB_SETTOPINDEX: if (!valid()) return LB_ERR; state.scroll(window,state.offsets[wparam]); return 0;
        case LB_GETITEMRECT: {
            if (!valid() || !lparam || wparam+1>=state.offsets.size()) return LB_ERR;
            RECT rect{}; GetClientRect(window,&rect); rect.top=state.offsets[wparam]-state.position; rect.bottom=rect.top+state.rows[wparam].height;
            *reinterpret_cast<RECT*>(lparam)=rect; return 0;
        }
        case WM_GETDLGCODE: return DLGC_WANTARROWS|DLGC_WANTCHARS;
        case WM_CHAR: {
            if (GetKeyState(VK_CONTROL)&0x8000) return 0;
            if (wparam<32 || state.rows.empty()) return 0;
            const auto now=GetTickCount64();
            if (now-state.typed_at>1000 || (state.search.size()==1 && state.search[0]==wchar_t(wparam))) state.search.clear();
            state.typed_at=now; state.search+=wchar_t(wparam);
            const auto start=state.search.size()==1?state.selected+1:(std::max)(0,state.selected);
            for (int step=0;step<int(state.rows.size());++step) {
                const auto row=(start+step)%int(state.rows.size()); const auto& label=state.rows[row].label;
                if (label.size()>=state.search.size() && CompareStringOrdinal(label.data(),int(state.search.size()),state.search.data(),int(state.search.size()),TRUE)==CSTR_EQUAL) {
                    state.select(window,row,true); break;
                }
            }
            return 0;
        }
        case WM_SETFOCUS: case WM_KILLFOCUS: InvalidateRect(window,nullptr,FALSE); return 0;
        case WM_LBUTTONDOWN: {
            SetFocus(window); const auto row=state.at(GET_Y_LPARAM(lparam)+state.position);
            state.close_editor();
            if (row>=0) {
                state.select(window,row,true,false);
                RECT client{}; GetClientRect(window,&client); const int x=GET_X_LPARAM(lparam);
                const int column=x<client.right*30/100?0:x<client.right*73/100?1:2;
                if (const auto editor=state.open_cell(window,row,column)) {
                    POINT click{x,GET_Y_LPARAM(lparam)}; MapWindowPoints(window,editor,&click,1);
                    SendMessageW(editor,WM_LBUTTONDOWN,wparam,MAKELPARAM(click.x,click.y));
                }
            }
            return 0;
        }
        case WM_KEYDOWN: {
            if ((GetKeyState(VK_CONTROL)&0x8000) && (wparam=='C' || wparam==VK_INSERT)) return SendMessageW(window,WM_COPY,0,0);
            if ((GetKeyState(VK_CONTROL)&0x8000) && wparam=='A') {
                return SendMessageW(window,LB_SETSEL,TRUE,-1);
            }
            if (state.rows.empty()) return 0;
            int row=state.selected<0?0:state.selected;
            if (wparam==VK_HOME) row=0;
            else if (wparam==VK_END) row=int(state.rows.size()-1);
            else if (wparam==VK_UP) row=(std::max)(0,row-1);
            else if (wparam==VK_DOWN) row=(std::min)(int(state.rows.size()-1),row+1);
            else if (wparam==VK_PRIOR || wparam==VK_NEXT) {
                const auto step=wparam==VK_NEXT?state.page(window):-state.page(window);
                state.scroll(window,state.position+step);
                row=(std::max)(0,state.at((std::min)(state.offsets.back()-1,state.position)));
                // Preserve pixel movement within a row taller than the viewport.
                const bool changed=state.selected!=row; state.selected=row;
                InvalidateRect(window,nullptr,FALSE);
                if (changed) SendMessageW(GetParent(window),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(window),LBN_SELCHANGE),reinterpret_cast<LPARAM>(window));
                return 0;
            } else return DefWindowProcW(window,message,wparam,lparam);
            state.select(window,row,true); return 0;
        }
        case WM_MOUSEWHEEL: {
            UINT lines=3; SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0,&lines,0);
            state.wheel+=GET_WHEEL_DELTA_WPARAM(wparam); const auto steps=state.wheel/WHEEL_DELTA; state.wheel%=WHEEL_DELTA;
            state.scroll(window,state.position-steps*(lines==WHEEL_PAGESCROLL?state.page(window):int(lines)*20)); return 0;
        }
        case WM_VSCROLL: {
            SCROLLINFO info{sizeof info,SIF_TRACKPOS}; GetScrollInfo(window,SB_VERT,&info);
            int value=state.position;
            switch (LOWORD(wparam)) {
                case SB_TOP: value=0; break; case SB_BOTTOM: value=state.limit(window); break;
                case SB_LINEUP: value-=20; break; case SB_LINEDOWN: value+=20; break;
                case SB_PAGEUP: value-=state.page(window); break; case SB_PAGEDOWN: value+=state.page(window); break;
                case SB_THUMBTRACK: case SB_THUMBPOSITION: value=info.nTrackPos; break;
                default: return 0;
            }
            state.scroll(window,value); return 0;
        }
        case WM_ERASEBKGND: return 1;
        case WM_PRINTCLIENT: case WM_PAINT: {
            if (!state.redraw) { ValidateRect(window,nullptr); return 0; }
            PAINTSTRUCT paint{}; const auto dc=message==WM_PAINT?BeginPaint(window,&paint):reinterpret_cast<HDC>(wparam);
            RECT client{}; GetClientRect(window,&client); SetDCBrushColor(dc,RGB(248,238,212));
            FillRect(dc,&client,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
            for (int row=(std::max)(0,state.at(state.position)); row<int(state.rows.size());++row) {
                const int top=state.offsets[row]-state.position; if (top>=client.bottom) break;
                DRAWITEMSTRUCT item{}; item.CtlType=ODT_LISTBOX; item.CtlID=UINT(GetDlgCtrlID(window)); item.itemID=UINT(row);
                item.itemAction=ODA_DRAWENTIRE; item.hwndItem=window; item.hDC=dc;
                item.itemState=(state.all_marked || state.selected==row?ODS_SELECTED:0)|((state.selected==row && GetFocus()==window)?ODS_FOCUS:0);
                item.rcItem={0,top,client.right,top+state.rows[row].height}; item.itemData=ULONG_PTR(state.rows[row].data);
                SendMessageW(GetParent(window),WM_DRAWITEM,item.CtlID,reinterpret_cast<LPARAM>(&item));
            }
            if (message==WM_PAINT) EndPaint(window,&paint); return 0;
        }
    }
    return DefWindowProcW(window,message,wparam,lparam);
}
}
// One hidden EDIT uses exactly the visible cells' text layout. Reuse it for
// every uncached height instead of allocating a window for each measurement.
class EffectCellMeasurer {
    HWND edit_=nullptr;
    HFONT font_=nullptr;
    int line_height_=0;
public:
    explicit EffectCellMeasurer(HWND parent){
        edit_=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,0,0,1,10000,parent,nullptr,
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent,GWLP_HINSTANCE)),nullptr);
        if(!edit_)throw std::runtime_error("Cannot create effect measurement field.");
        SendMessageW(edit_,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,0);
#ifdef EFFECT_TABLE_PERF_COUNTERS
        ++effect_table_detail::performance.measurement_windows;
#endif
    }
    ~EffectCellMeasurer(){if(IsWindow(edit_))DestroyWindow(edit_);}
    EffectCellMeasurer(const EffectCellMeasurer&)=delete;
    EffectCellMeasurer& operator=(const EffectCellMeasurer&)=delete;
    int measure(HFONT font,const std::wstring& text,int width,int padding_x,int padding_y){
#ifdef EFFECT_TABLE_PERF_COUNTERS
        ++effect_table_detail::performance.measurements;
#endif
        if(font_!=font || !line_height_){
            font_=font;SendMessageW(edit_,WM_SETFONT,reinterpret_cast<WPARAM>(font),FALSE);
            const auto dc=GetDC(edit_);const auto previous=SelectObject(dc,font);TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);
            line_height_=metrics.tmHeight;SelectObject(dc,previous);ReleaseDC(edit_,dc);
        }
        SetWindowPos(edit_,nullptr,0,0,width,10000,SWP_NOZORDER|SWP_NOACTIVATE);
        RECT area{padding_x,padding_y,width-padding_x,9999-padding_y};SendMessageW(edit_,EM_SETRECTNP,0,reinterpret_cast<LPARAM>(&area));
        SetWindowTextW(edit_,text.c_str());
        return int(SendMessageW(edit_,EM_GETLINECOUNT,0,0))*line_height_+2*padding_y+2;
    }
};
inline bool register_effect_table(HINSTANCE module) {
    WNDCLASSW existing{};
    if (GetClassInfoW(module,L"DominionEffectTable",&existing)) return true;
    WNDCLASSW cls{}; cls.lpfnWndProc=effect_table_detail::procedure; cls.hInstance=module;
    cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)); cls.lpszClassName=L"DominionEffectTable";
    return RegisterClassW(&cls)!=0;
}
}
