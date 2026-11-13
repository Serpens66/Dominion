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
  if(picker.mode==kingdom::CardSelection::multiple)require(initial && ImageList_GetImageCount(initial)>=3,"Custom checkbox image list missing at initialization.");
  else require(initial==nullptr,"Single/ordered selection has a state image list.");
 }
 void stable(HWND tree)const{require(TreeView_GetImageList(tree,TVSIL_STATE)==initial,"Checkbox image list replaced after initial creation.");}
 void run(HWND dialog,kingdom::Picker& picker) {
  const auto tree=GetDlgItem(dialog,IDC_CARD_TREE);stable(tree);
  require(initial==picker.checks_images,"Picker lost checkbox image ownership.");
  if(picker.mode!=kingdom::CardSelection::multiple) {
   require(!(GetWindowLongPtrW(tree,GWL_STYLE)&TVS_CHECKBOXES),"Single/ordered selection has native checkboxes.");
   for(const auto* query:{L"",L"village",L"market",L""}) {
    SetDlgItemTextW(dialog,IDC_CARD_QUERY,query);
    for(auto group=TreeView_GetRoot(tree);group;group=TreeView_GetNextSibling(tree,group))TreeView_Expand(tree,group,TVE_EXPAND);
    RedrawWindow(tree,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);stable(tree);
    for(const auto& entry:picker.visible)require(TreeView_GetItemState(tree,entry.first,TVIS_STATEIMAGEMASK)==0,"Single/ordered card has state-image bits.");
   }
   Snapshot full(dialog);full.save(output/(name+L"-no-checkbox-dialog.png"));return;
  }
  require(picker.selected.empty(),"Fixture starts with selected cards.");
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
  RedrawWindow(tree,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);stable(tree);
  Snapshot checked(tree);no_black_margin(checked.dc,rect);const auto checked_pixels=measure(checked.dc,rect);checked.save(output/(name+L"-checked-tree.png"));
  {Snapshot full(dialog);full.save(output/(name+L"-checked-dialog.png"));}
  std::ofstream report(output/(name+L"-pixels.tsv"));report<<"state\tbright\tdark\ttotal\tmean\nempty\t"<<empty_pixels.bright<<'\t'<<empty_pixels.dark<<'\t'<<empty_pixels.total<<'\t'<<empty_pixels.luminance<<"\nchecked\t"<<checked_pixels.bright<<'\t'<<checked_pixels.dark<<'\t'<<checked_pixels.total<<'\t'<<checked_pixels.luminance<<"\nimage_list\t"<<reinterpret_cast<std::uintptr_t>(initial)<<"\nrect\t"<<rect.left<<'\t'<<rect.top<<'\t'<<rect.right<<'\t'<<rect.bottom<<'\n';
  require(empty_pixels.bright*100>=empty_pixels.total*65 && empty_pixels.luminance>=170,"Empty checkbox interior is dark.");
  require(checked_pixels.dark>=empty_pixels.dark+3 && checked_pixels.luminance+5<empty_pixels.luminance,"Checked checkbox has no visible dark checkmark.");
  const auto space=[&]{const auto beeps=silent_control_test::blocked.load();SendMessageW(tree,WM_KEYDOWN,VK_SPACE,1);SendMessageW(tree,WM_CHAR,L' ',1);SendMessageW(tree,WM_KEYUP,VK_SPACE,0xC0000001);require(silent_control_test::blocked.load()==beeps,"Space requested a Windows alert (silenced in test).");};
  SetFocus(tree);space();require(picker.selected.empty() && !TreeView_GetCheckState(tree,first),"Space down/char/up did not toggle exactly once to empty.");
  space();require(picker.selected.size()==1 && TreeView_GetCheckState(tree,first),"Space down/char/up did not toggle exactly once to checked.");
  const auto selected=picker.selected;
  for(const auto* query:{L"market",L"smithy",L"village",L""}) {
   SetDlgItemTextW(dialog,IDC_CARD_QUERY,query);UpdateWindow(tree);stable(tree);
   require(picker.selected==selected,"Search changed selected cards.");
   for(const auto& entry:picker.visible)require(bool(TreeView_GetCheckState(tree,entry.first))==picker.has(entry.second),"Search renders wrong checkbox state.");
  }
  for(auto group=TreeView_GetRoot(tree);group;group=TreeView_GetNextSibling(tree,group)){TreeView_Expand(tree,group,TVE_COLLAPSE);TreeView_Expand(tree,group,TVE_EXPAND);}
  UpdateWindow(tree);stable(tree);require(picker.selected==selected,"Expansion changed selected cards.");
 }
};
}
