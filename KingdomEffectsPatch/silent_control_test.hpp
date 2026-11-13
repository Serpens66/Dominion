#pragma once
// Test-only: suppress and count MessageBeep imported by the actual tree-view
// module in THIS process. No sound settings, files or other processes change.
#include <atomic>
#include <vector>
#include <stdexcept>

namespace silent_control_test {
inline std::atomic<unsigned> blocked{0};
inline BOOL WINAPI quiet_beep(UINT) { ++blocked;return TRUE; }
struct Hook {
    struct Slot {void** address;void* original;};
    std::vector<Slot> slots;
    std::vector<HMODULE> modules;
    ~Hook(){restore();}
    void restore() noexcept {
        for(const auto& slot:slots){
            DWORD old=0;
            if(VirtualProtect(slot.address,sizeof(void*),PAGE_READWRITE,&old)){
                InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(slot.address),slot.original,reinterpret_cast<void*>(quiet_beep));
                DWORD ignored=0;VirtualProtect(slot.address,sizeof(void*),old,&ignored);
            }
        }
        slots.clear();modules.clear();
    }
    void attach(HWND tree){
        HMODULE module{};
        const auto procedure=reinterpret_cast<LPCWSTR>(GetClassLongPtrW(tree,GCLP_WNDPROC));
        if(!procedure || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,procedure,&module))
            throw std::runtime_error("Cannot identify native control module for silent tests.");
        for(auto existing:modules)if(existing==module)return;
        const auto* base=reinterpret_cast<const BYTE*>(module);
        const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<=0 || dos->e_lfanew>4096)
            throw std::runtime_error("Invalid control-module header.");
        const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            throw std::runtime_error("Unexpected control-module architecture.");
        const auto size=nt->OptionalHeader.SizeOfImage;
        const auto imports=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if(!imports.VirtualAddress || imports.VirtualAddress>=size || imports.Size>size-imports.VirtualAddress)
            throw std::runtime_error("Invalid control-module imports.");
        const auto beep=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"MessageBeep"));
        if(!beep)throw std::runtime_error("Cannot resolve test alert API.");
        const auto before=slots.size();
        const auto* descriptors=reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base+imports.VirtualAddress);
        for(unsigned i=0;i<imports.Size/sizeof(IMAGE_IMPORT_DESCRIPTOR) && descriptors[i].Name;++i){
            const auto offset=descriptors[i].FirstThunk;
            if(!offset || offset>=size)continue;
            auto* thunks=reinterpret_cast<IMAGE_THUNK_DATA32*>(const_cast<BYTE*>(base)+offset);
            for(unsigned n=0;n<(size-offset)/sizeof(IMAGE_THUNK_DATA32) && thunks[n].u1.Function;++n){
                auto** address=reinterpret_cast<void**>(&thunks[n].u1.Function);
                if(*address!=beep)continue;
                DWORD protection=0;
                if(!VirtualProtect(address,sizeof(void*),PAGE_READWRITE,&protection))throw std::runtime_error("Cannot silence control test alerts.");
                const auto previous=InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(address),reinterpret_cast<void*>(quiet_beep),beep);
                DWORD ignored=0;const bool protected_again=VirtualProtect(address,sizeof(void*),protection,&ignored)!=FALSE;
                if(previous==beep)slots.push_back({address,beep});
                if(previous!=beep || !protected_again)throw std::runtime_error("Cannot guard control test alerts safely.");
            }
        }
        if(slots.size()==before)throw std::runtime_error("Native control has no guardable MessageBeep import; noisy tests were not run.");
        modules.push_back(module);
    }
};
inline Hook guard;
}
