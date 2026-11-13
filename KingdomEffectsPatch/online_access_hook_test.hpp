#pragma once
// Developer-only: include after patch.cpp. Exercises the actual naked hook
// without installing it or calling a network/create-game function.
#include <array>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <filesystem>

namespace online_access_fixture {
const std::byte* config=nullptr;
DWORD entry_esp=0,returned_esp=0,filter_esp=0,cancel_esp=0;
DWORD filter_calls=0,cancel_calls=0,filter_ecx=0,filter_edx=0,filter_count=0;

__declspec(naked) void native_filter_stub() {
    __asm {
        mov filter_ecx,ecx
        mov filter_edx,edx
        mov eax,esp
        mov filter_esp,eax
        mov eax,dword ptr [esp+4]
        mov filter_count,eax
        inc filter_calls
        ret
    }
}
__declspec(naked) void cancel_stub() {
    __asm {
        mov eax,esp
        mov cancel_esp,eax
        mov returned_esp,eax
        inc cancel_calls
        xor eax,eax
        ret
    }
}
__declspec(naked) void invoke_actual_hook() {
    __asm {
        mov eax,esp
        mov entry_esp,eax
        mov ecx,0x11223344
        mov edx,0x12345678
        mov eax,0x55667788
        // Match native caller 0x1FF9FF..0x1FFA03: alignment, count,
        // ECX=config, call. The stub retains both caller-owned words.
        push ecx
        push eax
        mov ecx,config
        call online_filter_hook
        add esp,8
        mov eax,esp
        mov returned_esp,eax
        ret
    }
}

struct Restore {
    std::uintptr_t filter=native_online_filter,cancel=create_cancel;
    bool measuring=measure_only,suppress=test_suppress_access_messages;
    kingdom::CardAccessQuery provider=test_card_access,editor_provider=kingdom::editor_card_access;
    ~Restore() {
        native_online_filter=filter;create_cancel=cancel;
        measure_only=measuring;test_suppress_access_messages=suppress;
        test_card_access=std::move(provider);kingdom::editor_card_access=std::move(editor_provider);
        config=nullptr;
    }
};
}

DWORD run_online_access_hook_checks() {
    using namespace online_access_fixture;
    Restore restore;
    native_online_filter=reinterpret_cast<std::uintptr_t>(&native_filter_stub);
    create_cancel=reinterpret_cast<std::uintptr_t>(&cancel_stub);
    measure_only=false;test_suppress_access_messages=true;
    const auto* profile=active_profile();
    if (!profile) return 741;
    std::array<std::byte,0x38> borrowed{};
    std::memcpy(borrowed.data(),profile+0x10,borrowed.size());
    *reinterpret_cast<DWORD*>(borrowed.data()+0x0C)=0;
    *reinterpret_cast<DWORD*>(borrowed.data()+0x28)=23;
    *reinterpret_cast<const char**>(borrowed.data()+0x30)="-x twist_typhoon(village,market)";
    *reinterpret_cast<const char**>(borrowed.data()+0x34)=nullptr;
    const auto original=borrowed;
    config=borrowed.data();
    std::array<std::byte,0x7868> profile_before{};
    std::memcpy(profile_before.data(),profile,profile_before.size());
    for (unsigned scenario=0;scenario<3;++scenario) {
        filter_calls=cancel_calls=entry_esp=returned_esp=filter_esp=cancel_esp=0;
        unsigned queries=0;
        test_card_access=[&](std::string_view id) {
            ++queries;
            if(id=="market" && scenario==1)return kingdom::CardAccess::denied;
            if(id=="market" && scenario==2)return kingdom::CardAccess::unknown;
            return kingdom::CardAccess::allowed;
        };
        invoke_actual_hook();
        if (!queries || entry_esp!=returned_esp || borrowed!=original) return 742+scenario;
        if (scenario==0) {
            if(filter_calls!=1 || cancel_calls || filter_esp!=entry_esp-12 ||
               filter_ecx!=reinterpret_cast<DWORD>(borrowed.data()) ||
               filter_edx!=0x12345678 || filter_count!=0x55667788) return 745;
        } else if (filter_calls || cancel_calls!=1 || cancel_esp!=entry_esp) return 746+scenario;
        if (std::memcmp(profile_before.data(),profile,profile_before.size())) return 749+scenario;
    }
    return 0;
}
