#pragma once
// Developer checks; include AFTER card_picker.cpp in one test translation unit.
// Sends messages only to the test dialog. No input injection or foreground API.
#include <fstream>
#include <stdexcept>
#include <sstream>
#include "silent_control_test.hpp"
namespace checkbox_checks {
inline void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Snapshot {
 HDC screen=nullptr,dc=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ old=nullptr;HWND window=nullptr;int width=0,height=0;
 explicit Snapshot(HWND target):window(target) {
  RECT client{};require(GetClientRect(window,&client)!=FALSE,"Cannot inspect checkbox test window.");width=client.right;height=client.bottom;
  require(width>0 && height>0,"Empty checkbox test window.");
  screen=GetDC(window);if(screen)dc=CreateCompatibleDC(screen);
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;
  info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;void* pixels=nullptr;
  if(dc)bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
  if(bitmap)old=SelectObject(dc,bitmap);
  if(!screen || !dc || !bitmap || !pixels || !old || old==HGDI_ERROR){release();throw std::runtime_error("Cannot allocate checkbox screenshot.");}
  RECT full{0,0,width,height};FillRect(dc,&full,static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
  SendMessageW(window,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND|PRF_CHECKVISIBLE);
 }
 void release(){if(dc && old && old!=HGDI_ERROR)SelectObject(dc,old);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);if(screen)ReleaseDC(window,screen);old=nullptr;bitmap=nullptr;dc=nullptr;screen=nullptr;}
 ~Snapshot(){release();}
 Snapshot(const Snapshot&)=delete;Snapshot& operator=(const Snapshot&)=delete;
 void save(const std::filesystem::path& path) {
  Gdiplus::Bitmap image(bitmap,nullptr);const CLSID png{0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0,0,0xf8,0x1e,0xf3,0x2e}};
  require(image.Save(path.c_str(),&png,nullptr)==Gdiplus::Ok,"Cannot save checkbox screenshot.");
 }
};
struct Pixels {unsigned bright=0,dark=0,total=0;double luminance=0;};
inline Pixels measure(HDC dc,RECT rect) {
 Pixels result{};
 const int inset=(std::max)(2L,(rect.right-rect.left)/5);InflateRect(&rect,-inset,-inset);
 for(int y=rect.top;y<rect.bottom;++y)for(int x=rect.left;x<rect.right;++x) {
  const auto color=GetPixel(dc,x,y);require(color!=CLR_INVALID,"Checkbox pixel unavailable.");
  const auto light=(GetRValue(color)+GetGValue(color)+GetBValue(color))/3;
  ++result.total;result.luminance+=light;if(light>=175)++result.bright;if(light<115)++result.dark;
 }
 require(result.total>0,"Checkbox interior is empty.");result.luminance/=result.total;return result;
}
inline void no_black_margin(HDC dc,RECT rect) {
 for(int y=rect.top;y<rect.bottom;++y)for(int x=rect.left;x<rect.right;++x){
  const auto color=GetPixel(dc,x,y);require(color!=CLR_INVALID,"Checkbox margin pixel unavailable.");
  require(GetRValue(color)>8 || GetGValue(color)>8 || GetBValue(color)>8,"Black pixels in rendered checkbox or margin.");
 }
}
inline HTREEITEM card_item(const kingdom::Picker& picker,const char* id) {
 for(const auto& entry:picker.visible)if(std::string_view(entry.second->id)==id)return entry.first;
 return nullptr;
}
inline RECT checkbox_rect(HWND tree,HTREEITEM item) {
 RECT label{},client{};require(TreeView_GetItemRect(tree,item,&label,TRUE)!=FALSE,"Card row is not visible.");GetClientRect(tree,&client);
 int size=0,height=0;const auto images=TreeView_GetImageList(tree,TVSIL_STATE);
 require(images && ImageList_GetIconSize(images,&size,&height),"Missing checkbox image list.");
 const int y=(label.top+label.bottom)/2;int left=-1,right=-1;
 for(int x=0;x<(std::min)(client.right,label.left);++x){TVHITTESTINFO hit{};hit.pt={x,y};TreeView_HitTest(tree,&hit);if(hit.hItem==item && (hit.flags&TVHT_ONITEMSTATEICON)){if(left<0)left=x;right=x+1;}}
 require(left>=0 && right-left>=size,"Cannot locate actual checkbox hit region.");
 // SysTreeView32 draws the state image at the right edge of its hit region.
 return {right-size,y-height/2,right,y-height/2+height};
}
inline std::wstring text(HWND control) {
 const auto length=GetWindowTextLengthW(control);std::wstring value(length+1,L'\0');GetWindowTextW(control,value.data(),length+1);value.resize(length);return value;
}
inline void selection_invariants(HWND dialog,const kingdom::Picker& picker) {
 const auto tree=GetDlgItem(dialog,IDC_CARD_TREE);
 for(const auto& entry:picker.visible) {
  const auto state=TreeView_GetItemState(tree,entry.first,TVIS_STATEIMAGEMASK)&TVIS_STATEIMAGEMASK;
  const auto expected=static_cast<UINT>(INDEXTOSTATEIMAGEMASK(picker.has(entry.second)?2:1));
  if(state!=expected)throw std::runtime_error("Checkbox bits disagree: "+std::string(entry.second->id)+" actual="+std::to_string(state)+" expected="+std::to_string(expected)+" mode="+std::to_string(unsigned(picker.mode)));
 }
 require(bool(IsWindowEnabled(GetDlgItem(dialog,IDOK)))==!picker.selected.empty(),"Done state disagrees with selected cards.");
 if(picker.mode!=kingdom::CardSelection::ordered)return;
 const auto* candidate=kingdom::card_at(tree,TreeView_GetSelection(tree));
 if(candidate) {
  const auto chosen=picker.has(candidate);
  require(text(GetDlgItem(dialog,IDC_CARD_ADD))==kingdom::ui::tr(chosen?L"Bereits gewählt":L"Stapel hinzufügen"),"Ordered Add caption is not localized for chosen state.");
  require(bool(IsWindowEnabled(GetDlgItem(dialog,IDC_CARD_ADD)))==(!chosen && picker.selected.size()<20),"Ordered Add enabled state is incorrect.");
 }
 const auto list=GetDlgItem(dialog,IDC_CARD_SELECTION);
 require(SendMessageW(list,LB_GETCOUNT,0,0)==static_cast<LRESULT>(picker.selected.size()),"Ordered list count disagrees with selected cards.");
 for(std::size_t i=0;i<picker.selected.size();++i) {
  const auto length=SendMessageW(list,LB_GETTEXTLEN,i,0);require(length>=0,"Missing ordered label.");
  std::wstring label(static_cast<std::size_t>(length)+1,L'\0');SendMessageW(list,LB_GETTEXT,i,reinterpret_cast<LPARAM>(label.data()));label.resize(static_cast<std::size_t>(length));
  require(label==std::to_wstring(i+1)+L". "+kingdom::card_title(*picker.selected[i]),"Ordered labels or numbering disagree with selected cards.");
 }
}
struct Session {
 HIMAGELIST initial=nullptr;std::filesystem::path output;std::wstring name;
 void begin(HWND dialog,const std::filesystem::path& directory,std::wstring label) {
  output=directory;name=std::move(label);std::filesystem::create_directories(output);
  const auto tree=GetDlgItem(dialog,IDC_CARD_TREE);
  silent_control_test::guard.attach(tree);
  require(!(GetWindowLongPtrW(tree,GWL_STYLE)&TVS_CHECKBOXES),"Native checkbox style is still enabled.");
  initial=TreeView_GetImageList(tree,TVSIL_STATE);
  const auto& picker=*reinterpret_cast<const kingdom::Picker*>(GetWindowLongPtrW(dialog,DWLP_USER));
  require(initial==picker.checks_images,"Attached image list differs from picker-owned list at initialization.");
  if(picker.mode!=kingdom::CardSelection::single)require(initial && ImageList_GetImageCount(initial)>=3,"Custom checkbox image list missing at initialization.");
  else require(initial==nullptr,"Single selection has a state image list.");
 }
 void stable(HWND tree)const{require(TreeView_GetImageList(tree,TVSIL_STATE)==initial,"Checkbox image list replaced after initial creation.");}
 void run(HWND dialog,kingdom::Picker& picker) {
  const auto tree=GetDlgItem(dialog,IDC_CARD_TREE);stable(tree);
  require(initial==picker.checks_images,"Picker lost checkbox image ownership.");
  if(picker.mode==kingdom::CardSelection::single) {
   require(!(GetWindowLongPtrW(tree,GWL_STYLE)&TVS_CHECKBOXES),"Single selection has native checkboxes.");
   for(const auto* query:{L"",L"village",L"market",L""}) {
    SetDlgItemTextW(dialog,IDC_CARD_QUERY,query);
    for(auto group=TreeView_GetRoot(tree);group;group=TreeView_GetNextSibling(tree,group))TreeView_Expand(tree,group,TVE_EXPAND);
    RedrawWindow(tree,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);stable(tree);
    for(const auto& entry:picker.visible)require((TreeView_GetItemState(tree,entry.first,TVIS_STATEIMAGEMASK)&TVIS_STATEIMAGEMASK)==0,"Single card has state-image bits.");
   }
   Snapshot full(dialog);full.save(output/(name+L"-no-checkbox-dialog.png"));return;
  }
  require(picker.selected.empty(),"Fixture starts with selected cards.");
  selection_invariants(dialog,picker);
  SetDlgItemTextW(dialog,IDC_CARD_QUERY,L"");
  for(auto group=TreeView_GetRoot(tree);group;group=TreeView_GetNextSibling(tree,group))TreeView_Expand(tree,group,TVE_EXPAND);
  RedrawWindow(tree,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);stable(tree);
  const auto first=card_item(picker,"village");require(first!=nullptr,"Missing fixture Village.");
  TreeView_EnsureVisible(tree,first);TreeView_SelectItem(tree,first);UpdateWindow(tree);
  require(TreeView_GetCheckState(tree,first)==FALSE,"Initial checkbox is unexpectedly checked.");
  const auto rect=checkbox_rect(tree,first);
  Snapshot empty(tree);no_black_margin(empty.dc,rect);const auto empty_pixels=measure(empty.dc,rect);empty.save(output/(name+L"-empty-tree.png"));
  {Snapshot full(dialog);full.save(output/(name+L"-empty-dialog.png"));}
  const auto click=MAKELPARAM((rect.left+rect.right)/2,(rect.top+rect.bottom)/2);
  SendMessageW(tree,WM_LBUTTONDOWN,MK_LBUTTON,click);SendMessageW(tree,WM_LBUTTONUP,0,click);
  require(picker.selected.size()==1 && std::string_view(picker.selected.front()->id)=="village" && TreeView_GetCheckState(tree,first),"Checkbox mouse message did not select Village.");
  selection_invariants(dialog,picker);
  RedrawWindow(tree,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);stable(tree);
  Snapshot checked(tree);no_black_margin(checked.dc,rect);const auto checked_pixels=measure(checked.dc,rect);checked.save(output/(name+L"-checked-tree.png"));
  {Snapshot full(dialog);full.save(output/(name+L"-checked-dialog.png"));}
  std::ofstream report(output/(name+L"-pixels.tsv"));report<<"state\tbright\tdark\ttotal\tmean\nempty\t"<<empty_pixels.bright<<'\t'<<empty_pixels.dark<<'\t'<<empty_pixels.total<<'\t'<<empty_pixels.luminance<<"\nchecked\t"<<checked_pixels.bright<<'\t'<<checked_pixels.dark<<'\t'<<checked_pixels.total<<'\t'<<checked_pixels.luminance<<"\nimage_list\t"<<reinterpret_cast<std::uintptr_t>(initial)<<"\nrect\t"<<rect.left<<'\t'<<rect.top<<'\t'<<rect.right<<'\t'<<rect.bottom<<'\n';
  require(empty_pixels.bright*100>=empty_pixels.total*65 && empty_pixels.luminance>=170,"Empty checkbox interior is dark.");
  require(checked_pixels.dark>=empty_pixels.dark+3 && checked_pixels.luminance+5<empty_pixels.luminance,"Checked checkbox has no visible dark checkmark.");
  const auto space=[&]{const auto beeps=silent_control_test::blocked.load();SendMessageW(tree,WM_KEYDOWN,VK_SPACE,1);SendMessageW(tree,WM_CHAR,L' ',1);SendMessageW(tree,WM_KEYUP,VK_SPACE,0xC0000001);require(silent_control_test::blocked.load()==beeps,"Space requested a Windows alert (silenced in test).");};
  SetFocus(tree);space();require(picker.selected.empty() && !TreeView_GetCheckState(tree,first),"Space down/char/up did not toggle exactly once to empty.");selection_invariants(dialog,picker);
  space();require(picker.selected.size()==1 && TreeView_GetCheckState(tree,first),"Space down/char/up did not toggle exactly once to checked.");selection_invariants(dialog,picker);
  const auto selected=picker.selected;
  for(const auto* query:{L"market",L"smithy",L"village",L""}) {
   SetDlgItemTextW(dialog,IDC_CARD_QUERY,query);UpdateWindow(tree);stable(tree);
   require(picker.selected==selected,"Search changed selected cards.");
   selection_invariants(dialog,picker);
  }
  for(auto group=TreeView_GetRoot(tree);group;group=TreeView_GetNextSibling(tree,group)){TreeView_Expand(tree,group,TVE_COLLAPSE);TreeView_Expand(tree,group,TVE_EXPAND);}
  UpdateWindow(tree);stable(tree);require(picker.selected==selected,"Expansion changed selected cards.");
  selection_invariants(dialog,picker);
  if(picker.mode==kingdom::CardSelection::ordered)ordered(dialog,picker);
 }
 void ordered(HWND dialog,kingdom::Picker& picker) {
  const auto progress=[&](const char* stage){std::ofstream file(output/(name+L"-progress.txt"),std::ios::app);file<<stage<<std::endl;};
  progress("ordered-start");
  const auto tree=GetDlgItem(dialog,IDC_CARD_TREE),list=GetDlgItem(dialog,IDC_CARD_SELECTION);
  silent_control_test::guard.attach(tree);
  const auto validate=[&]{stable(tree);selection_invariants(dialog,picker);};
  const auto select=[&](const char* id){const auto item=card_item(picker,id);require(item!=nullptr,"Missing ordered fixture card.");TreeView_EnsureVisible(tree,item);TreeView_SelectItem(tree,item);return item;};
  const auto mouse=[&](HTREEITEM item){const auto r=checkbox_rect(tree,item);const auto xy=MAKELPARAM((r.left+r.right)/2,(r.top+r.bottom)/2);SendMessageW(tree,WM_LBUTTONDOWN,MK_LBUTTON,xy);SendMessageW(tree,WM_LBUTTONUP,0,xy);validate();};
  const auto twice=[&](HTREEITEM item){RECT r{};require(TreeView_GetItemRect(tree,item,&r,TRUE)!=FALSE,"Missing double-click label.");const auto xy=MAKELPARAM(r.left+3,(r.top+r.bottom)/2);PostMessageW(tree,WM_LBUTTONDOWN,MK_LBUTTON,xy);PostMessageW(tree,WM_LBUTTONUP,0,xy);PostMessageW(tree,WM_LBUTTONDBLCLK,MK_LBUTTON,xy);PostMessageW(tree,WM_LBUTTONUP,0,xy);MSG message{};unsigned count=0;while(count++<16 && PeekMessageW(&message,tree,WM_MOUSEFIRST,WM_MOUSELAST,PM_REMOVE))DispatchMessageW(&message);validate();};
  progress("before-double-clicks");
  const auto village=select("village");twice(village);require(picker.selected.size()==1,"Double-click removed an already chosen card.");
  const auto market=select("market");twice(market);require(picker.selected.size()==2,"Double-click failed to add card.");twice(market);require(picker.selected.size()==2,"Double-click duplicated chosen card.");
  mouse(select("village"));require(picker.selected.size()==1,"Unchecking failed to remove ordered card.");mouse(select("village"));require(picker.selected.size()==2 && std::string_view(picker.selected.back()->id)=="village","Rechecking did not append at end.");
  SendMessageW(list,LB_SETCURSEL,0,0);SendMessageW(dialog,WM_COMMAND,IDC_CARD_REMOVE,0);validate();require(picker.selected.size()==1 && !picker.has(kingdom::card_at(tree,market)),"Lower Remove did not clear corresponding checkbox.");
  progress("before-capacity");
  require(picker.cards.size()>=21,"Ordered capacity fixture needs at least 21 cards.");
  for(const auto& entry:picker.visible) {
   if(picker.selected.size()==20)break;if(picker.has(entry.second))continue;
   TreeView_SelectItem(tree,entry.first);SendMessageW(dialog,WM_COMMAND,IDC_CARD_ADD,0);validate();
  }
  require(picker.selected.size()==20,"Could not fill 20 ordered slots.");
  const auto saved=picker.selected;
  for(const auto& entry:picker.visible)if(!picker.has(entry.second)) {
   TreeView_EnsureVisible(tree,entry.first);TreeView_SelectItem(tree,entry.first);SendMessageW(dialog,WM_COMMAND,IDC_CARD_ADD,0);validate();require(picker.selected==saved,"21st Add changed ordered selection.");
   mouse(entry.first);require(picker.selected==saved,"21st checkbox changed ordered selection.");
   const auto beeps=silent_control_test::blocked.load();SetFocus(tree);SendMessageW(tree,WM_KEYDOWN,VK_SPACE,1);SendMessageW(tree,WM_CHAR,L' ',1);SendMessageW(tree,WM_KEYUP,VK_SPACE,0xC0000001);validate();
   require(picker.selected==saved,"21st Space sequence changed ordered selection.");require(silent_control_test::blocked.load()==beeps,"Rejected ordered Space requested an alert.");break;
  }
  progress("before-drag");
  SendMessageW(list,LB_SETTOPINDEX,0,0);RECT first{},second{};SendMessageW(list,LB_GETITEMRECT,0,reinterpret_cast<LPARAM>(&first));SendMessageW(list,LB_GETITEMRECT,1,reinterpret_cast<LPARAM>(&second));
  SendMessageW(list,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(8,first.top+2));SendMessageW(list,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(8,second.bottom-1));SendMessageW(list,WM_LBUTTONUP,0,MAKELPARAM(8,second.bottom-1));validate();require(picker.selected[1]==saved[0] && picker.selected[0]==saved[1],"Ordered drag failed.");
  progress("before-alt");
  SendMessageW(list,LB_SETCURSEL,1,0);const auto beeps=silent_control_test::blocked.load();SendMessageW(list,WM_SYSKEYDOWN,VK_UP,1L<<29);validate();require(picker.selected==saved,"Ordered Alt+Up failed.");require(silent_control_test::blocked.load()==beeps,"Ordered Alt+Up requested an alert.");
  progress("before-delete");
  SendMessageW(list,LB_SETCURSEL,0,0);SetFocus(list);SendMessageW(list,WM_KEYDOWN,VK_DELETE,1);SendMessageW(list,WM_KEYUP,VK_DELETE,0xC0000001);validate();require(picker.selected.size()==19,"Ordered Delete failed.");
  progress("before-clear");
  for(unsigned tries=0;!picker.selected.empty() && tries<21;++tries){SendMessageW(list,LB_SETCURSEL,0,0);SendMessageW(dialog,WM_COMMAND,IDC_CARD_REMOVE,0);validate();}
  progress("after-clear");
  require(picker.selected.empty(),"Ordered Remove did not clear the selection within 21 attempts.");
  require(!IsWindowEnabled(GetDlgItem(dialog,IDOK)),"Empty ordered selection can be accepted.");
  const auto root=TreeView_GetRoot(tree);if(root)TreeView_EnsureVisible(tree,root);
 }
};
}
