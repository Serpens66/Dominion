#pragma once
#include <array>
#include <cstdint>
namespace kingdom::optional_automation {
// Dominion 3315 / SHA256 56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877.
// Audited native display-card initializers RVA24B80..285B9 and effect registry.
// This is an identity mapping, not a grant, target or commutativity policy.
struct CampaignOrderSource {std::uint32_t display,effect;};
inline constexpr std::array<CampaignOrderSource,56> campaign_order_sources{{
    {0x1500,0x1771}, // twist_busy_busy; display registration RVA0x24B80
    {0x1501,0x1772}, // twist_shoppers; display registration RVA0x24C8F
    {0x1502,0x1773}, // twist_extra_oomph; display registration RVA0x24D9E
    {0x1503,0x1774}, // twist_big_ideas; display registration RVA0x24EAD
    {0x1504,0x1775}, // twist_penny_party; display registration RVA0x24FBC
    {0x1505,0x1776}, // twist_landfill; display registration RVA0x250CB
    {0x1506,0x1777}, // twist_rebates; display registration RVA0x251DA
    {0x1507,0x1778}, // twist_so_much_stuff; display registration RVA0x252E9
    {0x1508,0x17D4}, // twist_from_the_throne; display registration RVA0x253F8
    {0x1509,0x17D5}, // twist_head_start; display registration RVA0x25507
    {0x150A,0x17D6}, // twist_beakers; display registration RVA0x25616
    {0x150B,0x17D7}, // twist_sale; display registration RVA0x25720
    {0x150C,0x17D8}, // twist_room_for_more; display registration RVA0x25834
    {0x150D,0x17D9}, // twist_incoming; display registration RVA0x2593B
    {0x150E,0x17DA}, // twist_garbage_heap; display registration RVA0x25A52
    {0x150F,0x17DB}, // twist_pennies_saved; display registration RVA0x25B61
    {0x1510,0x17DC}, // twist_fast_track; display registration RVA0x25C70
    {0x1511,0x17DD}, // twist_real_estate_barons; display registration RVA0x25D7F
    {0x1512,0x17DE}, // twist_delayed_joy; display registration RVA0x25E8E
    {0x1513,0x17DF}, // twist_cramped; display registration RVA0x25F9D
    {0x1514,0x17E0}, // twist_double_up; display registration RVA0x260AC
    {0x1515,0x17E1}, // twist_turn_turn_turn; display registration RVA0x261BB
    {0x1516,0x17E2}, // twist_mirror_loot; display registration RVA0x262CA
    {0x1517,0x17E3}, // twist_typhoon; display registration RVA0x263D9
    {0x1518,0x1838}, // twist_favored_landlords; display registration RVA0x264E8
    {0x1519,0x1839}, // twist_favored_wealthy; display registration RVA0x265F7
    {0x151A,0x183A}, // twist_forever_favored; display registration RVA0x26706
    {0x151B,0x1B59}, // stamp_offers_insight; display registration RVA0x26815
    {0x151C,0x1B5A}, // stamp_two_for_one; display registration RVA0x2691C
    {0x151D,0x1B5B}, // stamp_discounted; display registration RVA0x26A33
    {0x151E,0x1B5C}, // stamp_victorified; display registration RVA0x26B42
    {0x151F,0x1B5D}, // stamp_durationified; display registration RVA0x26C51
    {0x1520,0x1B5E}, // stamp_treasureified; display registration RVA0x26D60
    {0x1521,0x1B61}, // stamp_plus_buy; display registration RVA0x26E6F
    {0x1522,0x1B62}, // stamp_plus_coin; display registration RVA0x26F7E
    {0x1523,0x1B63}, // stamp_plus_action; display registration RVA0x2708D
    {0x1524,0x1B64}, // stamp_plus_card; display registration RVA0x2719C
    {0x1525,0x1B65}, // stamp_chameleonize; display registration RVA0x272AB
    {0x1526,0x1B66}, // stamp_with_spoils; display registration RVA0x273BA
    {0x1527,0x1B67}, // stamp_with_villager; display registration RVA0x274C9
    {0x1528,0x1B68}, // stamp_with_horse; display registration RVA0x275D8
    {0x1529,0x1B69}, // stamp_pay_later; display registration RVA0x276E7
    {0x152A,0x141E}, // stamp_got_it_already; display registration RVA0x277F6
    {0x152B,0x141F}, // stamp_bonus_money; display registration RVA0x27900
    {0x152C,0x1421}, // stamp_one_track_mind; display registration RVA0x27A14
    {0x152D,0x1422}, // stamp_fawning_service; display registration RVA0x27B1B
    {0x152E,0x1426}, // stamp_perks_of_wealth; display registration RVA0x27C32
    {0x152F,0x1B5F}, // stamp_reactionified; display registration RVA0x27D41
    {0x1530,0x1425}, // stamp_also_silver; display registration RVA0x27F5F
    {0x1531,0x1427}, // stamp_motivational; display registration RVA0x27E50
    {0x1532,0x151A}, // stamp_coin_way; display registration RVA0x2806E
    {0x1533,0x1519}, // stamp_action_way; display registration RVA0x2817D
    {0x1534,0x1645}, // stamp_also_gold; display registration RVA0x2828C
    {0x1535,0x1646}, // stamp_also_plague; display registration RVA0x2839B
    {0x1536,0x1647}, // stamp_also_debt; display registration RVA0x284AA
    {0x1537,0x1648}, // stamp_also_respect; display registration RVA0x285B9
}};
}
