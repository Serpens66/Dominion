// Starts only the separate fixture on an invisible Windows desktop.
#define wmain background_main
#include "background_effect_test.cpp"
#undef wmain
#include "probe_request.hpp"
int wmain(int argc,wchar_t** argv) {
 try {
  require(argc==3,"checkbox_game_test <separate Dominion.exe> <checkbox_game_probe.dll>");
  const auto exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
  Handle working(CreateFileW(exe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
  Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
  require(working.valid()&&original.valid()&&!same_file(identify(working.value),identify(original.value)),"Separate test copy required.");verify_executable(working.value);
  auto name=L"DominionCheckbox_"+std::to_wstring(GetCurrentProcessId());HiddenDesktop desktop(name);
  const auto folder=fs::temp_directory_path()/name;require(fs::create_directory(folder),"Scratch exists.");
  const Probe probe{"twist_big_ideas",0,{5,5},Metric::hand,0,0,1,base_kingdom};write_profile(folder,probe,0);
  Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();wait_for_client(child,base,folder);
  const auto result=clear_request(child,dll,0);std::printf("Game-hosted checkbox result: %lu\n",result);std::wprintf(L"Disposable profile: %ls\n",folder.c_str());
  return result?1:0;
 }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
