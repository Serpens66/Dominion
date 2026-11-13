// Runs the same production apply function used by F8, without opening a window.
DWORD run_editor_apply_checks(std::uintptr_t request) {
    struct FixtureAccess { FixtureAccess(){test_card_access=[](std::string_view){return kingdom::CardAccess::allowed;};} ~FixtureAccess(){test_card_access={};} } fixture_access;
    image_base=reinterpret_cast<std::byte*>(GetModuleHandleW(nullptr));
    const auto base=reinterpret_cast<std::uintptr_t>(image_base);
    native_save_profiles=base+0xD8AD0;
    auto* profile=const_cast<std::byte*>(active_profile());if(!profile)return 401;
    const kingdom::EffectCommands final_effects{"twist_big_ideas"};
    if(request==21) {
        const auto* config=profile+0x2360;const auto current=resolve_config(config);
        const NativeSetupText text(current,23);
        return (*reinterpret_cast<const DWORD*>(config+0x18)&0x20) && parsed_effects_match(current,final_effects) &&
            kingdom::fixed_kingdom_ids(text.view()).size()==2?0:402;
    }
    HMODULE self{};wchar_t path[32768]{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&run_editor_apply_checks),&self);GetModuleFileNameW(self,path,32768);
    std::ofstream report(std::filesystem::path(path).parent_path()/L"editor-apply.tsv",std::ios::binary);
    report<<"context\tpreset\tstep\tresult\tflags\n"<<std::unitbuf;
    const std::size_t offsets[]{0x10,0x11B8,0x2360};
    auto* selector=reinterpret_cast<DWORD*>(base+0x4DBF38);
    auto* privacy=reinterpret_cast<DWORD*>(base+0x4DBF40);
    const auto old_selector=*selector,old_privacy=*privacy;
    using Parse=void(__fastcall*)(const char*,kingdom::Setup*);
    using Assign=void*(__thiscall*)(void*,const char* const*);
    using Build=const std::byte*(__thiscall*)(void*);
    unsigned failed=0;
    for(std::size_t context=0;context<3;++context)for(int preset=0;preset<2;++preset) {
        *selector=context?1:0;*privacy=context==2?1:0;
        auto* config=profile+offsets[context];kingdom::Setup source{};
        reinterpret_cast<Parse>(native_parser)("-k village,market -x twist_big_ideas",&source);
        NativeSetupText(source,23).write_to(config+0x30);
        const char* empty=nullptr;reinterpret_cast<Assign>(native_string_assign)(config+0x34,&empty);
        *reinterpret_cast<DWORD*>(config+0x28)=23;
        *reinterpret_cast<DWORD*>(config+0x0C)=preset?1:0;*reinterpret_cast<DWORD*>(config+0x10)=0;
        auto& flags=*reinterpret_cast<DWORD*>(config+0x18);
        flags=(flags&~0x30u)|0x10u|(context==2?0x20u:0u);
        const auto requested_flags=flags;
        auto current=resolve_config(config);const NativeSetupText initial(current,23);
        const auto cards=kingdom::fixed_kingdom_ids(initial.view());if(cards.size()!=2)return 403;
        const auto other1=std::string(NativeSetupText(resolve_config(profile+offsets[(context+1)%3]),23).view());
        const auto other2=std::string(NativeSetupText(resolve_config(profile+offsets[(context+2)%3]),23).view());
        std::array<std::byte,0x1900> protected_profile{};std::memcpy(protected_profile.data(),profile+0x58EC,protected_profile.size());
        // Native preset resolution may already clear its 0x10 bookkeeping bit before F8 opens.
        const auto initial_flags=flags;
        if((initial_flags&0x20)!=(requested_flags&0x20))return 406;
        kingdom::EffectCommands requested[13]{final_effects,
            {"start_double_pile(village)","start_double_pile(market)"},
            {"start_double_pile(market)","start_double_pile(village)","start_double_pile(village)"},
            {"twist_big_ideas","twist_typhoon(militia,mine)"}, {},
            kingdom::EffectCommands(32,"twist_big_ideas"),final_effects,
            kingdom::EffectCommands(33,"twist_big_ideas"),{"stamp_cost_2D_less(triumph"},
            {"start_with_cube(pageant)","start_with_cube(pageant)"},
            {"twist_not_a_real_effect"},{"start_double_pile(not_a_pile)"},
            {"start_double_pile(market)","twist_not_a_real_effect","twist_big_ideas"}};
        for(int step=0;step<13;++step) {
            std::array<std::byte,0x11A8> before{};std::memcpy(before.data(),config,before.size());
            const auto current_before=current;const NativeSetupText text_before(current,23);
            kingdom::EffectCommands original;if(!kingdom::read_effect_commands(text_before.view(),original))return 404;
            if(step==0) requested[0]=original;
            std::wstring error;const bool accepted=apply_editor_effects(&current,config,original,requested[step],error);
            const auto reloaded=resolve_config(config);const NativeSetupText text(reloaded,23);
            bool ok=accepted==(step<7) && *selector==(context?1u:0u) && *privacy==(context==2?1u:0u) &&
                flags==(step==0?initial_flags:initial_flags&~0x10u) &&
                kingdom::fixed_kingdom_ids(text.view())==cards &&
                std::memcmp(protected_profile.data(),profile+0x58EC,protected_profile.size())==0;
            if(step==0 || step>=7) ok&=std::memcmp(before.data(),config,before.size())==0 && current==current_before;
            if(step<7) ok&=parsed_effects_match(current,requested[step]) && parsed_effects_match(reloaded,requested[step]);
            else ok&=!error.empty() && parsed_effects_match(reloaded,final_effects);
            for(std::size_t i=0;i<before.size();++i) {
                if((i>=0x0C && i<0x14) || (i>=0x18 && i<0x1C) || (i>=0x30 && i<0x34))continue;
                ok&=before[i]==config[i];
            }
            ok&=other1==NativeSetupText(resolve_config(profile+offsets[(context+1)%3]),23).view();
            ok&=other2==NativeSetupText(resolve_config(profile+offsets[(context+2)%3]),23).view();
            if(context)ok&=*reinterpret_cast<const DWORD*>(reinterpret_cast<Build>(base+0xB3B80)(config)+0x18)==flags;
            report<<context<<'\t'<<preset<<'\t'<<step<<'\t'<<(ok?"PASS":"FAIL")<<'\t'<<flags<<'\n';
            if(!ok) report<<"# before_flags="<<*reinterpret_cast<const DWORD*>(before.data()+0x18)<<" initial_flags="<<initial_flags
                <<" accepted="<<accepted<<" same_commands="<<(original==requested[step])<<" same_config="<<(std::memcmp(before.data(),config,before.size())==0)
                <<" same_setup="<<(current==current_before)<<'\n';
            if(!ok)++failed;
        }
    }
    *selector=old_selector;*privacy=old_privacy;return failed;
}
