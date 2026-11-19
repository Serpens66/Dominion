#pragma once
// Developer-only checks dispatched on the existing hidden game's UI thread.
inline DWORD run_optional_automation_checks(DWORD phase) {
    using namespace kingdom::optional_automation;
    import_setup_fixture::ClientScope client_scope;
    const auto image=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    auto* game=reinterpret_cast<std::byte*>(image+0x780AD8);
    auto* client=local_client();if(!client)return 901;
    using Defs=DWORD(__fastcall*)(DWORD,const Definition**);
    using Value=DWORD(__fastcall*)(DWORD);
    using Toggle=void(__fastcall*)(DWORD,DWORD);
    const auto defs=reinterpret_cast<Defs>(image+0x21CE00);
    const auto value=reinterpret_cast<Value>(image+0x21CEE0);
    const auto change=reinterpret_cast<Toggle>(image+0x1E3EC0);
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
        const auto pending=word(game,0x19B8);
        std::vector<std::byte> other_game(0x3B568+pending*0xA8);
        std::memcpy(other_game.data(),game,other_game.size());
        for(unsigned i=0;i<pending;++i)*reinterpret_cast<DWORD*>(other_game.data()+0x3B568+i*0xA8+0x54)=0;
        if(decline_garbage_heap(other_game.data(),action))return 917;
        std::memcpy(other_game.data(),game,other_game.size());
        for(unsigned i=0;i<pending;++i)*reinterpret_cast<DWORD*>(other_game.data()+0x3B568+i*0xA8+0x60)=0;
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
