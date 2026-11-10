#pragma once

#include "effects.hpp"

#include <functional>
#include <string>
#include <vector>

namespace kingdom {

using EffectCommands = std::vector<std::string>;

// The game emits each option as "-<letter>" and separates effects with
// top-level commas. Keep the native argument spelling for existing entries.
inline bool read_effect_commands(std::string_view text, EffectCommands& commands) {
    if (validate_text(text, 4095) != Error::none) return false;
    commands.clear();
    bool in_extras = false;
    bool in_arguments = false;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        const bool end = i == text.size();
        const bool section = !end && !in_arguments && text[i] == '-' &&
            (i == 0 || whitespace(text[i - 1])) && i + 1 < text.size() &&
            (i + 2 == text.size() || whitespace(text[i + 2]));
        if (in_extras && (end || section || (!in_arguments && text[i] == ','))) {
            commands.emplace_back(trimmed(text.substr(start, i - start)));
            start = i + 1;
        }
        if (end) break;
        if (section) {
            in_extras = text[i + 1] == 'x' || text[i + 1] == 'X';
            start = i + 2;
            ++i;
        } else if (in_extras) {
            if (text[i] == '(') in_arguments = true;
            if (text[i] == ')') in_arguments = false;
        }
    }
    return true;
}

inline std::string effect_command_text(const EffectCommands& commands) {
    std::string text;
    for (const auto& command : commands) {
        text += text.empty() ? "-x " : ", ";
        text += command;
    }
    return text;
}

inline bool same_effect_commands(EffectCommands a, EffectCommands b) {
    for (auto* commands : {&a, &b}) {
        for (auto& command : *commands) {
            command.erase(std::remove_if(command.begin(), command.end(), whitespace), command.end());
        }
    }
    return a == b;
}

inline void replace_effects(Setup& destination, const Setup& parsed) {
    const auto old = read_effects(destination.data());
    const auto next = read_effects(parsed.data());
    for (const auto& field : {std::array<std::size_t, 3>{room, room_offset, room_bytes},
                              std::array<std::size_t, 3>{typhoon, typhoon_offset, typhoon_bytes}}) {
        const auto present = [&field](const Effect& effect) { return effect[0] == field[0]; };
        if (std::any_of(next.begin(), next.end(), present)) {
            std::memcpy(destination.data() + field[1], parsed.data() + field[1], field[2]);
        } else if (std::any_of(old.begin(), old.end(), present)) {
            std::memset(destination.data() + field[1], 0, field[2]);
        }
    }
    write_effects(destination.data(), next);
}

// Returns true only after the user accepts the draft.
bool edit_effects(void* owner, const EffectCommands& effects, bool& automatic,
                  const std::function<bool(const EffectCommands&)>& apply);

}
