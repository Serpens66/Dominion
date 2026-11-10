#pragma once

#include <windows.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

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
};

using CardTargets = std::vector<const CardTarget*>;
enum class CardSelection { single, multiple, ordered };

std::wstring fold_card_search(std::wstring_view text);

// Return an error to keep the picker and its selection open.
using AcceptCards = std::function<std::wstring(const CardTargets&, bool repeat)>;
void pick_cards(HWND owner, const wchar_t* title, const CardTargets& cards, const CardTargets& current_kingdom,
                CardSelection mode, const AcceptCards& accept);

}
