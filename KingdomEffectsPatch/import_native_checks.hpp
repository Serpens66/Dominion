#pragma once
// Reuses the established invisible-client test harness.
inline int run_import_native_checks(int argc,wchar_t** argv) {
    try {
        require(argc==3,"import_setup_native_test <separate Dominion.exe> <probe.dll>");
        const auto exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
        Handle working(CreateFileW(exe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
        Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
        require(working.valid()&&original.valid()&&!same_file(identify(working.value),identify(original.value)),"Separate test copy required.");verify_executable(working.value);
        auto name=L"DominionImportSetup_"+std::to_wstring(GetCurrentProcessId());HiddenDesktop desktop(name);
        const auto folder=fs::temp_directory_path()/name;require(fs::create_directory(folder),"Scratch exists.");
        const Probe fixture{"twist_big_ideas",0x1774,{5,6}};write_profile(folder,fixture,0);
        Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
        const auto client=wait_for_client(child,base,folder);
        require(!(client & 0x80000000u) && clear_request(child,dll,client | 0x80000000u)==0,"Native test client context could not be bound.");
        require(clear_request(child,dll,42)==0,"Native collection grant preparation failed.");
        const auto baseline=clear_request(child,dll,52);
        std::printf("Vanilla import comparison result: %lu\n",baseline);
        require(baseline==0,"Vanilla import baseline failed; see report.");
        const auto install=clear_request(child,dll,55);
        std::printf("Import fixture patch initialization: %lu\n",install);
        require(install==0,"Import patch initialization failed.");
        const auto result=clear_request(child,dll,51);
        std::printf("Native import setup result: vanilla=%lu patched=%lu; reports beside probe DLL.\n",baseline,result);
        require(result==0,"Patched import checks failed; see report.");
        const auto colonies_expected=clear_request(child,dll,53);
        require(colonies_expected<=1,"Could not prepare the import board check.");
        start_local_game(child,base,client,folder);
        const auto state=base+game_state;
        bool platinum=false,colony=false;
        for(unsigned slot=7;slot<0x48;++slot){const auto id=child.read(state+0x1524+slot*0x10);platinum|=id==1536;colony|=id==1537;}
        require(platinum==bool(colonies_expected) && colony==bool(colonies_expected),"Actual board did not follow the imported Colonies option.");
        const auto cards=child.read(state+0x1520),players=child.read(state+0xD38);
        require(players==2 && cards>2 && cards<=card_capacity,"Invalid board fixture.");
        for(unsigned player=0;player<players;++player){
            require(owned_count(child,state,cards,player,257)>=3,"NoShelters did not preserve the initial Estates.");
            for(const auto shelter:{2345u,2346u,2347u})require(!owned_count(child,state,cards,player,shelter),"NoShelters left a Shelter in a starting deck.");
        }
        std::printf("Actual board: Platinum/Colonies=%lu, initial Estates and no Shelters PASS.\n",colonies_expected);
        std::wprintf(L"Disposable profile: %ls\n",folder.c_str());return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"%s\n",error.what());return 1;}
}
