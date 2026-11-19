#pragma once
#include "basic_pile_automatic.hpp"
#include <cassert>

inline void run_basic_pile_automatic_checks() {
    using namespace kingdom;
    using namespace kingdom::basic_pile_automatic;
    struct Access {
        CardAccess result = CardAccess::allowed;
        unsigned special = 0;
        CardAccess query_native(unsigned type) const {
            return !special || special == type ? result : CardAccess::allowed;
        }
    };
    const Effects original{};
    auto plan = prepare(original, Access{});
    assert(plan.result == Result::none && plan.added == 8 && count(plan.effects) == 8);
    assert(original == Effects{});
    assert(std::none_of(plan.effects.begin(), plan.effects.end(), [](const Effect& e) { return e[1] == 0x601; }));
    Effects effects{};
    effects[0] = {double_pile, 0x103, 0xFFFFFFFFu, 0, 0};
    plan = prepare(effects, Access{});
    assert(plan.result == Result::none && plan.added == 7 && plan.effects[0] == effects[0]);
    effects[0][1] = 0x601;
    plan = prepare(effects, Access{});
    assert(plan.result == Result::none && plan.added == 7 && plan.effects[0] == effects[0]);
    assert(std::none_of(plan.effects.begin(), plan.effects.end(), [](const Effect& e) { return e[1] == 0x103; }));
    effects[0] = {triple_pile, 0x106, 0xFFFFFFFFu, 0, 0};
    plan = prepare(effects, Access{});
    assert(plan.result == Result::none && plan.added == 7 && plan.effects[0] == effects[0]);
    plan = prepare(original, Access{CardAccess::denied, 0x600});
    assert(plan.result == Result::none && plan.added == 7);
    plan = prepare(original, Access{CardAccess::unknown, 0x600});
    assert(plan.result == Result::access_unknown && original == Effects{});
    effects.fill({double_pile, 0x206, 0xFFFFFFFFu, 0, 0});
    const auto crowded = effects;
    plan = prepare(effects, Access{});
    assert(plan.result == Result::capacity && effects == crowded);
    effects[24] = {};
    plan = prepare(effects, Access{});
    assert(plan.result == Result::none && count(plan.effects) == 32 && plan.added == 8);
    effects[24] = effects[0]; effects[25] = {};
    plan = prepare(effects, Access{});
    assert(plan.result == Result::capacity);
    const auto once = prepare(original, Access{});
    plan = prepare(once.effects, Access{});
    assert(plan.result == Result::none && plan.added == 0 && plan.effects == once.effects);
}
