#pragma once

#include "effects.hpp"
#include <limits>

namespace kingdom {

// Presence, not a second interpretation of Dominion's decoded options.
// Duplicate instructions retain native last-wins behavior through parsed.
struct ImportedSetupOptions {
    bool colonies = false;
    bool shelters = false;
    bool landscapes = false;
};

enum class ImportOptionsError { none, malformed, invalid_native_value };

constexpr std::size_t import_colonies_offset = 0xBFC;
constexpr std::size_t import_shelters_offset = 0xC00;
constexpr std::size_t import_landscapes_offset = 0xBC8;

inline bool import_option_name(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : c; };
        if (lower(a[i]) != lower(b[i])) return false;
    }
    return true;
}

inline bool import_landscape_arguments(std::string_view args) {
    unsigned values[2]{}, count = 0;
    std::size_t at = 0;
    for (;;) {
        if (count == 2) return false;
        const auto comma = args.find(',', at);
        const auto value = trimmed(args.substr(at, comma == args.npos ? args.npos : comma - at));
        if (value.empty()) return false;
        unsigned number = 0;
        for (char ch : value) {
            if (ch < '0' || ch > '9' || number > 4) return false;
            number = number * 10 + unsigned(ch - '0');
        }
        if (number > 4) return false;
        values[count++] = number;
        if (comma == args.npos) break;
        at = comma + 1;
    }
    return count == 1 || values[0] <= values[1];
}

// All supported presence tokens are top-level entries in -k or -m. Nested
// assignment/effect text must never turn into a setup option. Undocumented
// abbreviations and unrelated modifiers are deliberately not recognized.
inline ImportOptionsError scan_import_setup_options(std::string_view text, ImportedSetupOptions& output) {
    ImportedSetupOptions next{};
    unsigned depth = 0;
    char section = 'k'; // Native parser also accepts a bare card list.
    std::size_t start = 0;
    const auto finish = [&](std::size_t end) {
        const auto token = trimmed(text.substr(start, end - start));
        if (token.empty() || (section != 'k' && section != 'm')) return ImportOptionsError::none;
        const auto opening = token.find('(');
        const auto name = trimmed(token.substr(0, opening));
        const bool colonies = import_option_name(name, "colonies") || (section == 'm' && import_option_name(name, "NoColonies"));
        const bool shelters = import_option_name(name, "shelters") || (section == 'm' && import_option_name(name, "NoShelters"));
        if (colonies || shelters) {
            if (opening != token.npos) return ImportOptionsError::malformed;
            next.colonies |= colonies;
            next.shelters |= shelters;
        } else if (section == 'm' && import_option_name(name, "Landscapes")) {
            if (opening == token.npos || token.back() != ')' ||
                !import_landscape_arguments(token.substr(opening + 1, token.size() - opening - 2)))
                return ImportOptionsError::malformed;
            next.landscapes = true;
        }
        return ImportOptionsError::none;
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        const bool header = !depth && text[i] == '-' && (i == 0 || whitespace(text[i - 1])) &&
            i + 1 < text.size() && (i + 2 == text.size() || whitespace(text[i + 2])) &&
            ((text[i + 1] >= 'a' && text[i + 1] <= 'z') || (text[i + 1] >= 'A' && text[i + 1] <= 'Z'));
        if (header) {
            const auto error = finish(i);
            if (error != ImportOptionsError::none) return error;
            section = text[i + 1];
            if (section >= 'A' && section <= 'Z') section += 'a' - 'A';
            start = i + 2;
            ++i;
        } else if (text[i] == '(') {
            ++depth;
        } else if (text[i] == ')') {
            if (!depth) return ImportOptionsError::malformed;
            --depth;
        } else if (text[i] == ',' && !depth) {
            const auto error = finish(i);
            if (error != ImportOptionsError::none) return error;
            start = i + 1;
        }
    }
    if (depth) return ImportOptionsError::malformed;
    const auto error = finish(text.size());
    if (error == ImportOptionsError::none) output = next;
    return error;
}

inline ImportOptionsError apply_import_setup_options(void* destination, const void* parsed,
                                                     const ImportedSetupOptions& options) {
    if (!destination || !parsed) return ImportOptionsError::invalid_native_value;
    const auto* source = static_cast<const std::byte*>(parsed);
    auto* target = static_cast<std::byte*>(destination);
    const auto read = [&](std::size_t offset) { std::uint32_t value; std::memcpy(&value, source + offset, sizeof value); return value; };
    const auto colonies = read(import_colonies_offset), shelters = read(import_shelters_offset);
    const auto low = read(import_landscapes_offset), high = read(import_landscapes_offset + 4);
    if ((options.colonies && colonies != 1 && colonies != 2) ||
        (options.shelters && shelters != 1 && shelters != 2) ||
        (options.landscapes && (low > high || high > 4))) return ImportOptionsError::invalid_native_value;
    if (options.colonies) std::memcpy(target + import_colonies_offset, source + import_colonies_offset, 4);
    if (options.shelters) std::memcpy(target + import_shelters_offset, source + import_shelters_offset, 4);
    if (options.landscapes) std::memcpy(target + import_landscapes_offset, source + import_landscapes_offset, 8);
    return ImportOptionsError::none;
}

inline ImportOptionsError apply_import_setup_options(Setup& destination, const Setup& parsed,
                                                     const ImportedSetupOptions& options) {
    return apply_import_setup_options(destination.data(), parsed.data(), options);
}

}
