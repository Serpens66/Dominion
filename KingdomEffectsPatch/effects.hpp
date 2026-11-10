#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace kingdom {

constexpr std::size_t setup_size = 0x1990;
constexpr std::size_t effects_offset = 0x1710;
constexpr std::size_t maximum_effects = 32;
constexpr std::uint32_t room = 0x17D8;
constexpr std::uint32_t typhoon = 0x17E3;
constexpr std::size_t room_offset = 0x860;
constexpr std::size_t typhoon_offset = 0x710;
constexpr std::size_t room_bytes = 0x10;
constexpr std::size_t typhoon_bytes = 20 * 0x10;

using Effect = std::array<std::uint32_t, 5>;
using Effects = std::array<Effect, maximum_effects>;
using KingdomPiles = std::array<std::uint32_t, 10>;
static_assert(effects_offset + sizeof(Effects) == setup_size);
using Setup = std::array<std::byte, setup_size>;

enum class Error {
    none, capacity, shared_target, malformed, repeated_section, input_length, repeated_project_cube, debt_stamp
};

constexpr std::uint32_t double_pile = 0xDB8;
constexpr std::uint32_t triple_pile = 0xDBA;
constexpr std::uint32_t project_cube = 0x14B5;
constexpr std::uint32_t debt_stamp = 0x1423;

inline const wchar_t* known_failure_message(Error error) {
    switch (error) {
        case Error::repeated_project_cube:
            return L"Projektstein darf f\u00fcr dasselbe Projekt nur einmal vorkommen. Entferne die Wiederholung.";
        case Error::debt_stamp:
            return L"Der Stempel \u201e\u22122 Schuldenkosten\u201c kann in diesem Build den Spielstart abbrechen. Entferne ihn aus dem Set.";
        default:
            return nullptr;
    }
}

inline Effects read_effects(const void* setup) {
    Effects result{};
    std::memcpy(&result, static_cast<const std::byte*>(setup) + effects_offset, sizeof result);
    return result;
}

inline std::size_t count(const Effects& list) {
    return std::find_if(list.begin(), list.end(), [](const Effect& e) { return !e[0]; }) - list.begin();
}

inline Error known_failure(const Effects& list) {
    const auto end = list.begin() + count(list);
    for (auto it = list.begin(); it != end; ++it) {
        if ((*it)[0] == debt_stamp) return Error::debt_stamp;
        if ((*it)[0] == project_cube && std::any_of(list.begin(), it, [&](const Effect& previous) {
            return previous[0] == project_cube && previous[1] == (*it)[1];
        })) return Error::repeated_project_cube;
    }
    return Error::none;
}

inline Error append_doublings(Effects& effects, const KingdomPiles& piles) {
    auto result = effects;
    auto size = count(result);
    for (auto it = piles.begin(); it != piles.end(); ++it) {
        const auto card = *it;
        if (!card || std::find(piles.begin(), it, card) != it) return Error::malformed;
        const auto explicit_multiplier = [card](const Effect& effect) {
            return (effect[0] == double_pile || effect[0] == triple_pile) && effect[1] == card;
        };
        if (std::any_of(result.begin(), result.begin() + size, explicit_multiplier)) continue;
        if (size == maximum_effects) return Error::capacity;
        result[size++] = {double_pile, card, 0xFFFFFFFFu, 0, 0};
    }
    effects = result;
    return Error::none;
}

struct Merge {
    Effects list{};
    std::array<std::byte, room_bytes> room_targets{};
    std::array<std::byte, typhoon_bytes> typhoon_targets{};
    bool has_room = false;
    bool has_typhoon = false;
    Error error = Error::none;
};

template<std::size_t N>
bool collect_shared(const std::byte* current, const std::byte* incoming,
                    const Effects& existing, const Effects& added, std::uint32_t id,
                    std::size_t offset, std::array<std::byte, N>& result, bool& present) {
    const auto matches = [id](const Effect& effect) { return effect[0] == id; };
    const bool old_has = std::any_of(existing.begin(), existing.begin() + count(existing), matches);
    const bool new_has = std::any_of(added.begin(), added.begin() + count(added), matches);
    present = old_has || new_has;
    if (old_has && new_has && std::memcmp(current + offset, incoming + offset, N) != 0) {
        return false;
    }
    if (present) {
        std::memcpy(result.data(), (new_has ? incoming : current) + offset, N);
    }
    return true;
}

inline Merge merge_effects(const void* current, const void* incoming) {
    const auto* old_setup = static_cast<const std::byte*>(current);
    const auto* new_setup = static_cast<const std::byte*>(incoming);
    const auto old_list = read_effects(current);
    const auto new_list = read_effects(incoming);
    Merge result{};
    if (!collect_shared(old_setup, new_setup, old_list, new_list, room, room_offset,
                        result.room_targets, result.has_room) ||
        !collect_shared(old_setup, new_setup, old_list, new_list, typhoon, typhoon_offset,
                        result.typhoon_targets, result.has_typhoon)) {
        result.error = Error::shared_target;
        return result;
    }
    const auto old_count = count(old_list);
    const auto new_count = count(new_list);
    std::copy_n(old_list.begin(), old_count, result.list.begin());
    auto size = old_count;
    for (std::size_t i = 0; i < new_count; ++i) {
        const auto& effect = new_list[i];
        const auto previous = std::count(old_list.begin(), old_list.begin() + old_count, effect);
        const auto requested = std::count(new_list.begin(), new_list.begin() + i + 1, effect);
        const bool shared = effect[0] == room || effect[0] == typhoon;
        if (requested <= previous || (shared && requested > 1)) {
            continue;
        }
        if (size == maximum_effects) {
            result.error = Error::capacity;
            return result;
        }
        result.list[size++] = effect;
    }
    return result;
}

inline void write_effects(void* setup, const Effects& list) {
    std::memcpy(static_cast<std::byte*>(setup) + effects_offset, &list, sizeof list);
}

inline void apply_merge(void* setup, const Merge& merge) {
    auto* bytes = static_cast<std::byte*>(setup);
    write_effects(setup, merge.list);
    if (merge.has_room) {
        std::memcpy(bytes + room_offset, merge.room_targets.data(), room_bytes);
    }
    if (merge.has_typhoon) {
        std::memcpy(bytes + typhoon_offset, merge.typhoon_targets.data(), typhoon_bytes);
    }
}

inline bool whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

inline std::string_view trimmed(std::string_view s) {
    while (!s.empty() && whitespace(s.front())) s.remove_prefix(1);
    while (!s.empty() && whitespace(s.back())) s.remove_suffix(1);
    return s;
}

inline Error validate_text(std::string_view text, std::size_t maximum_length, bool guard_known_failures = false) {
    if (text.size() > maximum_length) return Error::input_length;
    bool in_extras = false;
    bool seen_extras = false;
    bool seen_room = false;
    bool seen_typhoon = false;
    unsigned depth = 0;
    unsigned entries = 0;
    std::array<std::string_view, maximum_effects> projects{};
    std::size_t project_count = 0;
    std::size_t start = 0;
    const auto finish_entry = [&](std::size_t end) {
        const auto entry = trimmed(text.substr(start, end - start));
        if (entry.empty() || depth) return Error::malformed;
        if (++entries > maximum_effects) return Error::capacity;
        const auto opening = entry.find('(');
        const auto name = trimmed(entry.substr(0, opening));
        if (name.empty() || (opening != entry.npos && entry.back() != ')')) return Error::malformed;
        if (guard_known_failures) {
            if (name == "stamp_cost_2D_less" || name == "stamp_cost_2d_less") return Error::debt_stamp;
            if (name == "start_with_cube" && opening != entry.npos) {
                const auto target = trimmed(entry.substr(opening + 1, entry.size() - opening - 2));
                const auto previous = projects.begin() + project_count;
                if (std::find(projects.begin(), previous, target) != previous) return Error::repeated_project_cube;
                projects[project_count++] = target;
            }
        }
        if (name == "twist_room_for_more") {
            if (seen_room) return Error::shared_target;
            seen_room = true;
        }
        if (name == "twist_typhoon") {
            if (seen_typhoon) return Error::shared_target;
            seen_typhoon = true;
        }
        const bool needs_target = name == "start_double_pile" || name == "start_triple_pile" ||
                                  name == "start_replace_copper" || name == "start_make_split_pile" ||
                                  name == "start_shuffle_pile" || name == "twist_room_for_more" ||
                                  name == "twist_typhoon";
        if (needs_target && (opening == entry.npos ||
                            trimmed(entry.substr(opening + 1, entry.size() - opening - 2)).empty())) {
            return Error::malformed;
        }
        return Error::none;
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        const bool section = !depth && text[i] == '-' && (i == 0 || whitespace(text[i - 1])) &&
                             i + 1 < text.size() && (i + 2 == text.size() || whitespace(text[i + 2]));
        if (section) {
            if (in_extras) {
                const auto error = finish_entry(i);
                if (error != Error::none) return error;
            }
            in_extras = text[i + 1] == 'x' || text[i + 1] == 'X';
            if (in_extras && seen_extras) return Error::repeated_section;
            seen_extras |= in_extras;
            start = i + 2;
            ++i;
            continue;
        }
        if (!in_extras) continue;
        switch (text[i]) {
            case '(':
                if (++depth > 1) return Error::malformed;
                break;
            case ')':
                if (!depth) return Error::malformed;
                --depth;
                break;
            case ',':
                if (!depth) {
                    const auto error = finish_entry(i);
                    if (error != Error::none) return error;
                    start = i + 1;
                }
                break;
        }
    }
    return in_extras ? finish_entry(text.size()) : Error::none;
}

}
