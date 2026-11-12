#pragma once
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace kingdom::native_ui {

enum class BanHintStatus { applied, already_applied, wrong_thread, incompatible_api, unavailable, incompatible_entry };
struct BanHintResult { BanHintStatus status; unsigned changed; };
inline const char* ban_hint_text(unsigned language) {
    if(language==2)return u8"{taptip_normal}Markierte Karten sind online gesperrt. Alle Mitspieler-Verbote gelten. Mit Mod fasst die eigene Online-Liste bis zu 700 Karten und Querkarten.";
    if(language==1)return "{taptip_normal}Checked cards are excluded online. All players' bans are honored. With this mod, your online list supports up to 700 cards and landscapes combined.";
    return nullptr;
}

// Build 3315 only. The owner must belong to the game's UI thread. Native
// XString refcounts and translation/cache maps must never be edited by a
// remote worker, inside ThreadPause, or under the DLL loader lock.
inline BanHintResult apply_ban_hints(HWND owner,std::uintptr_t image) {
    auto* base=reinterpret_cast<std::byte*>(image);
    DWORD process=0;
    if(!base || !owner || GetWindowThreadProcessId(owner,&process)!=GetCurrentThreadId() || process!=GetCurrentProcessId())
        return {BanHintStatus::wrong_thread,0};
    struct Guard {std::uintptr_t rva;const char* bytes;std::size_t count;};
    const Guard guards[]{
        {0x28C730,"\x55\x8B\xEC\x6A\xFF",5},
        {0x23D720,"\x56\x8B\xF2\x57\x8B\xF9",6},
        {0x23D850,"\x55\x8B\xEC\x53\x8B\x5D\x08",7},
        {0x2378E0,"\x55\x8B\xEC\x51\x53\x56\x57",7},
    };
    for(const auto& guard:guards)if(std::memcmp(base+guard.rva,guard.bytes,guard.count)!=0)
        return {BanHintStatus::incompatible_api,0};
    if(!*reinterpret_cast<void**>(base+0x107ABFC) || !*reinterpret_cast<void**>(base+0x7E3598))return {BanHintStatus::unavailable,0};

    using Lookup=void*(__fastcall*)(const char*,unsigned);
    using Construct=void(__fastcall*)(void*,const char*);
    using Assign=void*(__thiscall*)(void*,const char* const*);
    using Clear=void(__thiscall*)(void*);
    const auto lookup=reinterpret_cast<Lookup>(base+0x28C730);
    const auto construct=reinterpret_cast<Construct>(base+0x23D720);
    const auto assign=reinterpret_cast<Assign>(base+0x23D850);
    struct Text {unsigned language;const char* key;const char* expected;const char* replacement;};
    const auto* de=ban_hint_text(2);
    const auto* en=ban_hint_text(1);
    const Text texts[]{
        {1,"dom_taptip_gallery_ban_mp_body","{taptip_normal}If this is checked, this card will not show up in multiplayer games. When a multiplayer game is generated, all players' banned cards are honored. Up to five cards may be banned this way.",en},
        {1,"dom_taptip_gallery_ban_online_body","{taptip_normal}If this is checked, this card will not show up in multiplayer games. When a multiplayer game is generated, all players' banned cards are honored. Up to five cards may be banned this way.",en},
        {2,"dom_taptip_gallery_ban_mp_body",u8"{taptip_normal}Wenn diese Option gewählt ist, wird diese Karte nicht in Online-Spielen auftauchen. Wenn ein Multiplayer-Spiel generiert wird, werden die gesperrten Karten aller Spieler:innen berücksichtigt. Hierfür kannst du bis zu fünf Karten sperren.",de},
        {2,"dom_taptip_gallery_ban_online_body",u8"{taptip_normal}Wenn markiert, wird diese Karte in Online Spielen nicht verwendet. Auch die verbotenen Karten der anderen Spieler:innen werden berücksichtigt.",de},
    };
    struct Stage {void* entry=nullptr;const char* replacement=nullptr;bool change=false;};
    std::array<Stage,4> staged{};
    // First inspect every entry and allocate every replacement. No map value
    // is changed if another mod/resource version has incompatible metadata.
    for(std::size_t i=0;i<staged.size();++i) {
        auto* entry=lookup(texts[i].key,texts[i].language);
        if(!entry)return {BanHintStatus::unavailable,0};
        const auto* fields=static_cast<const char* const*>(entry);
        const auto* data=static_cast<const std::byte*>(entry);
        const auto* current=fields[0];
        // The four audited tooltip entries have empty auxiliary strings and
        // default taptip-normal formatting metadata (1,1,0,0,0,0). Preserve all
        // metadata and the {taptip_normal} prefix. Offset 0x1B is padding and is
        // deliberately neither checked nor written.
        if(!current || (fields[1] && *fields[1]) || (fields[2] && *fields[2]) ||
           *reinterpret_cast<const unsigned*>(data+0x0C)!=1 ||
           *reinterpret_cast<const unsigned*>(data+0x10)!=1 ||
           *reinterpret_cast<const unsigned*>(data+0x14)!=0 ||
           data[0x18]!=std::byte{0} || data[0x19]!=std::byte{0} || data[0x1A]!=std::byte{0})
            return {BanHintStatus::incompatible_entry,0};
        const bool already=std::strcmp(current,texts[i].replacement)==0;
        if(!already && std::strcmp(current,texts[i].expected)!=0)
            return {BanHintStatus::incompatible_entry,0};
        staged[i].entry=entry;staged[i].change=!already;
    }
    struct Owned {
        Assign assign;const char* value=nullptr;
        ~Owned(){const char* empty=nullptr;assign(&value,&empty);}
    };
    // Native constructor allocates an owned XString. Passing a literal to the
    // native ref-assignment function would corrupt refcounts and is forbidden.
    std::array<Owned,4> strings{{{assign},{assign},{assign},{assign}}};
    for(std::size_t i=0;i<staged.size();++i)if(staged[i].change) {
        construct(&strings[i].value,texts[i].replacement);
        if(!strings[i].value || std::strcmp(strings[i].value,texts[i].replacement)!=0)
            return {BanHintStatus::unavailable,0};
    }
    unsigned changed=0;
    for(std::size_t i=0;i<staged.size();++i)if(staged[i].change) {
        assign(staged[i].entry,&strings[i].value);++changed;
    }
    if(changed) {
        // Same translated-text cache clear used by the native language-change
        // handler at RVA 224A8C; no language/profile setting is changed.
        reinterpret_cast<Clear>(base+0x2378E0)(base+0x7E3598);
    }
    return {changed?BanHintStatus::applied:BanHintStatus::already_applied,changed};
}

}
