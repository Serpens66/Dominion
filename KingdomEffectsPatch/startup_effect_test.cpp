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
// This targeted path uses the production patch, native grant preparation and
// the same local start dispatcher as the F8 setupData regression tests.
int run_basic_target_checks(const fs::path& exe,const fs::path& dll) {
 struct BasicCase {const char* command; DWORD effect,type; unsigned baseline; bool owned; unsigned factor;};
 const BasicCase cases[]{
  {"start_add_card(gold)",0xDB5,0x106,0,true,1},
  {"start_add_card(platinum)",0xDB5,0x600,0,true,1},
  {"start_double_pile(gold)",0xDB8,0x106,30,false,2},
  {"start_double_pile(platinum)",0xDB8,0x600,12,false,2},
  {"start_double_pile(estate)",0xDB8,0x101,8,false,2},
  {"start_double_pile(duchy)",0xDB8,0x102,8,false,2},
  {"start_double_pile(province)",0xDB8,0x103,8,false,2},
  {"start_double_pile(colony)",0xDB8,0x601,8,false,2},
  {"start_triple_pile(gold)",0xDBA,0x106,30,false,3},
  {"start_triple_pile(province)",0xDBA,0x103,8,false,3},
 };
 auto name=L"DominionBasicTargets_"+std::to_wstring(GetCurrentProcessId());HiddenDesktop desktop(name);
 const auto root=fs::temp_directory_path()/name;require(fs::create_directory(root),"Scratch exists.");
 unsigned failures=0,number=0;
 const std::string kingdom=std::string(base_kingdom)+" -m colonies";
 for(const auto& test:cases) for(unsigned copies=0;copies<=2;++copies) {
  const auto folder=root/std::to_wstring(++number);require(fs::create_directory(folder),"Scratch exists.");
  const Probe probe{test.command,test.effect,{},test.owned?Metric::owned:Metric::supply,test.type,0,2,kingdom.c_str()};
  write_profile(folder,probe,copies,"setupData");
  Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
  const auto client=wait_for_client(child,base,folder);
  require(load_patch(child,dll)==0,"Patch initialization failed.");
  require(load_patch(child,dll,42)==0,"Native collection grant preparation failed.");
  start_local_game(child,base,client,folder);
  const auto state=base+game_state;const auto cards=child.read(state+0x1520),players=child.read(state+0xD38);
  require(players==2 && cards>2 && cards<=card_capacity,"Unexpected local fixture.");
  unsigned installed=0;for(unsigned i=0;i<32;++i)if(child.read(state+0x1280+i*0x14)==test.effect)++installed;
  unsigned expected=test.baseline;for(unsigned i=0;i<copies;++i)expected=test.owned?expected+1:expected*test.factor;
  bool ok=installed==copies;
  if(test.owned) {
   for(DWORD who=0;who<players;++who) {
    const auto actual=owned_count(child,state,cards,who,test.type);ok&=actual==expected;
    std::printf("basic %s copies=%u player=%lu measured=%u expected=%u %s\n",test.command,copies,who,actual,expected,actual==expected?"PASS":"FAIL");
   }
  } else {
   const auto slot=pile_slot(child,state,test.type);unsigned actual=0,visited=0;
   auto handle=child.read(state+0x152C+slot*0x10);
   while(handle) {const auto index=handle&0xFFFF;require(index<cards && visited++<cards,"Invalid basic supply list.");const auto card=state+0x1A48+index*0x64;if(child.read(card+4)==test.type)++actual;handle=child.read(card+0x5C);}
   ok&=actual==expected;
   std::printf("basic %s copies=%u slot=%lu measured=%u expected=%u installed=%u %s\n",test.command,copies,slot,actual,expected,installed,ok?"PASS":"FAIL");
  }
  if(!ok)++failures;std::fflush(stdout);
 }
 std::printf("Basic target cases=%u failed=%u\n",number,failures);std::wprintf(L"Scratch: %ls\n",root.c_str());return failures?1:0;
}

int run_basic_automatic_checks(const fs::path& exe,const fs::path& dll) {
 struct AutomaticCase {const char* effects;bool colonies;bool both;};
 const AutomaticCase cases[]{{"",false,false},{"",true,false},{"start_double_pile(gold)",true,false},
  {"start_triple_pile(gold)",true,false},{"start_double_pile(colony)",true,false},
  {"start_double_pile(province)",true,false},{"",true,true}};
 const char* basic_names[]{"copper","silver","gold","estate","duchy","province","platinum","colony"};
 const DWORD basic_types[]{0x104,0x105,0x106,0x101,0x102,0x103,0x600,0x601};
 const char* kingdom_names[]{"cellar","market","merchant","militia","mine","moat","remodel","smithy","village","workshop"};
 const DWORD kingdom_types[]{512,533,516,522,534,514,525,526,518,519};
 auto name=L"DominionBasicAutomatic_"+std::to_wstring(GetCurrentProcessId());HiddenDesktop desktop(name);
 const auto root=fs::temp_directory_path()/name;require(fs::create_directory(root),"Scratch exists.");unsigned failures=0,number=0;
 for(std::size_t c=0;c<std::size(cases);++c) {
  const auto& test=cases[c];std::array<unsigned,18> expected{};
  for(unsigned automatic=0;automatic<2;++automatic) {
   std::string commands=test.effects;auto add=[&](const std::string& command){if(!commands.empty())commands+=",";commands+=command;};
   if(!automatic) {
    for(unsigned i=0;i<(test.colonies?7u:6u);++i) {
     if(i==2 && std::string_view(test.effects).find("gold")!=std::string_view::npos)continue;
     if(i==5 && (std::string_view(test.effects).find("province")!=std::string_view::npos ||
        std::string_view(test.effects).find("colony")!=std::string_view::npos))continue;
     add(std::string("start_double_pile(")+basic_names[i]+")");
    }
    if(test.both)for(const auto* id:kingdom_names)add(std::string("start_double_pile(")+id+")");
   }
   const auto folder=root/std::to_wstring(++number);require(fs::create_directory(folder),"Scratch exists.");
   const auto kingdom=std::string(base_kingdom)+(test.colonies?" -m Colonies":" -m NoColonies");
   const Probe probe{commands.c_str(),0,{},Metric::hand,0,0,1,kingdom.c_str()};write_profile(folder,probe,commands.empty()?0:1,"setupData");
   Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
   const auto client=wait_for_client(child,base,folder);require(load_patch(child,dll)==0,"Patch initialization failed.");
   require(load_patch(child,dll,42)==0,"Native collection grant preparation failed.");
   if(automatic)require(load_patch(child,dll,test.both?57:56)==0,"Cannot enable automatic fixture.");
   start_local_game(child,base,client,folder);const auto state=base+game_state;const auto cards=child.read(state+0x1520);
   require(cards>2 && cards<=card_capacity,"Unexpected local fixture.");
   std::array<unsigned,18> actual{};
   auto measure=[&](DWORD type){const auto slot=pile_slot(child,state,type);unsigned count=0,visited=0;auto handle=child.read(state+0x152C+slot*0x10);
    while(handle){const auto index=handle&0xFFFF;require(index<cards && visited++<cards,"Invalid automatic supply list.");const auto card=state+0x1A48+index*0x64;if(child.read(card+4)==type)++count;handle=child.read(card+0x5C);}return count;};
   for(unsigned i=0;i<(test.colonies?8u:6u);++i)actual[i]=measure(basic_types[i]);
   for(unsigned i=0;i<10;++i)actual[8+i]=measure(kingdom_types[i]);
   if(!automatic) {
    expected=actual;
   } else {
    const auto result=load_patch(child,dll,58);const bool ok=actual==expected && result>0 && !(result&0xFFFF0000);
    std::printf("basic automatic case=%zu colonies=%d both=%d result=%lu %s\n",c,int(test.colonies),int(test.both),result,ok?"PASS":"FAIL");
    for(unsigned i=0;i<8;++i)std::printf("  %s measured=%u expected=%u\n",basic_names[i],actual[i],expected[i]);
    if(!ok)++failures;std::fflush(stdout);
   }
  }
 }
 std::printf("Basic automatic cases=%zu failed=%u\n",std::size(cases),failures);std::wprintf(L"Scratch: %ls\n",root.c_str());return failures?1:0;
}

int capture_garbage_state(const fs::path& exe,const fs::path& dll) {
 auto name=L"DominionGarbageCapture_"+std::to_wstring(GetCurrentProcessId());HiddenDesktop desktop(name);
 const auto folder=fs::temp_directory_path()/name;require(fs::create_directory(folder),"Scratch exists.");
 const Probe probe{"twist_garbage_heap",0x17DA,{},Metric::hand,0,0,1,base_kingdom};write_profile(folder,probe,1,"setupData");
 Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
 const auto client=wait_for_client(child,base,folder);require(load_patch(child,dll)==0,"Patch initialization failed.");
 require(load_patch(child,dll,42)==0,"Native collection grant preparation failed.");start_local_game(child,base,client,folder);
 const auto state=base+game_state;
 std::printf("garbage G=%08lX client=%08lX active=%lX next=%lX client5068=%lX client7590=%lX\n",static_cast<DWORD>(state),static_cast<DWORD>(client),child.read(state+0x19CC),child.read(state+0x19D0),child.read(client+0x5068),child.read(client+0x7590));
 for(unsigned i=0;i<80;++i)std::printf("G+%04X=%08lX%s",0x20+4*i,child.read(state+0x20+4*i),i%4==3?"\n":" ");
 const auto events=child.read(state+0x19B8);require(events<=256,"Invalid pending event count.");
 std::printf("Pending events=%lu\n",events);
 for(unsigned event=0;event<(std::min)(events,DWORD(8));++event) {
  const auto record=state+0x3B568+event*0xA8;std::printf("event=%u\n",event);
  for(unsigned offset=0;offset<0xA8;offset+=4)std::printf(" +%02X=%08lX%s",offset,child.read(record+offset),(offset/4)%4==3?"\n":" ");std::puts("");
 }
 std::fflush(stdout);std::wprintf(L"Scratch: %ls\n",folder.c_str());return 0;
}

int check_garbage_automation(const fs::path& exe,const fs::path& dll) {
 auto name=L"DominionGarbageAutomation_"+std::to_wstring(GetCurrentProcessId());HiddenDesktop desktop(name);
 const auto root=fs::temp_directory_path()/name;require(fs::create_directory(root),"Scratch exists.");
 for(unsigned copies:{1u,2u}) {
 const auto folder=root/std::to_wstring(copies);require(fs::create_directory(folder),"Scratch exists.");
 const Probe probe{"twist_garbage_heap",0x17DA,{},Metric::hand,0,0,1,base_kingdom};write_profile(folder,probe,copies,"setupData");
 Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
 const auto client=wait_for_client(child,base,folder);require(load_patch(child,dll)==0,"Patch initialization failed.");
 require(load_patch(child,dll,42)==0,"Native collection grant preparation failed.");start_local_game(child,base,client,folder);
 require(load_patch(child,dll,0x80000000u|static_cast<DWORD>(client))==0,"Client fixture binding failed.");
 for(DWORD phase:{65u,63u,64u,61u}) {
  if(phase==63 || phase==64)Sleep(750);
  const auto result=load_patch(child,dll,phase);
  std::printf("Garbage automation copies=%u phase=%lu result=%lu %s\n",copies,phase,result,result?"FAIL":"PASS");std::fflush(stdout);
  require(result==0,"Garbage Heap native automation check failed.");
 }
 const auto state=base+game_state;
 const auto cards=child.read(state+0x1520);
 Sleep(300);
 const auto declined=load_patch(child,dll,66);
 std::printf("Garbage automation copies=%u declined=%lu\n",copies,declined);std::fflush(stdout);
 require(declined==copies,"Repeated Garbage Heap did not share the setting.");
 require(hand_count(child,state,cards,0)==5,"Declining Garbage Heap changed the hand.");
 const auto previous=child.read(state+0x19E0);
 const DWORD restart=9;child.write(client+0x5044,&restart,sizeof restart);
 const auto until=GetTickCount64()+10000;
 while(GetTickCount64()<until && child.alive() && (child.read(client+0x5044)!=0 || child.read(state+0x19E0)!=1))Sleep(50);
 require(child.alive() && child.read(client+0x5044)==0,"Native restart did not complete.");
 (void)previous;
 require(load_patch(child,dll,67)==1,"A new local game retained Never.");
 require(load_patch(child,dll,62)==0,"Explicit reset check failed.");
 }
 std::wprintf(L"Scratch: %ls\n",root.c_str());return 0;
}

int wmain(int argc,wchar_t** argv) {
 try {
  require(argc==4 || argc==5 || argc==6,"Usage: startup_effect_test <copy exe> <probe dll> <setupData|kingdomData> <patched 0|1> [case], or --basic-targets");
  const auto exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
  Handle working(CreateFileW(exe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
  Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
  require(working.valid() && original.valid() && !same_file(identify(working.value),identify(original.value)),"Separate test copy required.");verify_executable(working.value);
  if(argc==4) {
   const std::wstring_view mode(argv[3]);
   if(mode==L"--basic-targets")return run_basic_target_checks(exe,dll);
   if(mode==L"--garbage-capture")return capture_garbage_state(exe,dll);
   if(mode==L"--garbage-automation")return check_garbage_automation(exe,dll);
   if(mode==L"--basic-automatic")return run_basic_automatic_checks(exe,dll);
   throw std::runtime_error("Unknown test mode.");
  }
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
