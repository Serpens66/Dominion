// Native Clear Kingdom regression; no network sends or foreground input.
#define wmain background_main
#include "background_effect_test.cpp"
#undef wmain
#include <sstream>

DWORD clear_request(const Child& child,const fs::path& dll,DWORD request) {
    const auto local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
    require(local!=nullptr,"Cannot inspect probe DLL.");
    const auto proc=GetProcAddress(local,"RoundtripProbe");require(proc!=nullptr,"Missing probe export.");
    std::array<DWORD,512> args{};
    args[0]=reinterpret_cast<DWORD>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"));
    args[1]=reinterpret_cast<DWORD>(proc)-reinterpret_cast<DWORD>(local);FreeLibrary(local);args[2]=request;
    require((dll.native().size()+1)*2<sizeof(args)-12,"Probe path too long.");
    std::memcpy(args.data()+3,dll.c_str(),(dll.native().size()+1)*2);
    constexpr std::array<BYTE,23> code{0x53,0x8B,0x5C,0x24,0x08,0x8D,0x4B,0x0C,0x51,0xFF,0x13,
        0x03,0x43,0x04,0xFF,0x73,0x08,0xFF,0xD0,0x5B,0xC2,0x04,0};
    return child.query(code,args,30000);
}
std::vector<std::string> clear_outputs(const fs::path& path) {
    std::ifstream file(path);require(bool(file),"Missing Clear report.");
    std::vector<std::string> rows;std::string line;std::getline(file,line);
    while(std::getline(file,line)) {
        require(line.find("\tPASS\t")!=line.npos,"Native Clear comparison failed; see report.");
        const auto at=line.find("\tPASS\t");rows.push_back(line.substr(at+6));
    }
    require(rows.size()==27,"Incomplete Clear report.");return rows;
}
int wmain(int argc,wchar_t** argv) {
    try {
        require(argc==3,"Usage: clear_kingdom_test <separate Dominion.exe> <roundtrip_probe.dll>");
        const auto exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
        Handle working(CreateFileW(exe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
        Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
        require(working.valid() && original.valid() && !same_file(identify(working.value),identify(original.value)),"Separate test copy required.");
        verify_executable(working.value);
        auto name=L"DominionClear_"+std::to_wstring(GetCurrentProcessId());HiddenDesktop desktop(name);
        const auto root=fs::temp_directory_path()/name;require(fs::create_directory(root),"Scratch exists.");
        const auto report=dll.parent_path()/L"clear-kingdom.tsv";
        for(int hotseat=0;hotseat<2;++hotseat) {
            std::vector<std::string> reference;
            for(int mode:{0,3,4,1}) {
                const auto folder=root/(std::to_wstring(hotseat)+L"-"+std::to_wstring(mode));require(fs::create_directory(folder),"Scratch exists.");
                const Probe fixture{"twist_big_ideas",0,{5,6}};write_profile(folder,fixture,1);
                const auto profile=folder/L"profiles.xml";
                std::ifstream in(profile);std::string xml((std::istreambuf_iterator<char>(in)),{});in.close();
                if(!hotseat) {
                    const std::string from="<playerType>Local</playerType><playerName>Vergleich</playerName>";
                    const auto at=xml.find(from);require(at!=xml.npos,"Missing local player fixture.");
                    xml.replace(at,from.size(),"<playerType>AI</playerType><playerName>Vergleich</playerName>");
                }
                const auto end=xml.find("</profiles>");require(end!=xml.npos,"Malformed profile fixture.");
                xml.insert(end,"<bans>518</bans><localBans>526</localBans><saveSets><sets><name>Clear fixture</name><strKingdom>-k village,market -x twist_big_ideas</strKingdom><expansionsBitset>1</expansionsBitset></sets></saveSets>");
                std::ofstream out(profile,std::ios::binary|std::ios::trunc);out<<xml;out.close();
                {
                    Child child(exe,name);const auto base=child.image_base();redirect_save_folder(child,base,folder);child.resume();
                    const auto client=wait_for_client(child,base,folder);
                    if(mode) require(clear_request(child,dll,mode==3?11:mode==4?14:15)==0,"Patch installation failed.");
                    start_local_game(child,base,client,folder);
                    const auto result=clear_request(child,dll,(mode==3 || mode==4)?12:10);
                    std::printf("%s mode=%d clear_result=%lu\n",hotseat?"Weitergeben":"Singleplayer",mode,result);std::fflush(stdout);
                    require(result==0,"Native Clear checks failed.");
                    auto rows=clear_outputs(report);
                    if(!mode)reference=rows;else require(rows==reference,"Patch changed cleared kingdom content.");
                    fs::copy_file(report,folder/L"clear-kingdom.tsv");
                }
                if(mode==3 || mode==4) {
                    Child reopened(exe,name);const auto base=reopened.image_base();redirect_save_folder(reopened,base,folder);reopened.resume();
                    const auto client=wait_for_client(reopened,base,folder);start_local_game(reopened,base,client,folder);
                    require(clear_request(reopened,dll,13)==0,"Private status or cleared selection did not survive restart.");
                }
            }
        }
        std::wprintf(L"PASS: 216 native Clear comparisons, identical kingdom output, private persistence; scratch: %ls\n",root.c_str());return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"%s\n",error.what());return 1;}
}
