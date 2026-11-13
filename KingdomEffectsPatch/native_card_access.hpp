#pragma once

// Dominion 3315 only. The injector verifies the supported EXE hash before this
// adapter is installed. Call capture/query on the game's UI thread.
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "target_access.hpp"

namespace kingdom {

namespace access_detail {
inline bool copy_readable(void* destination, const void* source, std::size_t length) {
    if (!destination || !source || !length) return false;
    __try { std::memcpy(destination, source, length); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> bool read(const std::byte* at, T& value) {
    return copy_readable(&value, at, sizeof value);
}
inline bool official_match(const std::byte* base, std::uint32_t grant,
                           std::uint32_t card, bool& result) {
    using Match = bool (__fastcall*)(std::uint32_t,std::uint32_t);
    __try { result=reinterpret_cast<Match>(const_cast<std::byte*>(base)+0x0DB7A0)(grant,card); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}

class NativeCardAccess {
    const std::byte* base_=nullptr;
    const std::byte* profile_=nullptr;
    std::array<std::uint32_t,128> grants_{};
    std::size_t count_=0;
    bool known_=false;
public:
    static NativeCardAccess capture(const std::byte* base) {
        NativeCardAccess result;
        result.base_=base;
        if (!base) return result;
        constexpr std::array<unsigned char,12> getter_bytes{0x55,0x8B,0xEC,0x83,0xE4,0xF8,0x81,0xEC,0x34,0x02,0x00,0x00};
        constexpr std::array<unsigned char,12> match_bytes{0x55,0x8B,0xEC,0x83,0xE4,0xF8,0x51,0x53,0x56,0x33,0xF6,0x89};
        std::array<unsigned char,12> observed{};
        if (!access_detail::copy_readable(observed.data(),base+0x0DAF40,observed.size()) || observed!=getter_bytes ||
            !access_detail::copy_readable(observed.data(),base+0x0DB7A0,observed.size()) || observed!=match_bytes) return result;
        const std::byte* settings=nullptr;
        std::uint32_t profiles=0,active=0;
        if (!access_detail::read(base+0x8C8DC8,settings) || !settings ||
            !access_detail::read(settings+0x1E1A0,profiles) || !profiles || profiles>4 ||
            !access_detail::read(settings+0x1E1A4,active)) return result;
        for (std::uint32_t i=0;i<profiles;++i) {
            const auto* candidate=settings+i*0x7868;
            std::uint32_t id=0;
            if (!access_detail::read(candidate+0x42B0,id)) return result;
            if (id==active) { result.profile_=candidate; break; }
        }
        // The native active-profile resolver changes profile when the ID is not
        // found. Never trigger that repair from this read-only adapter.
        if (!result.profile_) return result;
        std::uint32_t purchases=0,cached=0;
        if (!access_detail::read(result.profile_+0x56D8,purchases) || purchases>128 ||
            !access_detail::read(result.profile_+0x7450,cached) || cached>128) return result;
        // Read only the cache already published by Dominion. Calling the
        // native getter on a cold cache would also mark grants as seen, rewrite
        // Kingdom configurations and save the profile (RVA 0x0E4D60).
        if (!cached) return result;
        result.count_=cached;
        if (!access_detail::copy_readable(result.grants_.data(),result.profile_+0x7454,
                                         result.count_*sizeof(std::uint32_t))) return result;
        std::uint32_t after=0;
        if (!access_detail::read(result.profile_+0x7450,after) || after!=cached) return result;
        // Grant definitions: 39 native entries, ID at +0, kind at +8. Reject an
        // uninitialized/corrupt entry rather than invoking the native fallback.
        for (std::size_t i=0;i<result.count_;++i) {
            bool found=false;
            for (unsigned j=0;j<39;++j) {
                std::uint32_t id=0,kind=0;
                const auto* entry=base+0x380878+j*0x10C;
                if (!access_detail::read(entry,id) || !access_detail::read(entry+8,kind)) return result;
                if (id==result.grants_[i] && kind>=1 && kind<=5) { found=true; break; }
            }
            if (!found) return result;
        }
        result.known_=true;
        return result;
    }
    bool known() const { return known_; }
    std::size_t grant_count() const { return count_; }
    const std::array<std::uint32_t,128>& grants() const { return grants_; }
    const std::byte* profile() const { return profile_; }
    CardAccess query_native(std::uint32_t card) const {
        // The caller maps IDs through the generated native card catalog; never
        // pass arbitrary/unregistered numeric IDs to native CardDef lookups.
        if (!known_ || !card) return CardAccess::unknown;
        for (std::size_t i=0;i<count_;++i) {
            bool permitted=false;
            if (!access_detail::official_match(base_,grants_[i],card,permitted)) return CardAccess::unknown;
            if (permitted) return CardAccess::allowed;
        }
        return CardAccess::denied;
    }
};

}
