// Read-only official entitlement checks on an invisible disposable game client.
#define wmain background_main
#include "background_effect_test.cpp"
#undef wmain
#include "probe_request.hpp"

int wmain(int argc,wchar_t** argv) {
    try {
        require(argc==3,"target_access_native_test <separate Dominion.exe> <roundtrip_probe.dll>");
        const auto exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
        Handle working(CreateFileW(exe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
        Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
        require(working.valid() && original.valid() && !same_file(identify(working.value),identify(original.value)),"Separate test copy required.");
        verify_executable(working.value);
        auto name=L"DominionTargetAccess_"+std::to_wstring(GetCurrentProcessId());
        HiddenDesktop desktop(name);
        const auto folder=fs::temp_directory_path()/name;require(fs::create_directory(folder),"Scratch exists.");
        const Probe fixture{"twist_big_ideas",0x1774,{5,6}};
        write_profile(folder,fixture,0);
        Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
        wait_for_client(child,base,folder);
        const auto result=clear_request(child,dll,40);
        const auto hook=clear_request(child,dll,41);
        std::printf("Online preflight hook result: %lu (synthetic only, no network).\n",hook);
        std::printf("Native target access result: %lu; report beside probe DLL.\n",result);
        std::wprintf(L"Disposable profile: %ls\n",folder.c_str());
        return result || hook?1:0;
    }catch(const std::exception& error){std::fprintf(stderr,"%s\n",error.what());return 1;}
}
