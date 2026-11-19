#pragma once
// Developer-only checks dispatched on the existing hidden game's UI thread.
inline DWORD run_optional_automation_checks(DWORD phase) {
    using namespace kingdom::optional_automation;
    import_setup_fixture::ClientScope client_scope;
    const auto image=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    auto* game=reinterpret_cast<std::byte*>(image+0x780AD8);
    if(phase==68){order_enabled=true;ordered_requests=0;order_auto_calls=order_kind6_calls=order_eligible_calls=order_origin_calls=0;order_frame_returns={};return 0;}
    auto* client=local_client();if(!client)return 901;
    using Defs=DWORD(__fastcall*)(DWORD,const Definition**);
    using Value=DWORD(__fastcall*)(DWORD);
    using Toggle=void(__fastcall*)(DWORD,DWORD);
    const auto defs=reinterpret_cast<Defs>(image+0x21CE00);
    const auto value=reinterpret_cast<Value>(image+0x21CEE0);
    const auto change=reinterpret_cast<Toggle>(image+0x1E3EC0);
    if(phase==82) {
        const auto count=word(game,0x19B8);if(count>256)return 960;
        for(unsigned i=0;i<count;++i){const auto* event=game+0x3B568+i*0xA8;
            if(word(event,0x54)!=image+0x113990)continue;
            // First Typhoon activation records the first selected pile for a
            // later turn. Native113680 writes this persistent event payload.
            const auto payload=word(event,0x78);
            return (payload>>16)==0xFFFF && (payload&0xFFFF)<14?0:961;
        }
        return 962;
    }
    if(phase==81) {
        std::ofstream report(import_setup_fixture::report_path(L"campaign-order-origin.tsv"));
        report<<"calls\tkind6\teligible\torigin\n"<<order_auto_calls<<'\t'<<order_kind6_calls<<'\t'<<order_eligible_calls<<'\t'<<order_origin_calls<<'\n';
        report<<"action\t"<<word(game+0x20,4)<<"\toptions\t"<<word(game+0x20,0x6C)<<'\n';
        for(const auto returned:order_frame_returns)report<<std::hex<<returned<<'\n';
        report<<"capture\n";for(unsigned i=0;i<order_capture.size();++i)report<<std::hex<<i<<'\t'<<order_capture[i]<<'\n';
        return 0;
    }
    if(phase==71)return ordered_requests;
    if(phase==77)return order_enabled?1:0;
    if(phase==74) {
        const auto* action=game+0x20;
        if(word(action,4)!=1 || word(action,0x54)!=3 || current(setting_id)!=ask || declined_requests)return 955;
        const auto* event=queued_event(game,word(action,0x58));
        if(!event || word(event,0x54)!=image+0x112BF0 || !word(action,0x6C))return 956;
        // This is the actual optional hand-card decision after the order was
        // accepted. The order preference must not answer it, even with an
        // otherwise valid source and a hypothetical valid order origin.
        return campaign_order_request(game,action,true) || automatic_response(game,action,0)?957:0;
    }
    if(phase==78) {
        std::vector<std::pair<std::string,DWORD>> registered;
        using Parse=void(__fastcall*)(const char*,kingdom::Setup*);
        const auto parse=reinterpret_cast<Parse>(native_parser);
        for(const auto& effect:fixture::effect_catalog) {
            std::string command=effect.id;
            if(effect.first!=fixture::TargetKind::none){command+='(';command+=fixture::target(effect.first);
                if(effect.second!=fixture::TargetKind::none){command+=',';command+=fixture::target(effect.second,true);}command+=')';}
            kingdom::Setup setup{};const auto text="-x "+command;parse(text.c_str(),&setup);
            const auto effects=kingdom::read_effects(setup.data());
            if(kingdom::count(effects)!=1)return 940;
            registered.emplace_back(effect.id,effects[0][0]);
        }
        using Props=const std::byte*(__fastcall*)(DWORD,DWORD);
        const auto props=reinterpret_cast<Props>(image+0x171B30);
        std::ofstream report(import_setup_fixture::report_path(L"campaign-order-sources.tsv"));
        report<<"source\tmetadata_lookup\tdeclared_display\teffect\tname\tresult\n";unsigned failed=0;
        for(std::size_t i=0;i<campaign_order_sources.size();++i) {
            const auto& source=campaign_order_sources[i];
            if(source.display!=0x1500+i)return 941;
            for(std::size_t j=0;j<i;++j)if(campaign_order_sources[j].effect==source.effect)return 942;
            // Display cards1530+ were introduced in the latest card schema24.
            // Kingdom text still uses its separately audited version23 codec.
            // The two native definitions are stored at each other's physical
            // lookup index. Validate exactly those aliases, but do not admit
            // either source to automatic ordering without event-source proof.
            const bool alias=source.display==0x1530 || source.display==0x1531;
            const auto lookup=alias?(source.display^1):source.display;
            const auto* properties=props(lookup,0x18);
            if(!properties)return 943;
            const auto returned_display=word(properties,0x8C);
            const auto* pointer=reinterpret_cast<const char*>(word(properties,0x58));
            std::string name;
            for(unsigned n=0;n<128;++n){char ch=0;if(!pointer || !kingdom::access_detail::copy_readable(&ch,pointer+n,1))return 944;if(!ch)break;name+=ch;}
            if(name=="stamp_treasureified")name="stamp_treasurified";
            bool matched=false;for(const auto& entry:registered)if(entry.first==name && entry.second==source.effect){matched=true;break;}
            if(alias && active_campaign_source(game,source.display))return 943;
            report<<std::hex<<source.display<<'\t'<<lookup<<'\t'<<returned_display<<'\t'<<source.effect<<'\t'<<name<<'\t'<<(matched && returned_display==source.display?"PASS":"FAIL")<<'\n';
            if(!matched || returned_display!=source.display)++failed;
        }
        return failed?945:0;
    }
    if(phase==79) {
        {
            auto* id=reinterpret_cast<DWORD*>(client+0x7590);const auto saved=*id;
            struct RestoreId {DWORD* pointer;DWORD value;~RestoreId(){*pointer=value;}} restore_id{id,saved};
            unsigned used=0;for(const auto& setting:players)used+=setting.used?1:0;
            *id=0xFFFFFFFF;
            if(player_setting() || current(setting_id)!=ask)return 958;
            unsigned after=0;for(const auto& setting:players)after+=setting.used?1:0;
            if(after!=used)return 959;
        }
        const auto previous=order_enabled;struct Restore {bool previous;~Restore(){order_enabled=previous;}} restore{previous};
        order_enabled=true;auto* action=game+0x20;
        if(!campaign_order_request(game,action,true))return 946;
        if(campaign_order_request(game,action,false) || native_order_origin(0))return 947;
        const auto count=word(game,0x19B8);
        std::vector<std::byte> copy_game(0x3B568+count*0xA8);
        std::array<std::byte,0xD10> copy_action{};
        std::memcpy(copy_game.data(),game,copy_game.size());std::memcpy(copy_action.data(),action,copy_action.size());
        const auto* original_event=queued_event(game,word(action,0x70));if(!original_event)return 948;
        const auto offset=original_event-game;auto* event=copy_game.data()+offset;
        *reinterpret_cast<DWORD*>(event+0x60)|=1;
        if(campaign_order_request(copy_game.data(),action,true))return 949;
        std::memcpy(copy_game.data(),game,copy_game.size());
        *reinterpret_cast<DWORD*>(event+0x24)=0x206;*reinterpret_cast<DWORD*>(event+0x28)=0;
        if(campaign_order_request(copy_game.data(),action,true))return 950;
        std::memcpy(copy_game.data(),game,copy_game.size());
        std::memset(copy_game.data()+0x1280,0,32*0x14);
        if(campaign_order_request(copy_game.data(),action,true))return 951;
        std::memcpy(copy_game.data(),game,copy_game.size());*reinterpret_cast<DWORD*>(event+0x54)=0;
        if(campaign_order_request(copy_game.data(),action,true))return 952;
        *reinterpret_cast<DWORD*>(copy_action.data()+4)=1;
        if(campaign_order_request(game,copy_action.data(),true))return 953;
        std::memcpy(copy_action.data(),action,copy_action.size());*reinterpret_cast<DWORD*>(copy_action.data()+0x70)|=0x10;
        if(campaign_order_request(game,copy_action.data(),true))return 954;
        return 0;
    }
    if(phase==69 || phase==70) {
        auto* action=game+0x20;const auto count=word(action,0x6C);
        if(word(action,4)!=6)return 1;
        if(!count || count>800)return 920;
        if(phase==70)*reinterpret_cast<DWORD*>(action+0x70)=word(action,0x70+(count-1)*4);
        *reinterpret_cast<DWORD*>(action+0x6C)=1;
        using Data=void*(*)();auto* data=reinterpret_cast<Data>(image+0xB9480)();
        const auto submit=image+0x2228C0;
        __asm {
            push action
            mov ecx,data
            call submit
            add esp,4
        }
        return 0;
    }
    if(phase==72) {
        auto* action=game+0x20;
        if(word(action,4)!=6 || word(action,0x6C)<2)return 921;
        // A real ordering prompt must remain present even with Never. Copy its
        // context and point it at the actual Garbage Heap request, without
        // submitting or modifying the live choice.
        const auto count=word(game,0x19B8);
        for(unsigned i=0;i<count && i<256;++i) {
            const auto* event=game+0x3B568+i*0xA8;
            if(word(event,0x54)!=image+0x112BF0)continue;
            std::array<std::byte,0xA0> copy{};std::memcpy(copy.data(),action,copy.size());
            *reinterpret_cast<DWORD*>(copy.data()+0x54)=3;
            *reinterpret_cast<DWORD*>(copy.data()+0x58)=word(event,0x2C)<<18;
            auto* setting=player_setting();if(!setting)return 928;
            const auto previous=setting->value;setting->value=never;
            const auto declined=decline_garbage_heap(game,copy.data());setting->value=previous;
            return declined?929:0;
        }
        return 923;
    }
    if(phase==73) {
        // The native dependency classifier already resolves these fixed
        // campaign bonuses before constructing an order prompt.
        std::array<DWORD,256> handles{};unsigned count=0;
        const auto pending_events=word(game,0x19B8);if(pending_events>256)return 924;
        for(unsigned i=0;i<pending_events;++i) {
            const auto* event=game+0x3B568+i*0xA8;
            const auto source=word(event,0x24),callback=word(event,0x54);
            if(!((source==0x1500 && callback==image+0xFA4E0) || (source==0x1501 && callback==image+0x1125F0)))continue;
            handles[count++]=word(event,0x2C)<<18;
        }
        if(count<2)return 925;
        const auto commute=image+0x17C2F0;
        for(unsigned i=0;i<count;++i)for(unsigned j=0;j<count;++j)if(i!=j) {
            const auto left=handles[i],right=handles[j];DWORD result=0;
            __asm {
                push right
                push left
                mov ecx,game
                call commute
                add esp,8
                mov result,eax
            }
            if(result==0 || result==3)return 926;
        }
        struct Choices {std::array<DWORD,256> values{};DWORD count=0;} choices;
        choices.values=handles;choices.count=count;
        const auto select=image+0x17F120;auto* list=&choices;
        const auto actor=word(game+0x20,0x4C);
        for(unsigned direction=0;direction<2;++direction) {
            if(direction)std::reverse(choices.values.begin(),choices.values.begin()+count);
            DWORD selected=0xFFFFFFFFu,result=0;auto* output=&selected;
            __asm {
                push output
                push list
                mov edx,actor
                mov ecx,game
                call select
                add esp,8
                mov result,eax
            }
            if(result!=1 || selected!=choices.values[0])return 927;
        }
        return 0;
    }
    static std::array<std::byte,0x204> saved_profile{},saved_active{};
    static bool saved_smartplays=false;
    if(phase==75 || phase==76) {
        using Get=std::byte*(__fastcall*)(DWORD);const auto get=reinterpret_cast<Get>(image+0x1E3C60);
        if(phase==75){std::memcpy(saved_profile.data(),get(0),saved_profile.size());std::memcpy(saved_active.data(),get(1),saved_active.size());saved_smartplays=true;return 0;}
        return saved_smartplays && !std::memcmp(saved_profile.data(),get(0),saved_profile.size()) &&
            !std::memcmp(saved_active.data(),get(1),saved_active.size())?0:932;
    }
    if(phase==65) {
        EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM)->BOOL {
            DWORD pid=0;GetWindowThreadProcessId(window,&pid);
            if(pid==GetCurrentProcessId() && !GetWindow(window,GW_OWNER)){
                ShowWindow(window,SW_SHOWNOACTIVATE);UpdateWindow(window);
            }
            return TRUE;
        },0);
        return 0;
    }
    if(phase==61) {
        const Definition* output[4]{};const Definition* baseline[4]{};
        if(defs(garbage_heap_card,output)!=1 || output[0]!=&definition)return 902;
        const auto regular=defs(0x20B,output),original=original_defs(0x20B,baseline);
        if(regular!=original || std::memcmp(output,baseline,regular*sizeof(void*)))return 903;
        if(value(setting_id)!=ask)return 904;
        using Get=std::byte*(__fastcall*)(DWORD);
        const auto native_get=reinterpret_cast<Get>(image+0x1E3C60);
        std::array<std::byte,0x204> profile{},active{};
        std::memcpy(profile.data(),native_get(0),profile.size());
        std::memcpy(active.data(),native_get(1),active.size());
        change(setting_id,1);
        if(value(setting_id)!=never)return 905;
        // The native "use as default" handler must not serialize the private ID.
        const auto save=image+0x1E3E50;const auto id=setting_id;
        __asm {
            push 2
            mov ecx,id
            call save
            add esp,4
        }
        if(std::memcmp(profile.data(),native_get(0),profile.size()) ||
           std::memcmp(active.data(),native_get(1),active.size()))return 906;
        // A different local participant retains Ask. Restore the fixture field
        // before any game processing; no real profile is changed.
        auto* who=reinterpret_cast<DWORD*>(client+0x7590);const auto previous=*who;
        *who=previous+1;const auto other=value(setting_id);*who=previous;
        if(other!=ask || value(setting_id)!=never)return 907;
        auto* action=game+0x20;
        if(!decline_garbage_heap(game,action))return 908;
        std::array<std::byte,0xA0> unrelated{};std::memcpy(unrelated.data(),action,unrelated.size());
        *reinterpret_cast<DWORD*>(unrelated.data()+0x54)=0;
        if(decline_garbage_heap(game,unrelated.data()))return 915;
        std::memcpy(unrelated.data(),action,unrelated.size());
        *reinterpret_cast<DWORD*>(unrelated.data()+0x58)=0xFFFF0000;
        if(decline_garbage_heap(game,unrelated.data()))return 916;
        const auto pending_events=word(game,0x19B8);
        std::vector<std::byte> other_game(0x3B568+pending_events*0xA8);
        std::memcpy(other_game.data(),game,other_game.size());
        for(unsigned i=0;i<pending_events;++i)*reinterpret_cast<DWORD*>(other_game.data()+0x3B568+i*0xA8+0x54)=0;
        if(decline_garbage_heap(other_game.data(),action))return 917;
        std::memcpy(other_game.data(),game,other_game.size());
        for(unsigned i=0;i<pending_events;++i)*reinterpret_cast<DWORD*>(other_game.data()+0x3B568+i*0xA8+0x60)=0;
        if(decline_garbage_heap(other_game.data(),action))return 918;
        declined_requests=0;
        const auto automatic=image+0x222940;
        __asm {
            mov ecx,game
            mov edx,action
            call automatic
        }
        if(!declined_requests)return 909;
        return 0;
    }
    if(phase==62) {reset();return value(setting_id)==ask && !decline_garbage_heap(game,game+0x20)?0:910;}
    if(phase==63) {
        // Find the real, already-rendered campaign banner object. Invoke its
        // native zoom action instead of moving or clicking any input device.
        auto* objects=*reinterpret_cast<std::byte**>(image+0x7809E0);
        const auto count=*reinterpret_cast<DWORD*>(image+0x7809E4);
        if(!objects || !count || count>65535)return 911;
        for(std::size_t i=0;i<count;++i) {
            auto* object=objects+i*0x1C30;
            const auto kind=word(object,0x2C);
            if(!word(object,0x1C28) || !((kind==1 && word(object,0x170)==garbage_heap_card) ||
                (kind==3 && word(object,0x30)==garbage_heap_card) ||
                (kind==5 && word(object,0x204)==garbage_heap_card)))continue;
            definition_queries=0;
            *reinterpret_cast<bool*>(image+0x77D418)=true;
            using Zoom=void(__fastcall*)(void*);
            reinterpret_cast<Zoom>(image+0x1CFFD0)(object);
            return 0;
        }
        std::ofstream report(import_setup_fixture::report_path(L"optional-automation-objects.txt"));
        report<<"count="<<count<<" session="<<word(client,0x506C)<<'\n';
        for(std::size_t i=0;i<count;++i) {
            auto* object=objects+i*0x1C30;
            if(word(object,0x1C28))report<<i<<" kind="<<word(object,0x2C)<<" pile="<<std::hex<<word(object,0x30)
                <<" zoom="<<word(object,0x170)<<" effect="<<word(object,0x204)<<std::dec<<'\n';
        }
        return 912;
    }
    if(phase==64) {
        const auto row=*reinterpret_cast<DWORD*>(image+0x168CA30);
        return definition_queries && row?0:913;
    }
    if(phase==66)return declined_requests;
    if(phase==67)return value(setting_id);
    return 914;
}
