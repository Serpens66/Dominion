#pragma once
#include <windows.h>
#include <windowsx.h>
#include <oleacc.h>
#pragma comment(lib, "oleacc.lib")
#include <algorithm>
#include <new>
#include <string>
#include <vector>

namespace kingdom {
// A pixel-scrolled owner-drawn table. The editor's private LB_* interface and
// selection notifications stay intact; rows are not limited to 255 pixels.
constexpr UINT effect_table_layout = WM_APP + 83;
namespace effect_table_detail {
struct Row { std::wstring label; LPARAM data=0; int height=24; };
struct Model {
    std::vector<Row> rows;
    std::vector<int> offsets{0};
    HFONT font=nullptr;
    int selected=-1, position=0, wheel=0;
    bool redraw=true;
    std::wstring search;
    ULONGLONG typed_at=0;
    int page(HWND window) const { RECT r{}; GetClientRect(window,&r); return (std::max)(1,int(r.bottom)); }
    int limit(HWND window) const { return (std::max)(0,offsets.back()-page(window)); }
    void scroll(HWND window,int value) {
        position=std::clamp(value,0,limit(window));
        SCROLLINFO info{sizeof info,SIF_POS}; info.nPos=position;
        SetScrollInfo(window,SB_VERT,&info,TRUE); if (redraw) InvalidateRect(window,nullptr,FALSE);
    }
    void rebuild(HWND window) {
        offsets.assign(1,0);
        for (const auto& row:rows) offsets.push_back(offsets.back()+row.height);
        position=std::clamp(position,0,limit(window));
        SCROLLINFO info{sizeof info,SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL};
        info.nMin=0; info.nMax=(std::max)(0,offsets.back()-1); info.nPage=UINT(page(window)); info.nPos=position;
        SetScrollInfo(window,SB_VERT,&info,TRUE);
        if (redraw) InvalidateRect(window,nullptr,FALSE);
    }
    int at(int pixel) const {
        if (rows.empty() || pixel<0 || pixel>=offsets.back()) return -1;
        return int(std::upper_bound(offsets.begin(),offsets.end(),pixel)-offsets.begin())-1;
    }
    void select(HWND window,int row,bool notify) {
        if (row<0 || row>=int(rows.size())) row=-1;
        const bool changed=selected!=row; selected=row;
        if (row>=0 && row+1<int(offsets.size())) {
            if (offsets[row]<position) scroll(window,offsets[row]);
            else if (offsets[row+1]>position+page(window))
                scroll(window,rows[row].height>page(window)?offsets[row]:offsets[row+1]-page(window));
        }
        if (redraw) InvalidateRect(window,nullptr,FALSE);
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
            state.font=reinterpret_cast<HFONT>(wparam);
            if (!state.rows.empty()) SendMessageW(GetParent(window),effect_table_layout,0,0);
            return 0;
        case WM_SIZE:
            if (!state.rows.empty()) SendMessageW(GetParent(window),effect_table_layout,0,0);
            else state.rebuild(window);
            return 0;
        case LB_RESETCONTENT: state.rows.clear(); state.offsets.assign(1,0); state.position=0; state.selected=-1;
            if (state.redraw) state.rebuild(window); return 0;
        case LB_ADDSTRING: state.rows.push_back({reinterpret_cast<const wchar_t*>(lparam),0,24});
            if (state.redraw) state.rebuild(window); return LRESULT(state.rows.size()-1);
        case LB_GETCOUNT: return LRESULT(state.rows.size());
        case LB_SETITEMDATA: if (!valid()) return LB_ERR; state.rows[wparam].data=lparam; return 0;
        case LB_GETITEMDATA: return valid()?state.rows[wparam].data:LB_ERR;
        case LB_SETITEMHEIGHT: if (!valid() || lparam<=0 || lparam>100000) return LB_ERR;
            state.rows[wparam].height=int(lparam); if (state.redraw) state.rebuild(window); return 0;
        case LB_GETITEMHEIGHT: return valid()?state.rows[wparam].height:LB_ERR;
        case LB_SETCURSEL: state.select(window,int(wparam),false); return state.selected;
        case LB_GETCURSEL: return state.selected<0?LB_ERR:state.selected;
        case LB_GETSEL: return valid()?(int(wparam)==state.selected):LB_ERR;
        case LB_GETSELCOUNT: return state.selected>=0?1:0;
        case LB_GETSELITEMS: if (!wparam || !lparam || state.selected<0) return 0; *reinterpret_cast<int*>(lparam)=state.selected; return 1;
        case LB_SETSEL: if (wparam) state.select(window,int(lparam),true); return 0;
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
            if (row>=0) state.select(window,row,true); return 0;
        }
        case WM_KEYDOWN: {
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
                item.itemState=(state.selected==row?ODS_SELECTED:0)|((state.selected==row && GetFocus()==window)?ODS_FOCUS:0);
                item.rcItem={0,top,client.right,top+state.rows[row].height}; item.itemData=ULONG_PTR(state.rows[row].data);
                SendMessageW(GetParent(window),WM_DRAWITEM,item.CtlID,reinterpret_cast<LPARAM>(&item));
            }
            if (message==WM_PAINT) EndPaint(window,&paint); return 0;
        }
    }
    return DefWindowProcW(window,message,wparam,lparam);
}
}
inline bool register_effect_table(HINSTANCE module) {
    WNDCLASSW existing{};
    if (GetClassInfoW(module,L"DominionEffectTable",&existing)) return true;
    WNDCLASSW cls{}; cls.lpfnWndProc=effect_table_detail::procedure; cls.hInstance=module;
    cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)); cls.lpszClassName=L"DominionEffectTable";
    return RegisterClassW(&cls)!=0;
}
}
