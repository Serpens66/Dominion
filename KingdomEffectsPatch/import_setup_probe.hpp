#pragma once
// Developer-only parser/import checks. Include after patch.cpp.
#include <fstream>
#include <filesystem>

namespace import_setup_fixture {
inline std::byte* client_for_checks=nullptr;
struct ClientScope {
    std::byte** slot=reinterpret_cast<std::byte**>(reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr))+0x8C8D5C);
    std::byte* previous=*slot;
    ClientScope(){if(!previous && client_for_checks)*slot=client_for_checks;}
    ~ClientScope(){*slot=previous;}
};

inline DWORD word(const kingdom::Setup& setup,std::size_t at) {DWORD value=0;std::memcpy(&value,setup.data()+at,4);return value;}
inline unsigned count_main(const kingdom::Setup& setup) {unsigned n=0;for(unsigned row=0;row<10;++row)if(word(setup,row*16))++n;return n;}
inline bool has_main(const kingdom::Setup& setup,DWORD id) {for(unsigned row=0;row<10;++row)if(word(setup,row*16)==1 && word(setup,row*16+4)==id)return true;return false;}
inline DWORD card_id(const char* id) {const auto* card=kingdom::access_native_card(id);return card?card->native_id:0;}
inline void merge(bool patched,bool clipboard,void* config,void* current,const void* source) {
    const auto entry=patched?(clipboard?reinterpret_cast<std::uintptr_t>(&clipboard_merge_hook):reinterpret_cast<std::uintptr_t>(&search_merge_hook)):
        reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr))+0x1F6A90;
    __asm {
        push source
        mov edx,current
        mov ecx,config
        call entry
        add esp,4
    }
}
inline void initialize() {
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));image_base=reinterpret_cast<const std::byte*>(base);
    native_parser=base+0xDDBB0;native_setup_serializer=base+0xE0FF0;native_active_setup=base+0xE49D0;native_string_assign=base+0x23D850;
}
inline void initialize_config(std::byte* config,const kingdom::Setup& current,std::size_t context) {
    *reinterpret_cast<DWORD*>(config+0x0C)=0;*reinterpret_cast<DWORD*>(config+0x10)=0;*reinterpret_cast<DWORD*>(config+0x28)=23;
    auto& flags=*reinterpret_cast<DWORD*>(config+0x18);flags=(flags&~0x20u)|(context==2?0x20u:0u);
    NativeSetupText(current,23).write_to(config+0x30);
    const char* empty=nullptr;using Assign=void*(__thiscall*)(void*,const char* const*);reinterpret_cast<Assign>(native_string_assign)(config+0x34,&empty);
}
inline bool validate_path(bool patched,bool clipboard,const char* input) {
    if(!patched)return true; // Baseline must not run any new validation/helper.
    if(clipboard){begin_clipboard();return validate_clipboard(input);}
    return validate_search(input);
}
inline void finish_path(bool patched,bool clipboard) {if(patched && clipboard)end_clipboard();}
inline bool is_allowed(const kingdom::NativeCardAccess& access,const char* id) {
    const auto* card=kingdom::access_native_card(id);return card && access.query_native(card->native_id)==kingdom::CardAccess::allowed;
}
inline bool family_authorized(const kingdom::NativeCardAccess& access,DWORD family) {
    using Match=bool(__fastcall*)(DWORD,DWORD);
    const auto match=reinterpret_cast<Match>(reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr))+0xDB700);
    for(unsigned i=0;i<access.grant_count();++i)if(match(access.grants()[i],family))return true;
    return false;
}
inline std::filesystem::path report_path(const wchar_t* name) {
    HMODULE self{};wchar_t path[32768]{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&report_path),&self);GetModuleFileNameW(self,path,32768);
    return std::filesystem::path(path).parent_path()/name;
}
}

DWORD run_import_setup_merge_checks(bool patched) {
    using namespace import_setup_fixture;initialize();
    std::ofstream report(report_path(patched?L"import-setup-native.tsv":L"import-setup-vanilla.tsv"),std::ios::binary);if(!report)return 821;
    report<<std::unitbuf;
    report<<"case\tcontext\tpath\tcurrent_options\tsaved_options\tresult\n"<<std::unitbuf;
    auto* profile=const_cast<std::byte*>(active_profile());if(!profile)return 822;
    const auto access=kingdom::NativeCardAccess::capture(image_base);if(!access.known())return 823;
    using Parse=void(__fastcall*)(const char*,kingdom::Setup*);const auto parse=reinterpret_cast<Parse>(native_parser);
    struct Restore {bool suppress=test_suppress_access_messages;kingdom::CardAccessQuery query=test_card_access;~Restore(){test_suppress_access_messages=suppress;test_card_access=std::move(query);pending={};}} restore;
    test_suppress_access_messages=true;test_card_access={};
    unsigned failures=0,checks=0;const std::size_t offsets[]{0x10,0x11B8,0x2360};
    const auto record=[&](const std::string& name,std::size_t context,bool clipboard,bool ok,const kingdom::Setup* current=nullptr,const kingdom::Setup* saved=nullptr) {
        report<<name<<'\t'<<context<<'\t'<<(clipboard?"clipboard":"search")<<'\t';
        for(const auto* setup:{current,saved}){if(setup)report<<word(*setup,0xBFC)<<','<<word(*setup,0xC00)<<','<<word(*setup,0xBC8)<<','<<word(*setup,0xBCC);report<<'\t';}
        report<<(ok?"PASS":"FAIL")<<'\n';++checks;if(!ok)++failures;
    };
    // Expected patched options are independently declared, not inferred with
    // the production presence scanner. Vanilla retains the initial values.
    struct Case {const char* name;const char* initial;const char* input;DWORD colonies,shelters,lo,hi;unsigned effects;bool exact_payload=false;};
    const Case cases[]{
      {"omitted","-e base -k village -m NoColonies,NoShelters,Landscapes(1,3)","-k market",2,2,1,3,0},
      {"colonies_off","-e base -k village -m NoShelters,Landscapes(1,3)","-k market -m NoColonies",2,2,1,3,0},
      {"shelters_off","-e base -k village -m NoColonies,Landscapes(1,3)","-k market -m NoShelters",2,2,1,3,0},
      {"quota_four","-e base -k village -m NoColonies,NoShelters,Landscapes(1,3)","-k market -m Landscapes(4,4)",2,2,4,4,0},
      {"quota_zero","-e base -k village -m NoColonies,NoShelters,Landscapes(1,3)","-k market -m Landscapes(0,0)",2,2,0,0,0},
      {"effects","-e base -k village -x start_double_pile(village)","-k market -x start_double_pile(market)",0,0,0,2,2},
      {"ten_base_control_off","-e base -m Colonies,NoShelters,Landscapes(1,3)","-k cellar,chapel,moat,harbinger,merchant,vassal,village,workshop,bureaucrat,gardens -m NoColonies,NoShelters,Landscapes(0,0)",2,2,0,0,0,true}
    };
    const auto run=[&](const Case& test,std::size_t context,bool clipboard) {
        auto* config=profile+offsets[context];kingdom::Setup current{},incoming{};parse(test.initial,&current);parse(test.input,&incoming);const auto initial=current;
        initialize_config(config,current,context);const auto before_privacy=*reinterpret_cast<DWORD*>(config+0x18)&0x20u;
        const bool valid=validate_path(patched,clipboard,test.input);if(valid)merge(patched,clipboard,config,&current,&incoming);finish_path(patched,clipboard);
        const auto saved=resolve_config(config);
        const DWORD expected[]{patched?test.colonies:word(initial,0xBFC),patched?test.shelters:word(initial,0xC00),patched?test.lo:word(initial,0xBC8),patched?test.hi:word(initial,0xBCC)};
        bool ok=valid;
        const std::size_t fields[]{0xBFC,0xC00,0xBC8,0xBCC};
        for(unsigned n=0;n<4;++n){
            const auto saved_expected=!patched && n==0 && !family_authorized(access,6)?0u:
                !patched && n==1 && !family_authorized(access,9)?0u:expected[n];
            ok&=word(current,fields[n])==expected[n] && word(saved,fields[n])==saved_expected;
        }
        // Fixed imports must not alter the enabled random-expansion pool.
        ok&=std::memcmp(current.data()+0x9C8,initial.data()+0x9C8,0x200)==0;
        ok&=std::memcmp(saved.data()+0x9C8,initial.data()+0x9C8,0x200)==0;
        if(test.exact_payload)ok&=std::memcmp(current.data(),incoming.data(),0x870)==0;
        else ok&=count_main(current)==2 && has_main(current,card_id("village")) && has_main(current,card_id("market"));
        const auto expected_effects=patched?test.effects:(test.effects?1u:0u);
        ok&=kingdom::count(kingdom::read_effects(current.data()))==expected_effects && (*reinterpret_cast<DWORD*>(config+0x18)&0x20u)==before_privacy;
        record(test.name,context,clipboard,ok,&current,&saved);
    };
    for(std::size_t context=0;context<3;++context)for(bool clipboard:{false,true}) {
        // A pre-existing Colonies-on setting is used only with official access.
        for(const auto& test:cases)if(std::string_view(test.name)!="ten_base_control_off" || (is_allowed(access,"colony")&&is_allowed(access,"platinum")))run(test,context,clipboard);
        if(is_allowed(access,"colony") && is_allowed(access,"platinum")) {
            run({"colonies_on_k","-e base -k village -m NoColonies,NoShelters,Landscapes(1,3)","-k market,colonies",1,2,1,3,0},context,clipboard);
            run({"colonies_on_m","-e base -k village -m NoColonies,NoShelters,Landscapes(1,3)","-k market -m Colonies",1,2,1,3,0},context,clipboard);
            run({"colonies_on_preserved","-e base -k village -m Colonies,NoShelters,Landscapes(1,3)","-k market",1,2,1,3,0},context,clipboard);
            run({"ten_base_colonies_control","-e base,prosperity -m NoColonies,NoShelters,Landscapes(1,3)","-k cellar,chapel,moat,harbinger,merchant,vassal,village,workshop,bureaucrat,gardens,colonies -e base,prosperity -m Landscapes(0,0),NoShelters",1,2,0,0,0,true},context,clipboard);
        }
        if(is_allowed(access,"necropolis") && is_allowed(access,"hovel") && is_allowed(access,"overgrown_estate")) {
            run({"shelters_on_k","-e base -k village -m NoColonies,NoShelters,Landscapes(1,3)","-k market,shelters",2,1,1,3,0},context,clipboard);
            run({"shelters_on_m","-e base -k village -m NoColonies,NoShelters,Landscapes(1,3)","-k market -m Shelters",2,1,1,3,0},context,clipboard);
            run({"shelters_on_preserved","-e base -k village -m NoColonies,Shelters,Landscapes(1,3)","-k market",2,1,1,3,0},context,clipboard);
            run({"shelters_off_from_on","-e base -k village -m NoColonies,Shelters,Landscapes(1,3)","-k market -m NoShelters",2,2,1,3,0},context,clipboard);
        }
    }
    // A fixed, authorized expansion card works without adding it to -e.
    // Check both initially disabled and already-enabled random-pool states.
    const std::pair<const char*,const char*> expansion_cards[]{{"minion","intrigue"},{"anvil","prosperity"},{"lighthouse","seaside"},{"hostelry","menagerie"},{"pixie","nocturne"},{"scrying_pool","alchemy"}};
    bool expansion_tested=false;
    for(const auto& test:expansion_cards)if(is_allowed(access,test.first)) {
        expansion_tested=true;
        for(std::size_t context=0;context<3;++context)for(bool clipboard:{false,true})for(bool already:{false,true}) {
            auto* config=profile+offsets[context];kingdom::Setup current{},incoming{};
            const auto initial=std::string("-e base")+(already?std::string(",")+test.second:"");const auto input=std::string("-k ")+test.first;parse(initial.c_str(),&current);parse(input.c_str(),&incoming);const auto before=current;initialize_config(config,current,context);
            const bool valid=validate_path(patched,clipboard,input.c_str());if(valid)merge(patched,clipboard,config,&current,&incoming);finish_path(patched,clipboard);
            const auto saved=resolve_config(config);
            record(std::string("fixed_owned_card_pool_")+(already?"on":"off"),context,clipboard,valid && has_main(current,card_id(test.first)) && has_main(saved,card_id(test.first)) &&
                std::memcmp(current.data()+0x9C8,before.data()+0x9C8,0x200)==0 && std::memcmp(saved.data()+0x9C8,before.data()+0x9C8,0x200)==0,&current,&saved);
        }
        break;
    }
    if(!expansion_tested)report<<"fixed_owned_card_pool\t0\tnative\t\t\tSKIP_NO_OWNED_NONBASE_CARD\n";
    // Assignment payloads use only naturally authorized fixture cards.
    struct Assignment {const char* input;const char* needed[4];};
    const Assignment assignments[]{
        {"-k young_witch(village)",{"young_witch","village",nullptr,nullptr}},
        {"-k druid(the_moons_gift,the_suns_gift,the_mountains_gift)",{"druid","the_moons_gift","the_suns_gift","the_mountains_gift"}},
        {"-k way_of_the_mouse(village)",{"way_of_the_mouse","village",nullptr,nullptr}},
        {"-k ferryman(shop)",{"ferryman","shop",nullptr,nullptr}},
        {"-k riverboat(imperial_envoy)",{"riverboat","imperial_envoy",nullptr,nullptr}}
    };
    for(const auto& test:assignments) {
        bool authorized=true;for(const auto* id:test.needed)if(id && !is_allowed(access,id))authorized=false;
        if(!authorized){report<<test.input<<"\t0\tnative\t\t\tSKIP_UNAVAILABLE_ASSIGNMENT\n";continue;}
        for(bool clipboard:{false,true}) {
            auto* config=profile+0x10;kingdom::Setup current{},incoming{};parse("-e base",&current);parse(test.input,&incoming);const auto initial=current;initialize_config(config,current,0);
            const bool valid=validate_path(patched,clipboard,test.input);if(valid)merge(patched,clipboard,config,&current,&incoming);finish_path(patched,clipboard);
            const auto saved=resolve_config(config);
            record(test.input,0,clipboard,valid && std::memcmp(current.data(),incoming.data(),0x870)==0 && std::memcmp(saved.data(),incoming.data(),0x870)==0 &&
                std::memcmp(current.data()+0x9C8,initial.data()+0x9C8,0x200)==0,&current,&saved);
        }
    }
    if(patched) {
        // Semantic-only options preflight for the complete user example. The
        // provider fixture is scoped to a copied Setup; native filter/save are
        // never called and no account/profile grant is fabricated.
        const char* sample="-k anvil,library,vault,grand_market,hostelry,cavalry,cellar,laboratory,harbinger,sanctuary,way_of_the_rat,way_of_the_worm,summon,delay,colonies -m Landscapes(4,4),NoShelters";
        std::array<std::byte,0x7868> before_profile{};std::memcpy(before_profile.data(),profile,before_profile.size());
        kingdom::Setup current{},incoming{};parse("-e base",&current);parse(sample,&incoming);const auto before=current;
        test_card_access=[](std::string_view){return kingdom::CardAccess::allowed;};
        const bool valid=validate_search(sample) && prepare_merge(current.data(),incoming.data());
        record("sample_options_preflight",0,false,valid && word(current,0xBFC)==1 && word(current,0xC00)==2 && word(current,0xBC8)==4 && word(current,0xBCC)==4 &&
            std::memcmp(current.data()+0x9C8,before.data()+0x9C8,0x200)==0 && std::memcmp(current.data(),before.data(),0x9C8)==0 && std::memcmp(before_profile.data(),profile,before_profile.size())==0,&current,nullptr);
        pending={};test_card_access={};
        const kingdom::TargetAccessNativeCard* denied=nullptr;for(const auto& card:kingdom::access_catalog::cards)if(card.native_id>>8>=2 && access.query_native(card.native_id)==kingdom::CardAccess::denied){denied=&card;break;}
        if(denied)for(bool clipboard:{false,true}) {
            const auto input=std::string("-k ")+denied->id;std::array<std::byte,0x7868> profile_before{};std::memcpy(profile_before.data(),profile,profile_before.size());
            const bool blocked=!validate_path(true,clipboard,input.c_str());finish_path(true,clipboard);
            record("unowned_main_rejected",0,clipboard,blocked && std::memcmp(profile_before.data(),profile,profile_before.size())==0);
        }
        test_card_access=[](std::string_view){return kingdom::CardAccess::unknown;};
        {std::array<std::byte,0x7868> profile_before{};std::memcpy(profile_before.data(),profile,profile_before.size());record("unknown_access_rejected",0,false,!validate_search("-k village") && std::memcmp(profile_before.data(),profile,profile_before.size())==0);}
        test_card_access={};
    }
    report<<"summary\tchecks="<<checks<<"\tfailures="<<failures<<"\t\t\t"<<(failures?"FAIL":"PASS")<<'\n';return failures;
}

DWORD prepare_import_setup_board_check() {
    using namespace import_setup_fixture;initialize();auto* profile=const_cast<std::byte*>(active_profile());if(!profile)return 831;
    const auto access=kingdom::NativeCardAccess::capture(image_base);if(!access.known())return 832;
    const bool colonies=is_allowed(access,"colony") && is_allowed(access,"platinum");
    const char* initial=colonies?"-e base,prosperity -m NoColonies,NoShelters":"-e base -m NoColonies,NoShelters";
    const auto input=std::string("-k cellar,chapel,moat,harbinger,merchant,vassal,village,workshop,bureaucrat,gardens")+(colonies?",colonies":"")+" -m Landscapes(0,0),NoShelters";
    using Parse=void(__fastcall*)(const char*,kingdom::Setup*);const auto parse=reinterpret_cast<Parse>(native_parser);
    kingdom::Setup current{},incoming{};parse(initial,&current);parse(input.c_str(),&incoming);auto* config=profile+0x10;initialize_config(config,current,0);
    if(!validate_search(input.c_str()))return 833;merge(true,false,config,&current,&incoming);
    const auto saved=resolve_config(config);if(count_main(saved)!=10 || word(saved,0xBFC)!=(colonies?1u:2u) || word(saved,0xC00)!=2 || word(saved,0xBC8) || word(saved,0xBCC))return 834;
    return colonies?1u:0u;
}

DWORD run_import_setup_checks(std::uintptr_t request) {
    import_setup_fixture::ClientScope native_client_context;
    if(request==55)return install_patch(reinterpret_cast<void*>(2));
    if(request==51 || request==52)return run_import_setup_merge_checks(request==51);
    if(request==53)return prepare_import_setup_board_check();
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    image_base=reinterpret_cast<const std::byte*>(base);
    native_parser=base+0xDDBB0;native_setup_serializer=base+0xE0FF0;
    native_active_setup=base+0xE49D0;native_string_assign=base+0x23D850;
    HMODULE self{};wchar_t path[32768]{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&run_import_setup_checks),&self);
    GetModuleFileNameW(self,path,32768);
    std::ofstream report(std::filesystem::path(path).parent_path()/L"import-setup-inspect.tsv",std::ios::binary);
    if(!report)return 801;
    report<<"case\toffset\tvalue\tbaseline\n"<<std::unitbuf;
    using Parse=void(__fastcall*)(const char*,kingdom::Setup*);
    const auto parse=reinterpret_cast<Parse>(native_parser);
    kingdom::Setup baseline{};parse("",&baseline);
    const char* cases[]{"", "-e base", "-e prosperity", "-e base,prosperity", "-e menagerie", "-e promos",
        "-k village", "-k lighthouse", "-k young_witch(village)", "-k druid(the_moons_gift,the_suns_gift,the_mountains_gift)",
        "-k ferryman(shop)", "-k riverboat(imperial_envoy)", "-k way_of_the_mouse(village)","-k colonies", "-k shelters", "-m NoColonies,NoShelters", "-m Colonies,Shelters",
        "-m Landscapes(4,4)", "-m Landscapes(1,3)",
        "-k anvil,library,vault,grand_market,hostelry,cavalry,cellar,laboratory,harbinger,sanctuary,way_of_the_rat,way_of_the_worm,summon,delay,colonies -m Landscapes(4,4),NoShelters"};
    for(unsigned n=0;n<std::size(cases);++n){
        kingdom::Setup setup{};parse(cases[n],&setup);
        report<<n<<"\ttext\t"<<cases[n]<<"\t\n";
        const NativeSetupText text(setup,23);report<<n<<"\tserialized\t"<<text.view()<<"\t\n";
        for(std::size_t at=0;at<setup.size();at+=4){
            DWORD current=0,initial=0;std::memcpy(&current,setup.data()+at,4);std::memcpy(&initial,baseline.data()+at,4);
            if((n==0 && current) || current!=initial)report<<n<<'\t'<<std::hex<<at<<'\t'<<current<<'\t'<<initial<<std::dec<<'\n';
        }
    }
    using CardDef=const DWORD*(__fastcall*)(DWORD,DWORD);
    const auto lookup=reinterpret_cast<CardDef>(base+0x171B30);
    for(const auto* name:{"summon","captain","church","black_market","marchland","harbinger","village","adventurer"}){
        const auto* card=kingdom::access_native_card(name);
        if(!card){report<<"carddef\t"<<name<<"\tmissing\n";continue;}
        const auto* definition=lookup(card->native_id,24);
        report<<"carddef\t"<<name<<"\t"<<std::hex<<card->native_id<<'\t'<<(definition?definition[0]:0xFFFFFFFFu)<<std::dec<<'\n';
    }
    return request==50?0:802;
}
