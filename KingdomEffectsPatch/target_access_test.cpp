#include "target_access.hpp"

#include <cassert>
#include <iostream>

namespace {
using namespace kingdom;
constexpr TargetAccessRule rules[]{
    {"twist_big_ideas", 0, 0}, {"twist_typhoon", 1, 20},
    {"start_add_card", 1, 1}, {"start_make_split_pile", 2, 2},
    {"start_double_pile", 1, 1}, {"twist_room_for_more", 1, 1},
};

void all_arguments_and_atomic_failure() {
    std::vector<std::string> queries;
    const CardAccessQuery query = [&](std::string_view id) {
        queries.emplace_back(id);
        return id == "unowned" ? CardAccess::denied :
            id == "unresolved" ? CardAccess::unknown : CardAccess::allowed;
    };
    assert(!validate_target_access({"twist_big_ideas", "start_add_card(village)"}, rules, query));
    assert(queries == std::vector<std::string>{"village"});
    for (const auto& command : {"twist_typhoon(unowned,market,village)",
                               "twist_typhoon(market,unowned,village)",
                               "twist_typhoon(market,village,unowned)"}) {
        const std::vector<std::string> commands{"twist_big_ideas", command, "start_double_pile(village)"};
        const auto before = commands;
        const auto failure = validate_target_access(commands, rules, query);
        assert(failure && failure.state == CardAccess::denied && !failure.malformed);
        assert(failure.effect_id == "twist_typhoon" && failure.target_id == "unowned");
        assert(commands == before);
    }
    auto failure = validate_target_access({"start_make_split_pile(village,unowned)"}, rules, query);
    assert(failure && failure.target_id == "unowned");
    failure = validate_target_access({"start_make_split_pile(unowned,village)"}, rules, query);
    assert(failure && failure.target_id == "unowned");
    failure = validate_target_access({"start_add_card(unresolved)"}, rules, query);
    assert(failure.state == CardAccess::unknown && !failure.malformed);
    failure = validate_target_access({"start_add_card(village)"}, rules, {});
    assert(failure && failure.state == CardAccess::unknown);
    assert(!validate_target_access({"twist_big_ideas"}, rules, {}));
    assert(!validate_target_access({}, rules, {}));
}

void capacity_and_structure() {
    unsigned queries = 0;
    const CardAccessQuery query = [&](std::string_view) { ++queries; return CardAccess::allowed; };
    for (const auto* command : {"twist_typhoon", "twist_typhoon()", "twist_typhoon(village,)",
                               "start_make_split_pile(village)", "start_make_split_pile(village,market,smithy)",
                               "start_add_card(3)", "twist_big_ideas(3)", "start_add_card(village)(market)",
                               "start_add_card((village))", "start_add_card(village)trailing", ""}) {
        queries = 0;
        const auto failure = validate_target_access({command}, rules, query);
        assert(failure && failure.malformed && queries == 0);
    }
    const auto unknown = validate_target_access({"future_effect(3)"}, rules, query);
    assert(unknown && unknown.state == CardAccess::unknown && !unknown.malformed && queries == 0);
    std::string command = "twist_typhoon(";
    for (unsigned i = 0; i < 20; ++i) command += i ? ",village" : "village";
    command += ')';
    assert(!validate_target_access({command}, rules, query) && queries == 20);
    command.insert(command.size() - 1, ",market"); queries = 0;
    assert(validate_target_access({command}, rules, query).malformed && queries == 0);
}

void revalidation_and_aliases() {
    CardAccess state = CardAccess::allowed;
    std::string last;
    const CardAccessQuery query = [&](std::string_view id) { last = id; return state; };
    const std::vector<std::string> commands{" start_add_card(  fortune  ) "};
    assert(!validate_target_access(commands, rules, query) && last == "fortune");
    state = CardAccess::denied;
    assert(validate_target_access(commands, rules, query).state == CardAccess::denied);
    state = CardAccess::unknown;
    assert(validate_target_access(commands, rules, query).state == CardAccess::unknown);
    // Canonical/split-pile mapping belongs to the provider; the validator
    // passes the full unmodified native target identifier to that provider.
    assert(last == "fortune");
    assert(!validate_target_access({"twist_typhoon(village, market)", "twist_room_for_more(smithy)"},
                                  rules, [](std::string_view) { return CardAccess::allowed; }));
}
}

int main() {
    all_arguments_and_atomic_failure();
    capacity_and_structure();
    revalidation_and_aliases();
    std::cout << "PASS: shared target access, all argument positions, atomic denial, bounds, unknown access and revalidation.\n";
}
