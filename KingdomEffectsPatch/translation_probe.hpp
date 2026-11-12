DWORD run_translation_probe() {
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    using Lookup=const DWORD*(__fastcall*)(const char*,DWORD);
    const auto lookup=reinterpret_cast<Lookup>(base+0x28C730);
    HMODULE self{};wchar_t path[32768]{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&run_translation_probe),&self);GetModuleFileNameW(self,path,32768);
    std::ofstream report(std::filesystem::path(path).parent_path()/L"translation-probe.tsv",std::ios::binary);
    for(const auto* key:{"dom_taptip_gallery_ban_mp_body","dom_taptip_gallery_ban_online_body"})for(DWORD language:{1u,2u}) {
        const auto* entry=lookup(key,language);if(!entry)return 501;
        report<<key<<'\t'<<language<<'\t';
        for(int field=0;field<3;++field) {
            const auto* text=reinterpret_cast<const char*>(entry[field]);
            if(text)report<<std::string_view(text,strnlen_s(text,4096));report<<'\t';
        }
        report<<std::hex<<entry[3]<<','<<entry[4]<<','<<entry[5]<<','<<entry[6]<<std::dec<<'\n';
    }
    return report.good()?0:502;
}

DWORD run_ban_hint_checks() {
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    using Lookup=const DWORD*(__fastcall*)(const char*,DWORD);
    using Assign=void*(__thiscall*)(void*,const char* const*);
    const auto lookup=reinterpret_cast<Lookup>(base+0x28C730);
    const auto assign=reinterpret_cast<Assign>(base+0x23D850);
    const char* keys[]{"dom_taptip_gallery_ban_mp_body","dom_taptip_gallery_ban_online_body"};
    std::array<std::array<std::byte,23>,4> metadata{};
    struct Retained {Assign assign;const char* value=nullptr;~Retained(){const char* empty=nullptr;assign(&value,&empty);}};
    std::array<Retained,4> retained{{{assign},{assign},{assign},{assign}}};
    std::array<std::string,4> old{};
    const auto* foreign=lookup("dom_ui_campaign_features",1);
    const auto foreign_before=std::string(reinterpret_cast<const char*>(foreign[0]));
    unsigned n=0;
    for(const auto* key:keys)for(DWORD language:{1u,2u}) {
        const auto* entry=lookup(key,language);if(!entry)return 503;
        std::memcpy(metadata[n].data(),reinterpret_cast<const std::byte*>(entry)+4,23);
        assign(&retained[n].value,reinterpret_cast<const char* const*>(entry));old[n]=retained[n].value;++n;
    }
    HWND owner=nullptr;EnumWindows(find_native_owner,reinterpret_cast<LPARAM>(&owner));
    const auto applied=kingdom::native_ui::apply_ban_hints(owner,base);
    using Status=kingdom::native_ui::BanHintStatus;
    if(applied.status!=Status::applied && applied.status!=Status::already_applied)return 510+static_cast<DWORD>(applied.status);
    if(kingdom::native_ui::apply_ban_hints(nullptr,base).status!=Status::wrong_thread)return 520;
    auto* translations=*reinterpret_cast<std::byte**>(base+0x107ABFC);
    auto& selected=*reinterpret_cast<DWORD*>(translations+0x1F8);const auto original_language=selected;
    bool ok=true;n=0;
    for(const auto* key:keys)for(DWORD language:{1u,2u}) {
        const auto* entry=lookup(key,language);
        ok&=std::strcmp(reinterpret_cast<const char*>(entry[0]),kingdom::native_ui::ban_hint_text(language))==0;
        ok&=std::memcmp(metadata[n].data(),reinterpret_cast<const std::byte*>(entry)+4,23)==0;
        ok&=old[n]==retained[n].value; // Previously retained UI references are still valid.
        selected=language;
        const auto* current=lookup(key,0);
        ok&=std::strcmp(reinterpret_cast<const char*>(current[0]),kingdom::native_ui::ban_hint_text(language))==0;++n;
    }
    selected=original_language;
    ok&=foreign_before==reinterpret_cast<const char*>(lookup("dom_ui_campaign_features",1)[0]);
    const auto repeated=kingdom::native_ui::apply_ban_hints(owner,base);
    ok&=repeated.status==Status::already_applied && repeated.changed==0;
    return ok?0:521;
}
