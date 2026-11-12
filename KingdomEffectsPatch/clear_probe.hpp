// Developer-only checks. Called on the isolated game's UI thread.
DWORD run_clear_checks(std::uintptr_t request) {
    const bool patched=request==12;
    image_base=reinterpret_cast<std::byte*>(GetModuleHandleW(nullptr));
    auto* profile=const_cast<std::byte*>(active_profile());
    if(!profile) return 301;
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    auto* const private_config=profile+0x2360;
    if(request==13) {
        const auto setup=resolve_config(private_config);
        const NativeSetupText text(setup,23);
        return (*reinterpret_cast<DWORD*>(private_config+0x18)&0x20) &&
            kingdom::count(kingdom::read_effects(setup.data()))==0 &&
            kingdom::fixed_kingdom_ids(text.view()).empty()?0:302;
    }
    wchar_t path[32768]{};HMODULE self{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&run_clear_checks),&self);GetModuleFileNameW(self,path,32768);
    std::ofstream report(std::filesystem::path(path).parent_path()/L"clear-kingdom.tsv",std::ios::binary);
    report<<"context\tcase\tstep\tflags\tresult\tkingdom\n"<<std::unitbuf;
    using Parse=void(__fastcall*)(const char*,kingdom::Setup*);
    using Clear=void(__thiscall*)(void*);
    using Assign=void*(__thiscall*)(void*,const char* const*);
    const auto clear=reinterpret_cast<Clear>(base+0x1F7090);
    const auto parse=reinterpret_cast<Parse>(native_parser);
    const auto assign=reinterpret_cast<Assign>(native_string_assign);
    const auto selector=reinterpret_cast<DWORD*>(base+0x4DBF38);
    const auto privacy=reinterpret_cast<DWORD*>(base+0x4DBF40);
    const auto old_selector=*selector,old_privacy=*privacy;
    const char* inputs[]{"-e base",
        "-k village,market,smithy,travelling_fair -x start_double_pile(village),twist_typhoon(militia,mine),twist_big_ideas",
        "-k village,market,smithy -x twist_big_ideas"};
    const std::size_t offsets[]{0x10,0x11B8,0x2360};
    unsigned failures=0;
    for(std::size_t context=0;context<3;++context) {
        *selector=context?1:0;*privacy=context==2?1:0;
        auto* const config=profile+offsets[context];
        for(std::size_t n=0;n<std::size(inputs);++n) {
            kingdom::Setup source{};parse(inputs[n],&source);
            const NativeSetupText initial(source,23);
            initial.write_to(config+0x30);
            const char* empty=nullptr;assign(config+0x34,&empty);
            *reinterpret_cast<DWORD*>(config+0x28)=23;
            // A loaded set is detached by native Clear before resolving its cards.
            *reinterpret_cast<DWORD*>(config+0x0C)=n==2?1:0;
            *reinterpret_cast<DWORD*>(config+0x10)=0;
            auto& flags=*reinterpret_cast<DWORD*>(config+0x18);
            flags=(flags&~0x30u)|0x10u|(context==2?0x20u:0u);
            const auto expected_flags=flags&~(patched?0x10u:0x30u);
            if(n==2) {
                const auto loaded=resolve_config(config);const NativeSetupText text(loaded,23);
                if(kingdom::fixed_kingdom_ids(text.view()).size()!=2 || kingdom::count(kingdom::read_effects(loaded.data()))!=1)
                    return 303;
            }
            std::array<std::byte,0x11A8> before{};std::memcpy(before.data(),config,before.size());
            std::array<std::byte,0x1900> protected_profile{};
            std::memcpy(protected_profile.data(),profile+0x58EC,protected_profile.size());
            const auto other1=std::string(NativeSetupText(resolve_config(profile+offsets[(context+1)%3]),23).view());
            const auto other2=std::string(NativeSetupText(resolve_config(profile+offsets[(context+2)%3]),23).view());
            for(int step=0;step<3;++step) {
                if(step==2) initial.write_to(config+0x30); // Refill, then clear again.
                clear(config);
                const auto cleared=resolve_config(config);const NativeSetupText text(cleared,23);
                bool ok=flags==expected_flags && *selector==(context?1u:0u) && *privacy==(context==2?1u:0u) &&
                    *reinterpret_cast<DWORD*>(config+0x0C)==0 && *reinterpret_cast<DWORD*>(config+0x10)==0 &&
                    kingdom::count(kingdom::read_effects(cleared.data()))==0 && kingdom::fixed_kingdom_ids(text.view()).empty() &&
                    std::memcmp(protected_profile.data(),profile+0x58EC,protected_profile.size())==0;
                // Empty native entries are identified by their ID; inactive payload bytes need not be zero.
                for(std::size_t i=0;i<4;++i) ok&=*reinterpret_cast<const DWORD*>(cleared.data()+0xA0+i*0x48)==0;
                for(std::size_t i=0;i<0x6B;++i) ok&=*reinterpret_cast<const DWORD*>(cleared.data()+0x1C0+i*0x10)==0;
                for(std::size_t i=0;i<6;++i) {
                    ok&=*reinterpret_cast<const DWORD*>(cleared.data()+0xBC8+i*8)==0;
                    ok&=*reinterpret_cast<const DWORD*>(cleared.data()+0xBCC+i*8)==(i==4?1u:2u);
                }
                for(std::size_t i=0;i<before.size();++i) {
                    if((i>=0x0C && i<0x14) || (i>=0x18 && i<0x1C) || (i>=0x30 && i<0x34)) continue;
                    ok&=before[i]==config[i]; // Player slots, time control and all other fields.
                }
                ok&=other1==NativeSetupText(resolve_config(profile+offsets[(context+1)%3]),23).view();
                ok&=other2==NativeSetupText(resolve_config(profile+offsets[(context+2)%3]),23).view();
                // Serialize the actual native online config builder, without sending it.
                if(context) {
                    using Build=const std::byte*(__thiscall*)(void*);
                    const auto* network=reinterpret_cast<Build>(base+0xB3B80)(config);
                    ok&=*reinterpret_cast<const DWORD*>(network+0x18)==expected_flags;
                }
                report<<context<<'\t'<<n<<'\t'<<step<<'\t'<<flags<<'\t'<<(ok?"PASS":"FAIL")<<'\t'<<text.view()<<'\n';
                if(!ok) ++failures;
            }
        }
    }
    *selector=old_selector;*privacy=old_privacy;
    return failures;
}
