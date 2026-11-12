#pragma once
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <functional>
#include <vector>

namespace kingdom {
template<class T> void move_entry(std::vector<T>& values,int from,int to) {
    if(from<0 || to<0 || from>=int(values.size()) || to>=int(values.size()) || from==to) return;
    if(from<to) std::rotate(values.begin()+from,values.begin()+from+1,values.begin()+to+1);
    else std::rotate(values.begin()+to,values.begin()+from,values.begin()+from+1);
}
// Indices, rather than labels, distinguish intentionally repeated effects.
struct ListReorder {
    std::function<void(int,int)> move;
    std::function<void()> remove;
    POINT origin{},last{}; int source=-1,insertion=-1; bool dragging=false;
    void cancel(HWND list) {
        KillTimer(list,9071); source=insertion=-1;dragging=false;
        if(GetCapture()==list) ReleaseCapture(); InvalidateRect(list,nullptr,FALSE);
    }
    void track(HWND list,POINT point) {
        last=point;RECT client{};GetClientRect(list,&client);
        const int count=int(SendMessageW(list,LB_GETCOUNT,0,0));
        const auto hit=SendMessageW(list,LB_ITEMFROMPOINT,0,MAKELPARAM(point.x,point.y));
        int row=LOWORD(hit);RECT bounds{};
        if(count==0) insertion=0;
        else if(point.y<0) insertion=int(SendMessageW(list,LB_GETTOPINDEX,0,0));
        else if(row>=count) insertion=count;
        else {SendMessageW(list,LB_GETITEMRECT,row,reinterpret_cast<LPARAM>(&bounds));insertion=row+(point.y>(bounds.top+bounds.bottom)/2);}
        insertion=std::clamp(insertion,0,count);InvalidateRect(list,nullptr,FALSE);
    }
    static LRESULT CALLBACK proc(HWND list,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data) {
        auto& self=*reinterpret_cast<ListReorder*>(data);
        if(msg==WM_NCDESTROY) {self.cancel(list);RemoveWindowSubclass(list,proc,id);delete &self;return DefSubclassProc(list,msg,wp,lp);}
        if((msg==WM_KEYDOWN || msg==WM_SYSKEYDOWN) && wp==VK_ESCAPE && self.source>=0){self.cancel(list);return 0;}
        if(msg==WM_KEYDOWN && wp==VK_DELETE && GetFocus()==list){self.remove();return 0;}
        if(msg==WM_SYSKEYDOWN && (wp==VK_UP || wp==VK_DOWN) && ((GetKeyState(VK_MENU)&0x8000) || (lp&(1L<<29)))) {
            const int row=int(SendMessageW(list,LB_GETCURSEL,0,0)),count=int(SendMessageW(list,LB_GETCOUNT,0,0));
            const int next=row+(wp==VK_UP?-1:1);if(row>=0 && next>=0 && next<count)self.move(row,next);return 0;
        }
        if(msg==WM_GETDLGCODE) return DefSubclassProc(list,msg,wp,lp)|DLGC_WANTARROWS|(self.source>=0?DLGC_WANTALLKEYS:0);
        if(msg==WM_LBUTTONDOWN){self.cancel(list);self.origin={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
            const auto hit=SendMessageW(list,LB_ITEMFROMPOINT,0,lp);if(!HIWORD(hit))self.source=LOWORD(hit);
            if(self.source>=0){SetFocus(list);SendMessageW(list,LB_SETCURSEL,self.source,0);SetCapture(list);
                SendMessageW(GetParent(list),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(list),LBN_SELCHANGE),reinterpret_cast<LPARAM>(list));}
            return 0;
        }
        if(msg==WM_MOUSEMOVE && self.source>=0 && (wp&MK_LBUTTON)) {
            POINT point{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
            if(!self.dragging && (abs(point.x-self.origin.x)>=GetSystemMetrics(SM_CXDRAG) || abs(point.y-self.origin.y)>=GetSystemMetrics(SM_CYDRAG))) {
                if(GetCapture()!=list)SetCapture(list);
                self.dragging=true;SetTimer(list,9071,75,nullptr);
            }
            if(self.dragging){self.track(list,point);return 0;}
        }
        if(msg==WM_TIMER && wp==9071 && self.dragging){POINT point=self.last;RECT c{};GetClientRect(list,&c);
            const int edge=20;if(point.y<edge || point.y>c.bottom-edge){SendMessageW(list,WM_VSCROLL,point.y<edge?SB_LINEUP:SB_LINEDOWN,0);self.track(list,point);}return 0;}
        if(msg==WM_LBUTTONUP && self.source>=0){const int from=self.source,at=self.insertion;const bool dragging=self.dragging;
            RECT c{};GetClientRect(list,&c);POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};const bool inside=PtInRect(&c,p)!=FALSE;
            self.cancel(list);if(dragging){const int to=at>from?at-1:at;if(inside && to>=0 && to!=from)self.move(from,to);}return 0;}
        if(msg==WM_CAPTURECHANGED && self.dragging){self.cancel(list);return 0;}
        const auto result=DefSubclassProc(list,msg,wp,lp);
        if(msg==WM_PAINT && self.dragging && self.insertion>=0){RECT r{};const int n=int(SendMessageW(list,LB_GETCOUNT,0,0));
            if(n>0){SendMessageW(list,LB_GETITEMRECT,(std::min)(n-1,self.insertion),reinterpret_cast<LPARAM>(&r));const int y=self.insertion==n?r.bottom-1:r.top;
                const auto dc=GetDC(list);const auto pen=CreatePen(PS_SOLID,3,RGB(159,89,15));const auto old=SelectObject(dc,pen);MoveToEx(dc,2,y,nullptr);LineTo(dc,r.right-2,y);SelectObject(dc,old);DeleteObject(pen);ReleaseDC(list,dc);}}
        return result;
    }
    static void attach(HWND list,std::function<void(int,int)> move,std::function<void()> remove){auto* state=new ListReorder;state->move=std::move(move);state->remove=std::move(remove);
        if(!SetWindowSubclass(list,proc,9071,reinterpret_cast<DWORD_PTR>(state)))delete state;}
};
}
