#pragma once
#include "target_access.hpp"
#include <cstdint>
#include <utility>
namespace kingdom {
struct TargetAccessNativeCard { const char* id; std::uint32_t native_id; };
}
#include "target_access.generated.hpp"
namespace kingdom {
// Installed only from the active game's official authorization snapshot.
// Standalone UI tests must supply an explicit fixture provider.
inline CardAccessQuery editor_card_access;
inline std::string_view canonical_access_id(std::string_view id) {
    for (const auto& alias : access_catalog::aliases) if (id == alias.first) return alias.second;
    return id;
}
inline const TargetAccessNativeCard* access_native_card(std::string_view id) {
    id = canonical_access_id(id);
    for (const auto& card : access_catalog::cards) if (id == card.id) return &card;
    return nullptr;
}
inline CardAccess target_card_access(std::string_view id) {
    return editor_card_access ? editor_card_access(canonical_access_id(id)) : CardAccess::unknown;
}
inline TargetAccessFailure check_target_access(const std::vector<std::string>& commands) {
    return validate_target_access(commands, access_catalog::rules, target_card_access);
}
std::wstring target_access_message(const TargetAccessFailure& failure);
}
