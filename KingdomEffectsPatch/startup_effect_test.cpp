// Native setup generation tests; disposable profiles and an invisible desktop only.
#define wmain background_main
#include "background_effect_test.cpp"
#undef wmain
#include <sstream>

struct StartupCase {const char* commands; std::vector<DWORD> ids;};
const StartupCase startup_cases[]{
 {"twist_from_the_throne,twist_landfill,twist_so_much_stuff,twist_delayed_joy,twist_pennies_saved,twist_real_estate_barons,twist_head_start,twist_turn_turn_turn",{0x17D4,0x1776,0x1778,0x17DE,0x17DB,0x17DD,0x17D5,0x17E1}},
 {"twist_pennies_saved",{0x17DB}},
 {"start_double_pile(market),twist_pennies_saved,twist_big_ideas",{0xDB8,0x17DB,0x1774}},
 {"twist_pennies_saved,twist_busy_busy,twist_big_ideas",{0x17DB,0x1771,0x1774}},
 {"twist_busy_busy,twist_big_ideas,twist_pennies_saved",{0x1771,0x1774,0x17DB}},
 {"twist_real_estate_barons",{0x17DD}},
 {"start_double_pile(market),twist_real_estate_barons,twist_big_ideas",{0xDB8,0x17DD,0x1774}},
 {"twist_head_start",{0x17D5}},
 {"start_double_pile(market),twist_head_start,twist_big_ideas",{0xDB8,0x17D5,0x1774}},
 {"twist_turn_turn_turn",{0x17E1}},
 {"start_double_pile(market),twist_turn_turn_turn,twist_big_ideas",{0xDB8,0x17E1,0x1774}},
 {"twist_garbage_heap,twist_beakers,twist_big_ideas",{0x17DA,0x17D6,0x1774}},
};
DWORD load_patch(const Child& child,const fs::path& dll,DWORD request=1) {
 const auto local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
 require(local!=nullptr,"Cannot inspect probe DLL.");
 const auto proc=GetProcAddress(local,"RoundtripProbe"); require(proc!=nullptr,"Missing test export.");
 std::array<DWORD,512> args{};
 args[0]=reinterpret_cast<DWORD>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"));
 args[1]=reinterpret_cast<DWORD>(proc)-reinterpret_cast<DWORD>(local); FreeLibrary(local); args[2]=request;
 require((dll.native().size()+1)*2<sizeof(args)-12,"Path too long.");
 std::memcpy(args.data()+3,dll.c_str(),(dll.native().size()+1)*2);
 constexpr std::array<BYTE,23> code{0x53,0x8B,0x5C,0x24,0x08,0x8D,0x4B,0x0C,0x51,0xFF,0x13,0x03,0x43,0x04,0xFF,0x73,0x08,0xFF,0xD0,0x5B,0xC2,0x04,0};
 return child.query(code,args,30000);
}
int wmain(int argc,wchar_t** argv) {
 try {
  require(argc==5 || argc==6,"Usage: startup_effect_test <copy exe> <probe dll> <setupData|kingdomData> <patched 0|1> [case]");
  const auto exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
  Handle working(CreateFileW(exe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
  Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
  require(working.valid() && original.valid() && !same_file(identify(working.value),identify(original.value)),"Separate test copy required.");verify_executable(working.value);
  const bool setup=std::wstring_view(argv[3])==L"setupData"; require(setup || std::wstring_view(argv[3])==L"kingdomData","Unknown path.");
  const bool patched=std::wstring_view(argv[4])==L"1";
  require(patched || std::wstring_view(argv[4])==L"0","Expected patched 0 or 1.");
  const int selected=argc==6?_wtoi(argv[5]):-1;
  require(selected>=-1 && selected<int(std::size(startup_cases)),"Unknown test case.");
  auto name=L"DominionStartup_"+std::to_wstring(GetCurrentProcessId()); HiddenDesktop desktop(name);
  const auto root=fs::temp_directory_path()/name;require(fs::create_directory(root),"Scratch exists.");
  unsigned failed=0;
  for(std::size_t n=0;n<std::size(startup_cases);++n) {
   if(selected>=0 && n!=std::size_t(selected)) continue;
   const auto& test=startup_cases[n];const auto folder=root/std::to_wstring(n);require(fs::create_directory(folder),"Scratch exists.");
   const Probe probe{test.commands,0,{5,5},Metric::hand,0,0,1,setup?"-e base":base_kingdom};
   write_profile(folder,probe,1,setup?"setupData":"kingdomData");
   Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
   const auto client=wait_for_client(child,base,folder);if(patched) require(load_patch(child,dll)==0,"Patch initialization failed.");
   require(load_patch(child,dll,42)==0,"Native collection grant preparation failed.");
   start_local_game(child,base,client,folder);
   std::vector<DWORD> ids; for(unsigned i=0;i<32;++i) {const auto id=child.read(base+game_state+0x1280+i*0x14);if(!id) break;ids.push_back(id);}
   const bool ok=ids==test.ids; if(!ok) ++failed;
   std::printf("case=%zu path=%s patched=%d expected=%zu installed=%zu %s ids=",n,setup?"setupData":"kingdomData",patched,test.ids.size(),ids.size(),ok?"PASS":"FAIL");for(auto id:ids)std::printf("%lX,",id);std::puts("");std::fflush(stdout);
   if(patched && setup && n==0) {
       const auto previous=child.read(client+0x506C);
       // Dispatcher action 9 closes the old game (B1F60) and starts a new
       // local game (B1E20) on the correct native client thread.
       const DWORD restart=9;child.write(client+0x5044,&restart,sizeof restart);
       const auto until=GetTickCount64()+15000;
       while(GetTickCount64()<until && child.alive() &&
           (child.read(client+0x506C)==previous || child.read(client+0x5068)!=1 || child.read(base+game_state+0x19E0)!=1)) Sleep(50);
       require(child.alive() && child.read(client+0x506C)!=previous,"Native close/restart did not complete.");
       for(std::size_t i=0;i<test.ids.size();++i)
           require(child.read(base+game_state+0x1280+i*0x14)==test.ids[i],"Close/restart lost effects.");
       const auto reopened=load_patch(child,dll,3);
       std::printf("native close/restart + persisted configuration + F8 resolver: %lu effects\n",reopened);
       if(reopened!=test.ids.size()) ++failed;
   }
  }
  std::wprintf(L"Scratch: %ls\n",root.c_str());return failed?1:0;
 }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
