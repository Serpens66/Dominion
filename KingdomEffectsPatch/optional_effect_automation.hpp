#pragma once
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "campaign_order_sources.hpp"

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
inline std::uintptr_t automatic_first=0;
// Explicit F8 opt-in. Keep across local game loads within this process;
// reset() only resets the per-participant Ask/Never preferences.
inline bool order_enabled=false;
inline std::uintptr_t load_continue=0;
#ifdef KINGDOM_EFFECT_TEXT_TEST
inline unsigned definition_queries=0,declined_requests=0;
inline unsigned ordered_requests=0;
inline unsigned order_auto_calls=0,order_kind6_calls=0,order_eligible_calls=0,order_origin_calls=0;
inline std::array<DWORD,8> order_frame_returns{};
inline std::array<DWORD,48> order_capture{};
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
inline std::byte* order_client() {
    if(!base)return nullptr;
    auto* client=*reinterpret_cast<std::byte**>(base+0x8C8D5C);
    if(!client)return nullptr;
    const auto type=word(client,0x5068);
    return type==1 || type==2?client:nullptr;
}
inline PlayerSetting* player_setting() {
    const auto* client=local_client();if(!client)return nullptr;
    const auto id=word(client,0x7590);
    if(id==0xFFFFFFFF)return nullptr;
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
    if(word(action,4)!=1 || word(action,0x54)!=3)return false;
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

inline const std::byte* queued_event(const std::byte* game,DWORD handle) {
    if(!game || (handle&0x30))return nullptr;
    const auto count=word(game,0x19B8);if(!count || count>256)return nullptr;
    const auto sequence=handle>>18;
    for(std::size_t i=count;i-->0;) {
        const auto* event=game+0x3B568+i*0xA8;
        if(word(event,0x2C)==sequence)return event;
    }
    return nullptr;
}
inline bool active_campaign_source(const std::byte* game,DWORD display) {
    // These two display definitions are physically swapped in native lookup.
    // Their event-source identity has not been proved; retain native ordering.
    if(display==0x1530 || display==0x1531)return false;
    DWORD effect=0;
    for(const auto& source:campaign_order_sources)if(source.display==display){effect=source.effect;break;}
    if(!effect)return false;
    for(unsigned i=0;i<32;++i){const auto id=word(game,0x1280+i*0x14);if(id==effect)return true;if(!id)break;}
    return false;
}
inline bool campaign_order_request(const std::byte* game,const std::byte* action,bool order_origin) {
    if(!order_enabled || !order_origin || !game || !action || word(action,4)!=6)return false;
    const auto* client=order_client();
    if(!client || word(action,0x4C)!=word(action,0x50))return false;
    // Local/Weitergeben applies this process-wide preference to every local
    // human after native222940's player/control/readiness checks. Its viewer
    // key is still unset while initial turn triggers are being created.
    // Network replies remain restricted to the actual own-player key.
    if(word(client,0x5068)==2 && word(action,0x50)!=word(client,0x7590))return false;
    const auto count=word(action,0x6C),events=word(game,0x19B8);
    if(!count || count>256 || events<count || events>256)return false;
    for(unsigned i=0;i<count;++i) {
        const auto handle=word(action,0x70+i*4);
        for(unsigned j=0;j<i;++j)if(word(action,0x70+j*4)==handle)return false;
        const auto* event=queued_event(game,handle);
        if(!event || word(event,0x4C)!=2)return false;
        // Native17EC40/17F120 explicitly preserve an activation choice when
        // flag bit 0 is set, even for one candidate. Never auto-call those.
        if(word(event,0x60)&1)return false;
        const auto callback=word(event,0x54);
        if(callback<base+0x1000 || callback>=base+0x3742DD)return false;
        const auto override_source=word(event,0x28);
        const auto source=override_source?override_source:word(event,0x24);
        if(!active_campaign_source(game,source))return false;
    }
    return true;
}
inline bool native_order_origin(std::uintptr_t frame) {
    const auto* tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    const auto low=reinterpret_cast<std::uintptr_t>(tib->StackLimit);
    const auto high=reinterpret_cast<std::uintptr_t>(tib->StackBase);
    const DWORD returns[]{0x22349E,0x17EE53,0x17F2D1};
    for(unsigned i=0;i<4;++i) {
        if((frame&3) || frame<low || high<8 || frame>high-8)return false;
        const auto returned=word(reinterpret_cast<void*>(frame),4);
        if(i==3)return returned==base+0x180B5B || returned==base+0x18C50B;
        if(returned!=base+returns[i])return false;
        frame=word(reinterpret_cast<void*>(frame),0);
    }
    return false;
}
inline DWORD __cdecl automatic_response(const std::byte* game,const std::byte* action,std::uintptr_t frame) {
#ifdef KINGDOM_EFFECT_TEXT_TEST
    ++order_auto_calls;
    if(word(action,4)==6) {
        ++order_kind6_calls;
        if(campaign_order_request(game,action,true))++order_eligible_calls;
        if(native_order_origin(frame))++order_origin_calls;
        order_capture={};
        const auto* client=*reinterpret_cast<const std::byte**>(base+0x8C8D5C);
        order_capture[0]=static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(client));
        if(client){order_capture[1]=word(client,0x5068);order_capture[2]=word(client,0x7590);}
        order_capture[3]=word(action,0x4C);order_capture[4]=word(action,0x50);
        order_capture[5]=word(action,0x6C);order_capture[6]=word(game,0x19B8);order_capture[7]=order_enabled;
        for(unsigned i=0;i<4 && i<word(action,0x6C);++i){
            auto* fields=order_capture.data()+8+i*10;fields[0]=word(action,0x70+i*4);
            const auto* event=queued_event(game,fields[0]);if(!event)continue;
            fields[1]=1;fields[2]=word(event,0x4C);fields[3]=word(event,0x60);fields[4]=word(event,0x54)-static_cast<DWORD>(base);
            fields[5]=word(event,0x24);fields[6]=word(event,0x28);
            fields[7]=active_campaign_source(game,fields[6]?fields[6]:fields[5]);
        }
        const auto* tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
        const auto low=reinterpret_cast<std::uintptr_t>(tib->StackLimit),high=reinterpret_cast<std::uintptr_t>(tib->StackBase);
        auto cursor=frame;order_frame_returns={};
        for(unsigned i=0;i<order_frame_returns.size();++i) {
            if((cursor&3) || cursor<low || high<8 || cursor>high-8)break;
            order_frame_returns[i]=word(reinterpret_cast<void*>(cursor),4)-static_cast<DWORD>(base);
            cursor=word(reinterpret_cast<void*>(cursor),0);
        }
    }
#endif
    if(decline_garbage_heap(game,action))return 1;
    if(!campaign_order_request(game,action,native_order_origin(frame)))return 0;
#ifdef KINGDOM_EFFECT_TEXT_TEST
    ++ordered_requests;
#endif
    return 2;
}

__declspec(naked) inline void automatic_hook() {
    __asm {
        pushfd
        pushad
        push ebp
        push edi
        push ebx
        call automatic_response
        add esp,12
        cmp eax,2
        je first_option
        test al,al
        jz original
        popad
        popfd
        mov dword ptr [edi+6Ch],0
        jmp dword ptr [automatic_empty]
    first_option:
        popad
        popfd
        jmp dword ptr [automatic_first]
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
    automatic_first=image+0x2232D7;
    load_continue=image+0x224076;
}
}
