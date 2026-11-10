#include "effects.hpp"

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

}

int main() {
    distinct_targets_and_multiplicity();
    capacity();
    automatic_doublings();
    shared_targets();
    text_limits();
    std::cout << "Effect merge, shared targets and input limits passed.\n";
}
