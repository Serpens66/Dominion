#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace kingdom {

enum class CardAccess { allowed, denied, unknown };
using CardAccessQuery = std::function<CardAccess(std::string_view)>;

// Generated from the same argument metadata used by the editor. All 89
// effects have a row, including effects without card/landscape arguments.
struct TargetAccessRule {
    const char* id;
    unsigned minimum_targets;
    unsigned maximum_targets;
};

struct TargetAccessFailure {
    CardAccess state = CardAccess::allowed;
    std::string effect_id;
    std::string target_id;
    bool malformed = false;

    explicit operator bool() const { return state != CardAccess::allowed || malformed; }
};

inline std::string_view access_trim(std::string_view value) {
    const auto space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!value.empty() && space(value.front())) value.remove_prefix(1);
    while (!value.empty() && space(value.back())) value.remove_suffix(1);
    return value;
}

inline bool access_identifier(std::string_view value) {
    if (value.empty() || (value.front() >= '0' && value.front() <= '9')) return false;
    for (const char c : value) {
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') && c != '_') return false;
    }
    return true;
}

// Never changes the draft, trims a target list or drops following effects.
// Unknown effects/argument shapes fail closed rather than guessing that a
// number, effect parameter or an unrecognised native value is a card ID.
inline TargetAccessFailure validate_target_access(
    const std::vector<std::string>& commands, const TargetAccessRule* rules,
    std::size_t rule_count, const CardAccessQuery& query) {
    for (const auto& command : commands) {
        const auto value = access_trim(command);
        const auto opening = value.find('(');
        const auto id = access_trim(value.substr(0, opening));
        const auto fail = [&](std::string_view target, bool malformed) {
            return TargetAccessFailure{CardAccess::unknown, std::string(id), std::string(target), malformed};
        };
        if (!access_identifier(id)) return fail({}, true);
        const TargetAccessRule* rule = nullptr;
        for (std::size_t i = 0; i < rule_count; ++i) {
            if (id == rules[i].id) { rule = rules + i; break; }
        }
        if (!rule) return fail({}, false);
        if (opening == value.npos) {
            if (rule->minimum_targets) return fail({}, true);
            continue;
        }
        if (value.back() != ')' || value.find('(', opening + 1) != value.npos ||
            value.find(')', opening + 1) != value.size() - 1) return fail({}, true);
        const auto body = access_trim(value.substr(opening + 1, value.size() - opening - 2));
        if (body.empty()) {
            if (rule->minimum_targets) return fail({}, true);
            continue;
        }
        std::vector<std::string_view> targets;
        std::size_t start = 0;
        for (;;) {
            const auto comma = body.find(',', start);
            const auto target = access_trim(body.substr(start, comma == body.npos ? body.npos : comma - start));
            if (!access_identifier(target)) return fail(target, true);
            targets.push_back(target);
            if (targets.size() > rule->maximum_targets) return fail(target, true);
            if (comma == body.npos) break;
            start = comma + 1;
        }
        if (targets.size() < rule->minimum_targets) return fail({}, true);
        for (const auto target : targets) {
            const auto state = query ? query(target) : CardAccess::unknown;
            if (state != CardAccess::allowed) return {state, std::string(id), std::string(target), false};
        }
    }
    return {};
}

template<std::size_t Count>
inline TargetAccessFailure validate_target_access(
    const std::vector<std::string>& commands, const TargetAccessRule (&rules)[Count],
    const CardAccessQuery& query) {
    return validate_target_access(commands, rules, Count, query);
}

}
