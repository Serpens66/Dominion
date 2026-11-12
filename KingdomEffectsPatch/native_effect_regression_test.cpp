// Isolated engine rechecks of the previously blocked native effects.
#define wmain background_main
#include "background_effect_test.cpp"
#undef wmain
#include "probe_request.hpp"

int wmain(int argc,wchar_t** argv) {
    try {
        require(argc==3 || argc==4,"Usage: native_effect_regression_test <separate Dominion.exe> <probe.dll> [--production|--exploratory]");
        const auto exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
        Handle working(CreateFileW(exe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
        Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
        require(working.valid() && original.valid() && !same_file(identify(working.value),identify(original.value)),"Separate test copy required.");
        verify_executable(working.value);
        auto name=L"DominionNativeEffects_"+std::to_wstring(GetCurrentProcessId());HiddenDesktop desktop(name);
        const auto root=fs::temp_directory_path()/name;require(fs::create_directory(root),"Scratch exists.");
        std::ofstream report(dll.parent_path()/L"native-effect-rechecks.tsv",std::ios::binary);
        report<<"case\tcopies\tinstalled\tvalue\treserve\tresult\n"<<std::unitbuf;
        require(argc==3 || std::wstring_view(argv[3])==L"--production" || std::wstring_view(argv[3])==L"--exploratory","Unknown test mode.");
        const bool production=argc==3 || std::wstring_view(argv[3])==L"--production";
        unsigned failed=0;
        for(int test=0;test<3;++test)for(unsigned copies=0;copies<4;++copies) {
            if(production && ((test==1 && copies>1) || (test==2 && copies>2)))continue;
            const auto folder=root/(std::to_wstring(test)+L"-"+std::to_wstring(copies));require(fs::create_directory(folder),"Scratch exists.");
            const Purchase allow{"start_allow_second_cube(pageant)",0};
            const Probe fixture{test?"start_with_cube(pageant)":"stamp_cost_2D_less(triumph)",test?0x14B5u:0x1423u,
                {5,3,1,0},test?Metric::project_cube:Metric::price,test?pageant:3101u,0,3,test?pageant_kingdom:triumph_kingdom,test==2?&allow:nullptr};
            write_profile(folder,fixture,copies);
            try {
                Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
                const auto client=wait_for_client(child,base,folder);
                // Only the audited caption call is patched: no start-validation bypass.
                require(clear_request(child,dll,production?1:32)==0,"Patch could not be installed.");
                start_local_game(child,base,client,folder);
                Sleep(350);require(child.alive(),"Game stopped after initial display.");
                const auto caption_calls=clear_request(child,dll,33);
                require(caption_calls!=0xFFFFFFFFu,"Localized debt captions invalid.");
                std::printf("caption calls=%lu\n",caption_calls);
                const auto state=base+game_state;const auto who=child.read(state+0x19CC);
                unsigned installed=0;for(unsigned i=0;i<32;++i)if(child.read(state+0x1280+i*0x14)==fixture.id)++installed;
                unsigned value=0,reserve=0;bool ok=installed==copies;
                if(!test){value=observed_price(child,base,state,who,3101);ok&=value==fixture.expected[copies];}
                else {
                    const auto location=pile_slot(child,state,pageant);
                    value=token_count(child,state,who,0xE01,false,location);
                    reserve=token_count(child,state,who,0xE01,false,0x3F1);
                    const unsigned expected_pool=test==2?2u:1u;
                    ok&=value==(std::min)(copies,expected_pool) && value+reserve==expected_pool;
                    const auto count=child.read(state+0x19A4);
                    for(DWORD i=0;i<count;++i){const auto token=state+0x152C8+i*0x20;if(child.read(token)==0xE01 && child.read(token+4)==who)ok&=child.read(token+0x14)<=expected_pool;}
                }
                report<<test<<'\t'<<copies<<'\t'<<installed<<'\t'<<value<<'\t'<<reserve<<'\t'<<(ok?"PASS":"DIFFERENT")<<'\n';
                std::printf("native case=%d copies=%u installed=%u value=%u reserve=%u %s\n",test,copies,installed,value,reserve,ok?"PASS":"DIFFERENT");
                if(!ok)++failed;
            }catch(const std::exception& error){report<<test<<'\t'<<copies<<"\t0\t0\t0\tSTART_FAILED\n";std::printf("native case=%d copies=%u %s\n",test,copies,error.what());++failed;}
            std::fflush(stdout);
        }
        std::wprintf(L"Native effect rechecks: failures=%u; scratch: %ls\n",failed,root.c_str());return failed?1:0;
    }catch(const std::exception& error){std::fprintf(stderr,"%s\n",error.what());return 1;}
}
