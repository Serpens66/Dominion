#pragma once

#include <windows.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include "ui_language.hpp"

namespace kingdom {

struct CardTarget {
    const char* id;
    const wchar_t* title;
    const wchar_t* expansion;
    const wchar_t* search;
    const wchar_t* types;
    const wchar_t* cost;
    const wchar_t* rules;
    const char* expansion_id;
    const char* type_icons;
    int coins;
    int potions;
    int debt;
    unsigned kinds;
    const wchar_t *title_en=L"", *expansion_en=L"", *types_en=L"", *cost_en=L"", *rules_en=L"";
};
inline const wchar_t* card_title(const CardTarget& card) {return ui::english() && *card.title_en?card.title_en:card.title;}
inline const wchar_t* card_expansion(const CardTarget& card) {return ui::english() && *card.expansion_en?card.expansion_en:card.expansion;}
inline const wchar_t* card_types(const CardTarget& card) {return ui::english() && *card.types_en?card.types_en:card.types;}
inline const wchar_t* card_cost(const CardTarget& card) {return ui::english() && *card.cost_en?card.cost_en:card.cost;}
inline const wchar_t* card_rules(const CardTarget& card) {return ui::english() && *card.rules_en?card.rules_en:card.rules;}

using CardTargets = std::vector<const CardTarget*>;
enum class CardSelection { single, multiple, ordered };

std::wstring fold_card_search(std::wstring_view text);

// Return an error to keep the picker and its selection open.
using AcceptCards = std::function<std::wstring(const CardTargets&, bool repeat)>;
void pick_cards(HWND owner, const wchar_t* title, const CardTargets& cards, const CardTargets& current_kingdom,
                CardSelection mode, const AcceptCards& accept);

}
