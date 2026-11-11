#define wmain background_main
#include "background_effect_test.cpp"
#undef wmain

int wmain(int argc,wchar_t** argv) {
    try {
        require(argc==3,"Usage: roundtrip_test <test Dominion.exe> <roundtrip_probe.dll>");
        const auto exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
        Handle working(CreateFileW(exe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
        Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
        require(working.valid() && original.valid() && !same_file(identify(working.value),identify(original.value)),"Separate test copy required.");
        verify_executable(working.value);
        auto name=L"DominionRoundtrip_"+std::to_wstring(GetCurrentProcessId());
        HiddenDesktop isolated(name);
        const auto root=fs::temp_directory_path()/name; require(fs::create_directory(root),"Scratch exists.");
        DWORD failures=0;
        for (DWORD phase=0;phase<2;++phase) {
        const auto folder=root/std::to_wstring(phase); require(fs::create_directory(folder),"Scratch exists.");
        const Probe regression{"twist_garbage_heap, twist_from_the_throne, twist_landfill, twist_so_much_stuff, twist_beakers, twist_big_ideas",0x17DA,{5,6}};
        write_profile(folder,regression,1);
        Child child(exe,name); const auto base=child.image_base(); redirect_save_folder(child,base,folder); child.resume();
        const auto client=wait_for_client(child,base,folder);
        const auto local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
        require(local!=nullptr,"Cannot inspect probe.");
        const auto proc=GetProcAddress(local,"RoundtripProbe"); require(proc!=nullptr,"Missing export.");
        std::array<DWORD,512> args{};
        args[0]=reinterpret_cast<DWORD>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"));
        args[1]=reinterpret_cast<DWORD>(proc)-reinterpret_cast<DWORD>(local); FreeLibrary(local);
        require((dll.native().size()+1)*2<sizeof(args)-12,"Path too long.");
        std::memcpy(args.data()+3,dll.c_str(),(dll.native().size()+1)*2);
        constexpr std::array<BYTE,23> code{0x53,0x8B,0x5C,0x24,0x08,0x8D,0x4B,0x0C,0x51,0xFF,0x13,
            0x03,0x43,0x04,0xFF,0x73,0x08,0xFF,0xD0,0x5B,0xC2,0x04,0};
        if (phase) { args[2]=1; require(child.query(code,args,30000)==0,"Test patch failed to initialize."); }
        // Native local setup initializes CardLookup; no user input is generated.
        start_local_game(child,base,client,folder);
        Sleep(600);
        constexpr std::array<DWORD,6> expected{0x17DA,0x17D4,0x1776,0x1778,0x17D6,0x1774};
        for(std::size_t i=0;i<expected.size();++i) {
            const auto actual=child.read(base+game_state+0x1280+i*0x14);
            std::printf("Installed regression effect %zu: %lX (expected %lX)\n",i+1,actual,expected[i]);
            require(actual==expected[i],"Actual local game lost an effect from the six-effect regression.");
        }
        args[2]=phase?2:0;
        const auto result=child.query(code,args,30000);
        std::printf("Native roundtrip result: %lu. Report beside probe DLL.\n",result);
        failures+=result;
        }
        return failures ? 1 : 0;
    } catch(const std::exception& error) { std::fprintf(stderr,"%s\n",error.what()); return 1; }
}
