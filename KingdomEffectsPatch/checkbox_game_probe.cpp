#define KINGDOM_UI_TEST
#define UNICODE
#include "card_picker.cpp"
#include "checkbox_render_checks.hpp"
namespace checkbox_game {
HANDLE done=nullptr;DWORD result=0;HWND owner=nullptr;unsigned font_points=9;bool missing=false;bool ran=false;std::string failure;checkbox_checks::Session checks;kingdom::CardSelection mode=kingdom::CardSelection::multiple;
std::filesystem::path directory;
const kingdom::CardTarget fixtures[]{
 {"village",L"Dorf",L"Basisspiel",L"dorf village",L"Aktion",L"3",L"+1 Karte\r\n+2 Aktionen","base","actions",3,0,0,1},
 {"market",L"Markt",L"Basisspiel",L"markt market",L"Aktion",L"5",L"+1 Karte\r\n+1 Aktion\r\n+1 Kauf\r\n+1 Münze","base","actions",5,0,0,1},
 {"smithy",L"Schmiede",L"Basisspiel",L"schmiede smithy",L"Aktion",L"4",L"+3 Karten","base","actions",4,0,0,1},
#define CHECKBOX_FIXTURE(id,title) {id,title,L"Basisspiel",L##id,L"Aktion",L"3",L"Testkarte","base","actions",3,0,0,1}
 CHECKBOX_FIXTURE("cellar",L"Keller"),CHECKBOX_FIXTURE("chapel",L"Kapelle"),CHECKBOX_FIXTURE("moat",L"Burggraben"),
 CHECKBOX_FIXTURE("harbinger",L"Vorbotin"),CHECKBOX_FIXTURE("merchant",L"Händlerin"),CHECKBOX_FIXTURE("vassal",L"Vasall"),
 CHECKBOX_FIXTURE("workshop",L"Werkstatt"),CHECKBOX_FIXTURE("bureaucrat",L"Bürokrat"),CHECKBOX_FIXTURE("gardens",L"Gärten"),
 CHECKBOX_FIXTURE("militia",L"Miliz"),CHECKBOX_FIXTURE("moneylender",L"Geldverleiher"),CHECKBOX_FIXTURE("poacher",L"Wilddiebin"),
 CHECKBOX_FIXTURE("remodel",L"Umbau"),CHECKBOX_FIXTURE("throne_room",L"Thronsaal"),CHECKBOX_FIXTURE("bandit",L"Banditin"),
 CHECKBOX_FIXTURE("council_room",L"Ratsversammlung"),CHECKBOX_FIXTURE("festival",L"Jahrmarkt"),CHECKBOX_FIXTURE("laboratory",L"Laboratorium"),
#undef CHECKBOX_FIXTURE
};
INT_PTR CALLBACK dialog_proc(HWND dialog,UINT message,WPARAM wparam,LPARAM lparam) {
 const auto value=kingdom::picker_proc(dialog,message,wparam,lparam);
 if(message==WM_INITDIALOG) {
  try{checks.begin(dialog,directory,L"font"+std::to_wstring(font_points)+(missing?L"-missing":L"-game")+L"-mode"+std::to_wstring(unsigned(mode)));SetTimer(dialog,0x7351,150,nullptr);}
  catch(const std::exception& e){failure=e.what();PostMessageW(dialog,WM_COMMAND,IDCANCEL,0);}
 }
 if(message==WM_TIMER && wparam==0x7351) {
  KillTimer(dialog,0x7351);
  try{auto& picker=*reinterpret_cast<kingdom::Picker*>(GetWindowLongPtrW(dialog,DWLP_USER));checks.run(dialog,picker);ran=true;}
  catch(const std::exception& e){failure=e.what();}
  PostMessageW(dialog,WM_COMMAND,IDCANCEL,0);
 }
 return value;
}
std::vector<std::byte> dialog_template(HMODULE module,unsigned points) {
 const auto resource=FindResourceW(module,MAKEINTRESOURCEW(IDD_CARD_PICKER),RT_DIALOG);checkbox_checks::require(resource!=nullptr,"Missing dialog resource.");
 const auto loaded=LoadResource(module,resource);const auto size=SizeofResource(module,resource);const auto* data=static_cast<const std::byte*>(LockResource(loaded));
 std::vector<std::byte> copy(data,data+size);auto* current=reinterpret_cast<WORD*>(copy.data()+26);const auto* end=copy.data()+copy.size();
 auto skip=[&]{checkbox_checks::require(reinterpret_cast<std::byte*>(current+1)<=end,"Truncated dialog template.");if(*current==0xFFFF){current+=2;return;}while(*current){++current;checkbox_checks::require(reinterpret_cast<std::byte*>(current+1)<=end,"Truncated dialog string.");}++current;};
 skip();skip();skip();checkbox_checks::require(reinterpret_cast<std::byte*>(current+4)<=end,"Missing dialog font.");*current=static_cast<WORD>(points);return copy;
}
void CALLBACK launch(HWND window,UINT,UINT_PTR id,DWORD) {
 KillTimer(window,id);result=0;failure.clear();
 try {
  HMODULE module{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&launch),&module);
  wchar_t path[32768]{};GetModuleFileNameW(module,path,32768);directory=std::filesystem::path(path).parent_path()/L"checkbox-game-ui";
  wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);const auto package=std::filesystem::path(exe).parent_path()/L"Dominion.xpack";
  INITCOMMONCONTROLSEX controls{sizeof controls,ICC_TREEVIEW_CLASSES};InitCommonControlsEx(&controls);
  for(const auto absent:{false,true})for(const auto points:{9u,11u,14u})for(const auto selection:{kingdom::CardSelection::multiple,kingdom::CardSelection::single,kingdom::CardSelection::ordered}) {
   missing=absent;font_points=points;mode=selection;ran=false;kingdom::set_resource_package_for_test(absent?directory/L"no-game-art.xpack":package);
   kingdom::CardTargets cards;for(const auto& card:fixtures)cards.push_back(&card);
   const kingdom::AcceptCards accept=[](const kingdom::CardTargets&,bool){return std::wstring{};};
   const kingdom::CardTargets none;
   kingdom::Picker picker{L"Checkbox background test",cards,none,mode,accept};
   const auto resource=dialog_template(module,points);
   const auto shown=DialogBoxIndirectParamW(module,reinterpret_cast<const DLGTEMPLATE*>(resource.data()),owner,dialog_proc,reinterpret_cast<LPARAM>(&picker));
   checkbox_checks::require(shown!=-1,"Cannot create real game-hosted picker.");if(!failure.empty())throw std::runtime_error(failure);
   checkbox_checks::require(ran,"Game-hosted checkbox check did not run.");
   checkbox_checks::require(!picker.checks_images && !picker.checks_tree,"Checkbox ownership was not cleared on close.");
  }
  kingdom::set_resource_package_for_test(package);
 }catch(const std::exception& e){failure=e.what();result=1;}
 if(!directory.empty()){std::filesystem::create_directories(directory);std::ofstream report(directory/L"result.txt");report<<(result?failure:"PASS: game-hosted checkbox pixels, identity, mouse/Space messages, search; fonts 9/11/14; game/missing graphics")<<'\n';}
 SetEvent(done);
}
BOOL CALLBACK find(HWND window,LPARAM argument){DWORD pid{};GetWindowThreadProcessId(window,&pid);if(pid==GetCurrentProcessId()&&!GetWindow(window,GW_OWNER)){*reinterpret_cast<HWND*>(argument)=window;return FALSE;}return TRUE;}
}
extern "C" DWORD WINAPI checkbox_game_probe(void*) {
 using namespace checkbox_game;EnumWindows(find,reinterpret_cast<LPARAM>(&owner));if(!owner)return 921;
 done=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!done)return 922;
 if(!SetTimer(owner,0x7350,20,launch))return 923;
 if(WaitForSingleObject(done,55000)!=WAIT_OBJECT_0)return 924;
 CloseHandle(done);done=nullptr;return result;
}
#pragma comment(linker,"/EXPORT:RoundtripProbe=_checkbox_game_probe@4")
