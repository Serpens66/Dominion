#include "effects.hpp"
#include "effect_editor.hpp"

#include <cassert>
#include <iostream>
#include <string>

namespace {

using namespace kingdom;

constexpr Effect doubling(std::uint32_t card) {
    return {0xDB8, card, 0xFFFFFFFFu, 0, 0};
}

Setup setup(const Effects& effects) {
    Setup result{};
    write_effects(result.data(), effects);
    return result;
}

void distinct_targets_and_multiplicity() {
    const auto village = doubling(1);
    const auto market = doubling(2);
    const auto old = setup({village});
    const auto incoming = setup({village, market, market});
    const auto merged = merge_effects(old.data(), incoming.data());
    assert(merged.error == Error::none);
    assert(count(merged.list) == 3);
    assert(merged.list[0] == village && merged.list[1] == market && merged.list[2] == market);
    const auto combined = setup(merged.list);
    const auto again = merge_effects(combined.data(), incoming.data());
    assert(again.error == Error::none && again.list == merged.list);

    const Effect split_a{0xDC5, 7, 0xFFFFFFFFu, 0, 8};
    const Effect split_b{0xDC5, 7, 0xFFFFFFFFu, 0, 9};
    const auto a = setup({split_a});
    const auto b = setup({split_b});
    assert(count(merge_effects(a.data(), b.data()).list) == 2);
}

void capacity() {
    Effects entries{};
    for (unsigned i = 0; i < 31; ++i) entries[i] = doubling(i + 1);
    const auto old = setup(entries);
    const auto last = setup({doubling(32)});
    const auto full = merge_effects(old.data(), last.data());
    assert(full.error == Error::none && count(full.list) == 32);
    const auto two = setup({doubling(32), doubling(33)});
    assert(merge_effects(old.data(), two.data()).error == Error::capacity);
    assert(read_effects(old.data()) == entries);
}

void automatic_doublings() {
    const KingdomPiles piles{11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    const Effect shoppers{0x177A, 0, 0xFFFFFFFFu, 0, 0};
    const Effect explicit_triple{triple_pile, 12, 0xFFFFFFFFu, 0, 0};
    Effects effects{shoppers, explicit_triple, doubling(20)};
    assert(append_doublings(effects, piles) == Error::none);
    assert(count(effects) == 11);
    assert(effects[0] == shoppers && effects[1] == explicit_triple && effects[2] == doubling(20));
    assert(effects[3] == doubling(11) && effects[4] == doubling(13) && effects[10] == doubling(19));
    const auto once = effects;
    assert(append_doublings(effects, piles) == Error::none && effects == once);

    Effects crowded{};
    for (unsigned i = 0; i < 23; ++i) crowded[i] = shoppers;
    const auto before = crowded;
    assert(append_doublings(crowded, piles) == Error::capacity && crowded == before);
    auto duplicate = piles;
    duplicate.back() = duplicate.front();
    Effects empty{};
    assert(append_doublings(empty, duplicate) == Error::malformed && count(empty) == 0);
}

void shared_targets() {
    auto a = setup({Effect{room, 0, 0xFFFFFFFFu, 0, 0}});
    a[room_offset] = std::byte{1};
    a[room_offset + 4] = std::byte{12};
    auto b = a;
    b[room_offset + 4] = std::byte{13};
    assert(merge_effects(a.data(), b.data()).error == Error::shared_target);

    auto empty = setup({});
    const auto plan = merge_effects(empty.data(), a.data());
    assert(plan.error == Error::none && plan.has_room);
    apply_merge(empty.data(), plan);
    assert(empty == a);
    write_effects(empty.data(), Effects{});
    assert(count(read_effects(empty.data())) == 0);
}

void text_limits() {
    constexpr auto ok = Error::none;
    assert(validate_text("-k market, village -x start_double_pile(village), "
                         "start_make_split_pile(market,smithy) -m Landscapes(0,4)", 4095) == ok);
    assert(validate_text("village", 4095) == ok);
    assert(validate_text("-x twist_typhoon(market,smithy)", 4095) == ok);
    assert(validate_text("-x start_double_pile()", 4095) == Error::malformed);
    assert(validate_text("-x start_double_pile(market", 4095) == Error::malformed);
    assert(validate_text("-x twist_shoppers -x twist_busy_busy", 4095) == Error::repeated_section);
    assert(validate_text("-x twist_room_for_more(market), twist_room_for_more(smithy)", 4095)
           == Error::shared_target);
    std::string text = "-x ";
    for (unsigned i = 0; i < 32; ++i) {
        if (i) text += ',';
        text += "twist_shoppers";
    }
    assert(validate_text(text, 4095) == ok);
    text += ",twist_shoppers";
    assert(validate_text(text, 4095) == Error::capacity);
    assert(validate_text(std::string(4096, 'a'), 4095) == Error::input_length);
}

void known_crashes() {
    const auto check = [](std::string_view text) { return validate_text(text, 4095, true); };
    assert(check("-x start_with_cube(pageant)") == Error::none);
    assert(check("-x start_with_cube(pageant), start_with_cube(road_network)") == Error::none);
    assert(check("-x start_with_cube(pageant), twist_big_ideas, start_with_cube( pageant )")
           == Error::repeated_project_cube);
    assert(check("-x stamp_cost_2D_less(triumph)") == Error::debt_stamp);
    assert(check("-x stamp_cost_2d_less(triumph)") == Error::debt_stamp);
    assert(validate_text("-x stamp_cost_2D_less(triumph)", 4095) == Error::none);
    Effects effects{};
    effects[0] = {project_cube, 0xE20, 0xFFFFFFFFu, 0, 0};
    effects[1] = {project_cube, 0xE21, 0xFFFFFFFFu, 0, 0};
    assert(known_failure(effects) == Error::none);
    effects[1][1] = effects[0][1];
    assert(known_failure(effects) == Error::repeated_project_cube);
    effects[0][0] = debt_stamp;
    assert(known_failure(effects) == Error::debt_stamp);
}

void editor_selection() {
    assert((fixed_kingdom_ids("-k village, market, young_witch(chapel), -m Landscapes(0,1) "
                              "-x twist_room_for_more(gardens)") ==
            std::vector<std::string>{"village", "market", "young_witch"}));
    assert(fixed_kingdom_ids("-m Landscapes(0,1) -x twist_room_for_more(gardens)").empty());

    EffectCommands draft(31, "twist_big_ideas");
    const EffectCommands two{"start_double_pile(village)", "start_double_pile(market)"};
    const auto original = draft;
    const auto overflow = append_editor_effects(draft, two, true);
    assert(overflow.error == Error::capacity && overflow.missing_slots == 1 && draft == original);
    assert(append_editor_effects(draft, {two.front()}, true).error == Error::none && draft.size() == 32);
    assert(append_editor_effects(draft, {"start_double_pile( village )"}, true).error == Error::none &&
           draft.size() == 32);
    assert(append_editor_effects(draft, {two.front()}, false).error == Error::capacity);

    draft.clear();
    assert(append_editor_effects(draft, two, true).error == Error::none && draft == two);
    assert(append_editor_effects(draft, {two.front()}, false).error == Error::none && draft.size() == 3);
    const auto with_repeat = draft;
    assert(append_editor_effects(draft, {"twist_room_for_more(chapel)", "stamp_cost_2D_less(triumph)"}, false)
               .error == Error::debt_stamp && draft == with_repeat);
}

}

int main() {
    std::array<std::byte, 0x3508> profile{};
    using kingdom::EditorContext;
    const auto base = profile.data();
    assert(kingdom::editor_context(base, base + 0x10, 0, 0) == EditorContext::local);
    assert(kingdom::editor_context(base, base + 0x11B8, 1, 0) == EditorContext::online);
    assert(kingdom::editor_context(base, base + 0x2360, 1, 1) == EditorContext::online);
    assert(kingdom::editor_context(base, base + 0x11B8, 1, 1) == EditorContext::none);
    assert(kingdom::editor_context(base, base + 0x2360, 1, 0) == EditorContext::none);
    assert(kingdom::editor_context(base, base + 0x10, 1, 0) == EditorContext::none);
    assert(kingdom::editor_context(base, base + 0x11B8, 0, 0) == EditorContext::none);
    assert(kingdom::editor_context(base, base + 0x11B8, 2, 0) == EditorContext::none);
    assert(kingdom::editor_context(base, base + 0x11B8, 3, 0) == EditorContext::none);
    assert(kingdom::editor_context(nullptr, base + 0x10, 0, 0) == EditorContext::none);
    distinct_targets_and_multiplicity();
    capacity();
    automatic_doublings();
    shared_targets();
    text_limits();
    known_crashes();
    editor_selection();
    kingdom::EffectCommands commands;
    assert(kingdom::read_effect_commands(
        "-k village, market -x start_double_pile(village), twist_typhoon(village,market), "
        "start_double_pile(village) -m Landscapes(0,4)", commands));
    assert((commands == kingdom::EffectCommands{"start_double_pile(village)",
        "twist_typhoon(village,market)", "start_double_pile(village)"}));
    assert(kingdom::same_effect_commands(commands, {"start_double_pile(village)",
        "twist_typhoon(village, market)", "start_double_pile(village)"}));
    assert(!kingdom::same_effect_commands(commands, {"twist_typhoon(village,market)",
        "start_double_pile(village)", "start_double_pile(village)"}));
    assert(kingdom::effect_command_text(commands).find("-x start_double_pile(village)") == 0);
    assert(kingdom::read_effect_commands("-k village", commands) && commands.empty());
    assert(!kingdom::read_effect_commands("-x start_double_pile(village", commands));

    kingdom::Setup active{}, replacement{};
    kingdom::Effects old{}, edited{};
    old[0][0] = kingdom::room;
    edited[0][0] = kingdom::typhoon;
    kingdom::write_effects(active.data(), old);
    kingdom::write_effects(replacement.data(), edited);
    active[kingdom::room_offset + 4] = std::byte{7};
    active[4] = std::byte{9};
    replacement[kingdom::typhoon_offset + 4] = std::byte{3};
    kingdom::replace_effects(active, replacement);
    assert(kingdom::read_effects(active.data()) == edited);
    assert(active[kingdom::room_offset + 4] == std::byte{});
    assert(active[kingdom::typhoon_offset + 4] == std::byte{3});
    assert(active[4] == std::byte{9});
    kingdom::replace_effects(active, {});
    assert(kingdom::count(kingdom::read_effects(active.data())) == 0);
    assert(active[kingdom::typhoon_offset + 4] == std::byte{});
    std::cout << "Effect merge, shared targets and input limits passed.\n";
}
