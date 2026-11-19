#pragma once
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Build 3315 only. A private, process-local setting, deliberately outside the
// native Smartplay registry and profile/network serialization.
namespace kingdom::optional_automation {
inline constexpr DWORD garbage_heap=0x17DA;
inline constexpr DWORD garbage_heap_card=0x150E;
inline constexpr DWORD setting_id=0x4B455001;
inline constexpr DWORD ask=1, never=2;
struct Option {DWORD value;const char* label;DWORD flags;};
struct Definition {
    DWORD id;
    std::array<DWORD,32> cards;
    const char* title;
    const char* description;
    std::array<Option,8> options;
};
static_assert(sizeof(Definition)==0xEC);
inline Definition definition{setting_id,{garbage_heap_card},"twist_garbage_heap","twist_garbage_heap_desc",
    {{{ask,"dom_smartplay_ask",2},{never,"dom_smartplay_never",0}}}};
inline std::uintptr_t base=0;
inline std::uintptr_t defs_continue=0,default_continue=0,native_smartplay_get=0;
inline std::uintptr_t toggle_continue=0,save_continue=0;
inline DWORD session=0;
inline bool session_known=false;
struct PlayerSetting {DWORD id=0;DWORD value=ask;bool used=false;};
inline std::array<PlayerSetting,6> players{};
inline std::uintptr_t automatic_continue=0,automatic_empty=0;
inline std::uintptr_t load_continue=0;
#ifdef KINGDOM_EFFECT_TEXT_TEST
inline unsigned definition_queries=0,declined_requests=0;
#endif
inline void reset() {session=0;session_known=false;players={};}

inline DWORD word(const void* pointer,std::size_t offset) {
    DWORD value=0;std::memcpy(&value,static_cast<const std::byte*>(pointer)+offset,4);return value;
}
inline std::byte* local_client() {
    if(!base)return nullptr;
    auto* client=*reinterpret_cast<std::byte**>(base+0x8C8D5C);
    if(!client || word(client,0x5068)!=1)return nullptr;
    const auto current=word(client,0x506C);
    if(!session_known || current!=session){session=current;session_known=true;players={};}
    return client;
}
inline PlayerSetting* player_setting() {
    const auto* client=local_client();if(!client)return nullptr;
    const auto id=word(client,0x7590);
    for(auto& setting:players)if(setting.used && setting.id==id)return &setting;
    for(auto& setting:players)if(!setting.used){setting={id,ask,true};return &setting;}
    return nullptr;
}
inline DWORD __fastcall current(DWORD) {
    const auto* setting=player_setting();return setting?setting->value:ask;
}
inline void __fastcall toggle(DWORD,DWORD) {
    if(auto* setting=player_setting())setting->value=setting->value==ask?never:ask;
}

inline bool __cdecl decline_garbage_heap(const std::byte* game,const std::byte* action) {
    const auto* client=local_client();
    if(!client || !game || !action || current(setting_id)!=never)return false;
    // Native readiness, player-type and turn gates have already run at this
    // call site. Keep a second explicit local-player check here.
    if(word(action,0x4C)!=word(action,0x50) || word(action,0x50)!=word(client,0x7590))return false;
    if(word(action,0x54)!=3)return false;
    const auto handle=word(action,0x58);
    if(handle&0x30)return false;
    const auto count=word(game,0x19B8);
    if(!count || count>256)return false;
    const auto sequence=handle>>18;
    // Reproduce only the bounded, read-only lookup performed by native
    // GetEvent (RVA176940). No native assert on an unknown/stale handle.
    for(std::size_t i=count;i-->0;) {
        const auto* event=game+0x3B568+i*0xA8;
        if(word(event,0x2C)!=sequence)continue;
        const bool match=word(event,0x54)==base+0x112BF0 && word(event,0x4C)==2 &&
            word(event,0x50)==1 && word(event,0x60)==0x10000 && word(event,0x84)==1;
#ifdef KINGDOM_EFFECT_TEXT_TEST
        if(match)++declined_requests;
#endif
        return match;
    }
    return false;
}

__declspec(naked) inline void automatic_hook() {
    __asm {
        pushfd
        pushad
        push edi
        push ebx
        call decline_garbage_heap
        add esp,8
        test al,al
        jz original
        popad
        popfd
        mov dword ptr [edi+6Ch],0
        jmp dword ptr [automatic_empty]
    original:
        popad
        popfd
        mov eax,dword ptr [edi+4]
        mov esi,dword ptr [edi+50h]
        mov dword ptr [ebp-1Ch],esi
        jmp dword ptr [automatic_continue]
    }
}

__declspec(naked) inline void load_hook() {
    __asm {
        pushfd
        pushad
        call reset
        popad
        popfd
        push ebx
        mov ebx,esp
        sub esp,8
        jmp dword ptr [load_continue]
    }
}

// Re-enter the original GetSmartplayDefs prologue for all ordinary cards.
__declspec(naked) inline DWORD __fastcall original_defs(DWORD,const Definition**) {
    __asm {
        push ebp
        mov ebp,esp
        push ecx
        push ebx
        push esi
        push edi
        jmp dword ptr [defs_continue]
    }
}
inline DWORD __fastcall get_defs(DWORD card,const Definition** output) {
    if(card==garbage_heap_card && local_client() && output){
#ifdef KINGDOM_EFFECT_TEXT_TEST
        ++definition_queries;
#endif
        output[0]=&definition;return 1;
    }
    return original_defs(card,output);
}
__declspec(naked) inline void default_hook() {
    __asm {
        cmp ecx,4B455001h
        je custom
        push esi
        call dword ptr [native_smartplay_get]
        jmp dword ptr [default_continue]
    custom:
        jmp current
    }
}
__declspec(naked) inline void toggle_hook() {
    __asm {
        cmp ecx,4B455001h
        je custom
        push ebp
        mov ebp,esp
        and esp,0FFFFFFF8h
        jmp dword ptr [toggle_continue]
    custom:
        jmp toggle
    }
}
__declspec(naked) inline void save_hook() {
    __asm {
        cmp ecx,4B455001h
        je custom
        push ebp
        mov ebp,esp
        and esp,0FFFFFFF8h
        jmp dword ptr [save_continue]
    custom:
        // Session settings have no profile default to save. The native caller
        // owns stack-argument cleanup, so this must be a plain RET.
        ret
    }
}
inline void initialize(std::uintptr_t image) {
    base=image;reset();
    defs_continue=image+0x21CE07;
    default_continue=image+0x21CEE6;native_smartplay_get=image+0x21CE90;
    toggle_continue=image+0x1E3EC6;save_continue=image+0x1E3E56;
    automatic_continue=image+0x2229BA;automatic_empty=image+0x2232DE;
    load_continue=image+0x224076;
}
}
