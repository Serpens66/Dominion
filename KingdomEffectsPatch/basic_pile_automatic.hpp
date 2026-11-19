#pragma once

#include "effects.hpp"
#include "target_access.hpp"

namespace kingdom::basic_pile_automatic {

// Use ordinary, serializable native effects. A missing pile is a native no-op;
// these commands do not enable Platinum/Colonies or select Kingdom cards.
constexpr std::array<std::uint32_t, 8> basic_types{
    0x104, 0x105, 0x106, 0x101, 0x102, 0x103, 0x600, 0x500
};
enum class Result : unsigned { none, access_unknown, invalid_state, capacity };
struct Plan {
    Result result = Result::none;
    Effects effects{};
    unsigned added = 0;
};

template<class Access>
Plan prepare(const Effects& original, const Access& access) {
    Plan plan;
    plan.effects = original;
    auto size = count(original);
    for (const auto type : basic_types) {
        const auto already = [&](const Effect& effect) {
            if (effect[0] != double_pile && effect[0] != triple_pile) return false;
            // Native Province multipliers also affect Colony. Treat these as
            // one group: an explicit multiplier on either takes precedence.
            return effect[1] == type || !effect[1] ||
                (type == 0x103 && effect[1] == 0x601);
        };
        if (std::any_of(original.begin(), original.begin() + count(original), already)) continue;
        const auto allowed = access.query_native(type);
        if (allowed == CardAccess::unknown) { plan.result = Result::access_unknown; return plan; }
        if (allowed != CardAccess::allowed) continue;
        if (size == maximum_effects) { plan.result = Result::capacity; return plan; }
        plan.effects[size++] = {double_pile, type, 0xFFFFFFFFu, 0, 0};
        ++plan.added;
    }
    return plan;
}

}
