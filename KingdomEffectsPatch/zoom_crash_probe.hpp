#pragma once
// Developer-only: inspect and invoke real zoom widgets on the isolated UI thread.
struct ZoomSnapshot {DWORD clone,source,location;};
inline std::vector<ZoomSnapshot> zoom_snapshots;
inline DWORD run_zoom_crash_probe(DWORD phase) {
    import_setup_fixture::ClientScope client_scope;
    const auto image=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const auto word=[](const void* pointer,std::size_t offset){DWORD value;std::memcpy(&value,static_cast<const std::byte*>(pointer)+offset,4);return value;};
    auto* objects=*reinterpret_cast<std::byte**>(image+0x7809E0);
    const auto count=*reinterpret_cast<DWORD*>(image+0x7809E4);
    if(!objects || !count || count>65535)return 970;
    std::ofstream report(import_setup_fixture::report_path(L"zoom-crash.tsv"),std::ios::app);
    report<<"phase\t"<<phase<<"\tzoom_mode\t"<<*reinterpret_cast<DWORD*>(image+0x77D424)<<'\n';
    const auto inspect=[&](){
        for(DWORD i=0;i<count;++i){const auto* object=objects+i*0x1C30;
            if(!(word(object,0x1C28)&0xFFFF0000) || word(object,0x2C)!=2)continue;
            const auto handle=word(object,0x24C),index=handle&0xFFFF;
            if(!handle || index>=count)continue;
            const auto* source=objects+index*0x1C30;
            if(word(source,0x1C28)!=handle)continue;
            report<<"icon\t"<<std::hex<<word(object,0x1C28)<<"\tsource\t"<<handle<<"\tkind\t"<<word(source,0x2C)
                <<"\tloc\t"<<word(source,0x370)<<"\tcard\t"<<word(source,0x30)<<"\tstamp\t"<<word(source,0x204)<<std::dec<<'\n';
        }
        report.flush();
    };
    if(phase==87)return static_cast<DWORD>(zoom_snapshots.size());
    if(phase==86){
        if(*reinterpret_cast<DWORD*>(image+0x77D424))return 972;
        for(const auto& saved:zoom_snapshots){
            if((saved.clone&0xFFFF)>=count || (saved.source&0xFFFF)>=count)return 975;
            const auto* clone=objects+(saved.clone&0xFFFF)*0x1C30;
            const auto* source=objects+(saved.source&0xFFFF)*0x1C30;
            if(word(clone,0x1C28)==saved.clone)return 973;
            if(word(source,0x1C28)!=saved.source || word(source,0x370)!=saved.location)return 974;
        }
        report<<"cleanup_verified\t"<<zoom_snapshots.size()<<'\n';return 0;
    }
    if(phase==83){
        zoom_snapshots.clear();
        for(DWORD i=0;i<count;++i){auto* object=objects+i*0x1C30;
            if(!(word(object,0x1C28)&0xFFFF0000) || word(object,0x2C)!=3 || word(object,0x30)!=0x213)continue;
            report<<"laboratory_source\t"<<std::hex<<word(object,0x1C28)<<"\tloc\t"<<word(object,0x370)<<std::dec<<'\n';report.flush();
            *reinterpret_cast<bool*>(image+0x77D418)=true;
            using Zoom=void(__fastcall*)(void*);reinterpret_cast<Zoom>(image+0x1CFFD0)(object);
            return 0;
        }
        return 971;
    }
    inspect();
    if(phase==84){zoom_snapshots.clear();for(DWORD i=0;i<count;++i){const auto* object=objects+i*0x1C30;
        if(!(word(object,0x1C28)&0xFFFF0000) || word(object,0x2C)!=2)continue;
        const auto handle=word(object,0x24C);if(!handle || (handle&0xFFFF)>=count)continue;
        const auto* source=objects+(handle&0xFFFF)*0x1C30;
        if(word(source,0x1C28)==handle)zoom_snapshots.push_back({word(object,0x1C28),handle,word(source,0x370)});
    }}
    if(phase==85){report<<"dismiss\n";report.flush();using Dismiss=void(*)();reinterpret_cast<Dismiss>(image+0x214CA0)();report<<"dismiss_completed\n";report.flush();}
    return 0;
}
