#pragma once
#include <cstdint>
#include <cstring>

namespace kingdom::effect_text_fix {
// Build 3315 LookupTranslationSafe: ECX=canonical key, EDX=language index,
// EAX=borrowed translation-entry pointer (entry[0] is the UTF-8 text).
// Targeted callers: RVA 0x19B3D2 (E8 59 13 0F 00) and
// RVA 0x19B68C (E8 9F 10 0F 00), both in the card-description builder.
using TranslationLookup = const void* (__fastcall*)(const char*, std::uint32_t);
inline TranslationLookup native_lookup = nullptr;
#ifdef KINGDOM_EFFECT_TEXT_TEST
inline unsigned debt_caption_calls=0;
#endif

inline void initialize(std::uintptr_t image_base) {
    native_lookup = reinterpret_cast<TranslationLookup>(image_base + 0x28C730);
}

inline const char* translation_key(const char* key) {
    // The command ID deliberately retains its original spelling. Only the
    // feature-caption lookup uses the existing localized display-name key.
    return key && std::strcmp(key, "stamp_cost_2D_less") == 0
        ? "stamp_minus_two_debt" : key;
}

inline const void* __fastcall lookup(const char* key, std::uint32_t language) {
#ifdef KINGDOM_EFFECT_TEXT_TEST
    if(key && std::strcmp(key,"stamp_cost_2D_less")==0)++debt_caption_calls;
#endif
    return native_lookup ? native_lookup(translation_key(key), language) : nullptr;
}

inline void (*hook_address())() {
    return reinterpret_cast<void(*)()>(lookup);
}

// Developer checks may query both installed dictionaries without changing
// the player's language preference: native indices 1=en-us and 2=de.
inline const char* debt_caption_for_test(std::uint32_t language) {
    const auto* entry=lookup("stamp_cost_2D_less",language);
    return entry ? *static_cast<const char* const*>(entry) : nullptr;
}
}
