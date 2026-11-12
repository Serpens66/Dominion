#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <intrin.h>

#include "effects.hpp"
#include "effect_editor.hpp"
#include "ui_language.hpp"
#include "effect_text_fix.hpp"
#include "ban_hint_fix.hpp"

#include <cstdio>
#include <vector>

static_assert(sizeof(void*) == 4, "Dominion build 3315 uses x86");

namespace {

std::uintptr_t native_parser;
std::uintptr_t native_local_normalize;
std::uintptr_t native_effect_requirements;
std::uintptr_t native_effect_advance;
std::uintptr_t native_clipboard;
std::uintptr_t native_merge;
std::uintptr_t native_serializer;
std::uintptr_t native_played_serializer;
std::uintptr_t native_saved_serializer;
std::uintptr_t native_setup_serializer;
std::uintptr_t native_string_construct;
std::uintptr_t native_local_load;
std::uintptr_t native_extras_serializer;
std::uintptr_t native_online_filter;
std::uintptr_t native_create_message;
std::uintptr_t native_game_desc;
std::uintptr_t native_game_state_init;
std::uintptr_t native_active_setup;
std::uintptr_t native_string_assign;
std::uintptr_t native_save_profiles;
const std::byte* image_base;
std::uintptr_t search_cancel;
std::uintptr_t create_cancel;
std::uintptr_t local_dispatch_continue;
std::uintptr_t local_dispatch_cancel;
std::uintptr_t merge_original_tail;
std::uintptr_t merge_serialization;
bool measure_only;
bool automatic_enabled;
bool local_safety_enabled;
volatile LONG initialized;
wchar_t log_path[MAX_PATH];

struct ImportState {
    void* destination;
    const void* source;
    kingdom::Merge result;
};

thread_local ImportState pending{};
thread_local bool from_clipboard;
thread_local bool clipboard_rejected;
thread_local const std::byte* automatic_save;
thread_local const wchar_t* automatic_failure;
thread_local kingdom::Effects expected_local_effects{};
thread_local bool expected_local_valid=false;
thread_local unsigned local_normalize_depth=0;
void __cdecl begin_local_normalize() { ++local_normalize_depth; }
void __cdecl end_local_normalize() { --local_normalize_depth; }
bool __cdecl keep_explicit_local_effects() { return local_normalize_depth!=0; }
void log_commands(const char*, const kingdom::EffectCommands&);
void trace_local_prepared(const std::byte*);
bool validate_native_import(const char*);

void log(const char* event, const kingdom::Effects* list = nullptr) {
    char line[3072];
    int used = std::snprintf(line, sizeof line, "%s", event);
    if (list) {
        const auto n = kingdom::count(*list);
        used += std::snprintf(line + used, sizeof line - used, " (%zu)", n);
        for (std::size_t i = 0; i < n; ++i) {
            const auto& e = (*list)[i];
            used += std::snprintf(line + used, sizeof line - used, " %X(%X,%X,%X,%X)",
                                  e[0], e[1], e[2], e[3], e[4]);
        }
    }
    if (used < 0 || used + 2 >= sizeof line) return;
    line[used++] = '\r';
    line[used++] = '\n';
    const HANDLE file = CreateFileW(log_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(file, line, static_cast<DWORD>(used), &written, nullptr);
    CloseHandle(file);
}

void reject(kingdom::Error error) {
    if (const auto* known = kingdom::known_failure_message(error)) {
        log(error==kingdom::Error::project_cube_capacity?"rejected project cube capacity":
            error==kingdom::Error::repeated_project_cube?"rejected repeated project cube":"rejected debt stamp");
        MessageBoxW(GetActiveWindow(), kingdom::ui::tr(known), L"Dominion Kingdom Effects Patch", MB_OK | MB_ICONWARNING);
        return;
    }
    const wchar_t* message = L"Der Effektabschnitt konnte nicht gelesen werden.";
    switch (error) {
        case kingdom::Error::capacity:
            message = L"Das Set w\u00fcrde mehr als 32 Effekte enthalten. Bitte k\u00fcrze die Liste.";
            break;
        case kingdom::Error::shared_target:
            message = L"Zwei Effekte belegen dasselbe gemeinsame Zielfeld unterschiedlich. "
                      L"Entferne einen davon oder beginne mit einem leeren Set.";
            break;
        case kingdom::Error::repeated_section:
            message = L"Verwende einen -x-Abschnitt mit kommagetrennten Effekten.";
            break;
        case kingdom::Error::input_length:
            message = L"Der Kingdom-Text ist zu lang f\u00fcr diesen Import.";
            break;
        default:
            break;
    }
    log("import rejected");
    MessageBoxW(GetActiveWindow(), kingdom::ui::tr(message), L"Dominion Kingdom Effects Patch", MB_OK | MB_ICONWARNING);
}

bool validate(const char* text) {
    if (!text) return true;
    constexpr std::size_t maximum_length = 4095;
    const auto size = strnlen_s(text, maximum_length + 1);
    const auto error = kingdom::validate_text({text, size}, maximum_length, local_safety_enabled);
    if (error == kingdom::Error::none) return true;
    reject(error);
    return false;
}

bool __cdecl validate_search(const char* text) {
    const bool valid=validate(text);
    kingdom::EffectCommands commands;
    if (valid && text && kingdom::read_effect_commands(text,commands)) log_commands("import requested",commands);
    return valid && validate_native_import(text);
}

void __cdecl begin_clipboard() {
    from_clipboard = true;
    clipboard_rejected = false;
}

bool __cdecl validate_clipboard(const char* text) {
    if (!from_clipboard) return true;
    if (validate_search(text)) return true;
    clipboard_rejected = true;
    return false;
}

bool __cdecl end_clipboard() {
    from_clipboard = false;
    return clipboard_rejected;
}

bool __cdecl prepare_merge(void* current, const void* incoming) {
    pending = {};
    const auto added = kingdom::read_effects(incoming);
    if (kingdom::count(added) == 0) return true;
    const auto existing = kingdom::read_effects(current);
    log("import existing", &existing);
    log("import incoming", &added);
    auto result = kingdom::merge_effects(current, incoming);
    if (result.error != kingdom::Error::none) {
        reject(result.error);
        return false;
    }
    if (local_safety_enabled) {
        const auto error = kingdom::known_failure(result.list);
        if (error != kingdom::Error::none) {
            reject(error);
            return false;
        }
    }
    log(measure_only ? "import would write" : "import prepared", &result.list);
    if (!measure_only) pending = {current, incoming, result};
    return true;
}

bool __cdecl commit_merge(void* current, const void* incoming) {
    if (pending.destination != current || pending.source != incoming) return false;
    kingdom::apply_merge(current, pending.result);
    log("import wrote", &pending.result.list);
    pending = {};
    return true;
}

void __cdecl apply_saved(void* config, void* combined, const void* selected) {
    std::uint32_t type = 0;
    std::memcpy(&type, static_cast<const std::byte*>(config) + 0xc, sizeof type);
    if (type != 1) return;
    const auto previous = kingdom::read_effects(combined);
    const auto next = kingdom::read_effects(selected);
    log("set previous", &previous);
    log(measure_only ? "set would write" : "set selected", &next);
    if (!measure_only) kingdom::write_effects(combined, next);
}

const char* __cdecl played_setup_with_effects() {
    const auto* client = *reinterpret_cast<const std::byte* const*>(image_base + 0x8C8D5C);
    if (!client || *reinterpret_cast<const DWORD*>(client + 0x5068) != 1) return nullptr;

    // The local ActiveGame branch returns client + 0x5098. Its XString at
    // +0x3C is the resolved kingdomData also written to the local game XML.
    const auto* field = reinterpret_cast<const char* const*>(client + 0x5098 + 0x3C);
    const char* text = *field;
    if (!text) return nullptr;
    constexpr std::size_t limit = 4095;
    const std::string_view kingdom_text(text, strnlen_s(text, limit + 1));
    if ((kingdom_text.substr(0, 3) != "-x " && kingdom_text.find(" -x ") == kingdom_text.npos) ||
        kingdom::validate_text(kingdom_text, limit) != kingdom::Error::none) return nullptr;
    log(measure_only ? "save played set would keep effects" : "save played set kept effects");
    return measure_only ? nullptr : text;
}

bool __cdecl setup_has_effects(const void* setup) {
    const auto effects = kingdom::read_effects(setup);
    if (!kingdom::count(effects)) return false;
    log(measure_only ? "save prepared set would keep effects" : "save prepared set kept effects", &effects);
    return !measure_only;
}

const std::byte* active_profile() {
    const auto* settings = *reinterpret_cast<const std::byte* const*>(image_base + 0x8C8DC8);
    if (!settings) return nullptr;
    const auto slots = *reinterpret_cast<const DWORD*>(settings + 0x1E1A0);
    if (!slots || slots > 4) return nullptr;
    const auto active = *reinterpret_cast<const DWORD*>(settings + 0x1E1A4);
    const auto* profile = settings;
    for (DWORD i = 0; i < slots; ++i) {
        const auto* candidate = settings + i * 0x7868;
        if (*reinterpret_cast<const DWORD*>(candidate + 0x42B0) == active) {
            profile = candidate;
            break;
        }
    }
    return profile;
}

bool __cdecl allow_local_start() {
    expected_local_valid=false;
    const auto* profile = active_profile();
    if (!profile) return true;
    const auto* config = profile + 0x10;
    const auto* setup_text = *reinterpret_cast<const char* const*>(config + 0x30);
    const auto* kingdom_text = *reinterpret_cast<const char* const*>(config + 0x34);
    const char* selected = nullptr;
    if (*reinterpret_cast<const DWORD*>(config + 0x0C) == 1) {
        const auto index = *reinterpret_cast<const DWORD*>(config + 0x10);
        const auto count = *reinterpret_cast<const DWORD*>(profile + 0x71DC);
        if (index < count && index < 64) {
            selected = *reinterpret_cast<const char* const*>(profile + 0x6EE0 + index * 12);
        }
    }
    // Local games can start from a resolved kingdom, a setup, or a selected profile set.
    for (const auto* text : {selected, setup_text, kingdom_text}) {
        if (!text) continue;
        constexpr std::size_t limit = 4095;
        const auto error = kingdom::validate_text({text, strnlen_s(text, limit + 1)}, limit, true);
        if (kingdom::known_failure_message(error)) {
            reject(error);
            return false;
        }
    }
    trace_local_prepared(config);
    return true;
}

void __cdecl begin_automatic(const std::byte* save) {
    automatic_save = nullptr;
    automatic_failure = nullptr;
    if (!automatic_enabled) return;
    const auto* client = *reinterpret_cast<const std::byte* const*>(image_base + 0x8C8D5C);
    if (!client || save != client + 0x5098 || *reinterpret_cast<const DWORD*>(client + 0x5068) != 1) return;
    const auto selector = *reinterpret_cast<const DWORD*>(save + 0x1C);
    const auto flags = *reinterpret_cast<const DWORD*>(save + 0x20);
    if (selector || (flags & 0x104)) return;
    automatic_save = save;
}

void __cdecl append_automatic(void* native_extras, const void* rolled_piles, const std::byte* save) {
    kingdom::Effects generated{}; std::memcpy(&generated,native_extras,sizeof generated);
    log("generator effects before text output",&generated);
    if (!automatic_save || automatic_save != save) return;
    automatic_save = nullptr;
    kingdom::KingdomPiles piles{};
    kingdom::Effects effects{};
    std::memcpy(&piles, rolled_piles, sizeof piles);
    std::memcpy(&effects, native_extras, sizeof effects);
    switch (kingdom::append_doublings(effects, piles)) {
        case kingdom::Error::none:
            std::memcpy(native_extras, &effects, sizeof effects);
            log("automatic kingdom effects", &effects);
            return;
        case kingdom::Error::capacity:
            automatic_failure = L"Die zehn Stapel und die vorhandenen Regeln passen nicht in die "
                                L"32 Effektpl\u00e4tze. Die Partie verwendet die bisherigen Regeln.";
            return;
        default:
            automatic_failure = L"Die zehn Hauptstapel konnten nicht eindeutig bestimmt werden. "
                                L"Die Partie verwendet die bisherigen Regeln.";
            return;
    }
}

void __cdecl end_automatic() {
    automatic_save = nullptr;
    kingdom::Effects installed{};
    std::memcpy(&installed,image_base+0x780AD8+0x1280,sizeof installed);
    log("local installed effects",&installed);
    const auto expected=kingdom::count(expected_local_effects);
    const auto received=kingdom::count(installed);
    const auto compare=!expected_local_valid ? 0 : automatic_enabled ? expected : (std::max)(expected,received);
    for (std::size_t i=0;i<compare;++i) {
        if (expected_local_effects[i][0]!=installed[i][0]) {
            char line[180]; std::snprintf(line,sizeof line,"local effect mismatch at %zu: expected=%X installed=%X",i+1,
                expected_local_effects[i][0],installed[i][0]); log(line); break;
        }
    }
    expected_local_valid=false;
    const auto* failure = automatic_failure;
    automatic_failure = nullptr;
    if (failure) {
        log("automatic doubling skipped");
        MessageBoxW(GetActiveWindow(), kingdom::ui::tr(failure), L"Dominion Kingdom Effects Patch", MB_OK | MB_ICONWARNING);
    }
}

class NativeSetupText {
public:
    NativeSetupText(const kingdom::Setup& setup, DWORD version) {
        const auto* data = setup.data();
        auto* output = &text_;
        __asm {
            push 0
            push version
            mov edx, data
            mov ecx, output
            call dword ptr [native_setup_serializer]
            add esp, 8
        }
    }

    ~NativeSetupText() {
        const char* empty = nullptr;
        assign(&text_, &empty);
    }

    NativeSetupText(const NativeSetupText&) = delete;
    NativeSetupText& operator=(const NativeSetupText&) = delete;

    std::string_view view() const {
        return text_ ? std::string_view(text_, strnlen_s(text_, 4096)) : std::string_view{};
    }

    void write_to(std::byte* field) const { assign(field, &text_); }

private:
    static void assign(void* target, const char* const* source) {
        using Assign = void* (__thiscall*)(void*, const char* const*);
        reinterpret_cast<Assign>(native_string_assign)(target, source);
    }

    const char* text_{};
};

void log_commands(const char* stage, const kingdom::EffectCommands& commands) {
    for (std::size_t i=0;i<commands.size();++i) {
        char line[2300];
        std::snprintf(line,sizeof line,"%s [%zu] %.*s",stage,i+1,
            static_cast<int>((std::min)(commands[i].size(),std::size_t(2100))),commands[i].c_str());
        log(line);
    }
}

void log_config_snapshot(const char* stage,const std::byte* config) {
    char line[180];
    std::snprintf(line,sizeof line,"%s: selector=%lu index=%lu flags=%lX version=%lu",stage,
        *reinterpret_cast<const DWORD*>(config+0x0C),*reinterpret_cast<const DWORD*>(config+0x10),
        *reinterpret_cast<const DWORD*>(config+0x18),*reinterpret_cast<const DWORD*>(config+0x28));
    log(line);
    for(const auto offset:{0x30,0x34}) {
        const auto* text=*reinterpret_cast<const char* const*>(config+offset);
        kingdom::EffectCommands commands;
        if(text && kingdom::read_effect_commands({text,strnlen_s(text,4096)},commands)) {
            std::snprintf(line,sizeof line,"%s %s",stage,offset==0x30?"setupData":"kingdomData");
            log_commands(line,commands);
        }
    }
}

kingdom::Setup resolve_config(const std::byte* config) {
    kingdom::Setup result{}; auto* output=&result;
    __asm {
        push output
        mov ecx,config
        call dword ptr [native_active_setup]
        add esp,4
    }
    return result;
}

void trace_local_prepared(const std::byte* config) {
    log_config_snapshot("local start configuration",config);
    const auto setup=resolve_config(config);
    expected_local_effects=kingdom::read_effects(setup.data());
    expected_local_valid=true;
    log("local prepared effects",&expected_local_effects);
    const NativeSetupText text(setup,*reinterpret_cast<const DWORD*>(config+0x28));
    kingdom::EffectCommands commands;
    if (kingdom::read_effect_commands(text.view(),commands)) log_commands("local prepared",commands);
}

bool parsed_effects_match(const kingdom::Setup& parsed, const kingdom::EffectCommands& commands, std::size_t* mismatch=nullptr) {
    const auto list = kingdom::read_effects(parsed.data());
    const auto entries=kingdom::count(list);
    for (std::size_t i = 0; i < (std::min)(entries,commands.size()); ++i) {
        if(mismatch) *mismatch=i;
        const auto& effect = list[i];
        const auto& command = commands[i];
        // Compare identity and every target against an independent native parse.
        // A count-only check cannot detect replacement, reordering or a wrong target.
        using Parse=void (__fastcall*)(const char*,kingdom::Setup*);
        kingdom::Setup single{};
        const auto text=kingdom::effect_command_text({command});
        reinterpret_cast<Parse>(native_parser)(text.c_str(),&single);
        const auto expected=kingdom::read_effects(single.data());
        if (kingdom::count(expected)!=1 || effect!=expected[0]) {
            log_commands("effect mismatch",{command}); return false;
        }
        if (command.find('(') == command.npos) continue;
        if (effect[0] == kingdom::room || effect[0] == kingdom::typhoon) {
            const auto offset = effect[0] == kingdom::room ? kingdom::room_offset : kingdom::typhoon_offset;
            const auto targets = 1 + std::count(command.begin(), command.end(), ',');
            if (targets > (effect[0] == kingdom::room ? 1 : 20)) return false;
            for (std::ptrdiff_t n = 0; n < targets; ++n) {
                if (!*reinterpret_cast<const DWORD*>(parsed.data() + offset + n * 0x10 + 4)) return false;
            }
            const auto bytes=static_cast<std::size_t>(targets)*0x10;
            if (std::memcmp(parsed.data()+offset,single.data()+offset,bytes)!=0) return false;
        } else if (!effect[1] ||
                   (command.find(',') != command.npos && effect[2] == 0xFFFFFFFFu && !effect[4])) {
            return false;
        }
    }
    if(entries!=commands.size()) { if(mismatch) *mismatch=(std::min)(entries,commands.size()); return false; }
    return true;
}

bool validate_native_import(const char* text) {
    if(!text) return true;
    kingdom::EffectCommands commands;
    if(!kingdom::read_effect_commands(text,commands) || commands.empty()) return true;
    using Parse=void (__fastcall*)(const char*,kingdom::Setup*);
    kingdom::Setup parsed{}; reinterpret_cast<Parse>(native_parser)(text,&parsed);
    const auto effects=kingdom::read_effects(parsed.data()); log("import parsed",&effects);
    std::size_t mismatch=0;
    if(parsed_effects_match(parsed,commands,&mismatch)) return true;
    const auto& command=commands[(std::min)(mismatch,commands.size()-1)];
    const std::wstring message=kingdom::ui::tr(L"Der Import wurde bei Effekt ")+std::to_wstring(mismatch+1)+L" ("+
        std::wstring(command.begin(),command.end())+kingdom::ui::tr(L") nicht vollständig eingelesen. Die bisherige Auswahl bleibt erhalten.");
    log_commands("import rejected mismatch",{command});
    MessageBoxW(GetActiveWindow(),message.c_str(),kingdom::ui::tr(L"Dominion – Königreich-Effekte"),MB_OK|MB_ICONWARNING);
    return false;
}

// Shared production apply path, also exercised directly by the isolated native tests.
bool apply_editor_effects(kingdom::Setup* current,std::byte* config,
                         const kingdom::EffectCommands& effects,const kingdom::EffectCommands& edited,
                         std::wstring& apply_error) {
    const DWORD version=*reinterpret_cast<const DWORD*>(config+0x28);
    apply_error.clear();
    if (edited == effects) return true;
    std::size_t mismatch=0;
    const auto fail=[&](const wchar_t* stage) {
        if(edited.empty()) { apply_error=std::wstring(kingdom::ui::tr(L"Leere Effektliste: "))+stage+kingdom::ui::tr(L". Die bisherige Auswahl bleibt erhalten."); return false; }
        const auto& command=edited[(std::min)(mismatch,edited.size()-1)];
        apply_error=std::wstring(kingdom::ui::tr(L"Effekt "))+std::to_wstring(mismatch+1)+L" ("+
            std::wstring(command.begin(),command.end())+L"): "+stage+kingdom::ui::tr(L". Die bisherige Auswahl bleibt erhalten.");
        log_commands("editor rejected",{command}); return false;
    };
    const auto commands = kingdom::effect_command_text(edited);
    log_commands("editor requested",edited);
    const auto validation=kingdom::validate_text(commands,4095,true);
    if(validation!=kingdom::Error::none) {
        if(const auto* known=kingdom::known_failure_message(validation)) apply_error=kingdom::ui::tr(known);
        else apply_error=kingdom::ui::tr(L"Diese Effektliste kann das Spiel nicht übernehmen.");
        return false;
    }
    using Parse = void (__fastcall*)(const char*, kingdom::Setup*);
    const auto parse = reinterpret_cast<Parse>(native_parser);
    kingdom::Setup parsed{};
    parse(commands.c_str(), &parsed);
    const auto parsed_list=kingdom::read_effects(parsed.data());
    log("editor parsed",&parsed_list);
    if (!parsed_effects_match(parsed, edited,&mismatch)) return fail(kingdom::ui::tr(L"beim Einlesen nicht vollständig übernommen"));

    auto next = *current;
    kingdom::replace_effects(next, parsed);
    const NativeSetupText output(next, version);
    kingdom::EffectCommands serialized;
    if (!kingdom::read_effect_commands(output.view(), serialized) ||
        !kingdom::same_effect_commands(serialized, edited)) {
        mismatch=0;
        while(mismatch<(std::min)(serialized.size(),edited.size()) && kingdom::same_effect_commands({serialized[mismatch]},{edited[mismatch]})) ++mismatch;
        log_commands("editor serialized mismatch",serialized); return fail(kingdom::ui::tr(L"bei der Ausgabe verändert"));
    }
    log_commands("editor serialized",serialized);
    // Resolve an independent custom configuration before touching the real one.
    // All strings are borrowed read-only; this path never selects a preset.
    std::array<std::byte,0x38> candidate{};
    std::memcpy(candidate.data(),config,candidate.size());
    *reinterpret_cast<DWORD*>(candidate.data()+0x0C)=0;
    *reinterpret_cast<const char**>(candidate.data()+0x30)=output.view().data();
    const auto resolved=resolve_config(candidate.data());
    const auto resolved_list=kingdom::read_effects(resolved.data()); log("editor resolved",&resolved_list);
    if (!parsed_effects_match(resolved,edited,&mismatch)) return fail(kingdom::ui::tr(L"beim erneuten Laden verändert"));
    // Detach the edited draft from the selected preset as the native
    // custom setup path does. The user can then save it as a set.
    output.write_to(config + 0x30);
    *reinterpret_cast<DWORD*>(config + 0x0C) = 0;
    *reinterpret_cast<DWORD*>(config + 0x10) = 0;
    // Preserve private/public status, just as Clear Kingdom now does.
    *reinterpret_cast<DWORD*>(config + 0x18) &= ~0x10u;
    *current = next;
    using SaveProfiles = void (__fastcall*)(void*);
    auto* settings = *reinterpret_cast<void* const*>(image_base + 0x8C8DC8);
    reinterpret_cast<SaveProfiles>(native_save_profiles)(settings);
    const auto list = kingdom::read_effects(next.data());
    log("effect editor applied", &list);
    log_config_snapshot("editor committed configuration",config);
    return true;
}

void __cdecl maybe_edit_effects(kingdom::Setup* current, std::byte* config) {
    static bool held = false;
    const bool pressed = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    const bool open = pressed && !held;
    held = pressed;
    if (!open) return;

    const HWND owner = GetActiveWindow();
    const auto* profile = active_profile();
    if (!owner || owner != GetForegroundWindow() || !profile) return;
    const auto screen = *reinterpret_cast<const DWORD*>(image_base + 0x4DBF38);
    const auto flags = *reinterpret_cast<const DWORD*>(image_base + 0x4DBF40);
    const auto context = kingdom::editor_context(profile, config, screen, flags);
    if (context == kingdom::EditorContext::none) {
        char event[160];
        std::snprintf(event, sizeof event, "effect editor blocked: screen=%lu flags=%lX local=%d online1=%d online2=%d",
                      screen, flags, int(config == profile + 0x10), int(config == profile + 0x11B8), int(config == profile + 0x2360));
        log(event);
        return;
    }
    const auto* client = *reinterpret_cast<const std::byte* const*>(image_base + 0x8C8D5C);
    if (!client || *reinterpret_cast<const DWORD*>(client + 0x5068)) {
        log("effect editor blocked: no idle client");
        return;
    }
    try {
        log_config_snapshot("editor configuration",config);
        const DWORD version = *reinterpret_cast<const DWORD*>(config + 0x28);
        const NativeSetupText original(*current, version);
        const auto opened_list=kingdom::read_effects(current->data()); log("editor configuration effects",&opened_list);
        kingdom::EffectCommands effects;
        if (!kingdom::read_effect_commands(original.view(), effects) ||
            !parsed_effects_match(*current, effects)) {
            throw kingdom::Error::malformed;
        }
        bool automatic = automatic_enabled;
        std::wstring apply_error;
        const auto apply = [&](const kingdom::EffectCommands& edited) {
            return apply_editor_effects(current,config,effects,edited,apply_error);
        };
        log(context == kingdom::EditorContext::online ? "effect editor opened: online" : "effect editor opened: local");
        log_commands("editor opened",effects);
        if (kingdom::edit_effects(owner, original.view(), effects, automatic, apply, context,&apply_error) &&
            context == kingdom::EditorContext::local) {
            automatic_enabled = automatic;
        }
    } catch (...) {
        MessageBoxW(owner, kingdom::ui::tr(L"Dominion konnte diese Effektliste nicht übernehmen. Die bisherige Auswahl bleibt erhalten."),
                    kingdom::ui::tr(L"Dominion – Königreich-Effekte"), MB_OK | MB_ICONWARNING);
    }
}

void log_online_text(const char* stage, const char* field, const char* data) {
    constexpr std::size_t limit = 4096;
    const std::string_view text(data ? data : "", data ? strnlen_s(data, limit) : 0);
    const char* extras = "absent";
    bool village = false;
    bool market = false;
    if (text.size() == limit) {
        extras = "over-limit";
    } else if (text.empty()) {
        extras = "empty-field";
    } else {
        // Log the presence of the test effects, never the network string itself.
        bool in_extras = false;
        std::size_t start = 0;
        for (std::size_t i = 0; i <= text.size(); ++i) {
            const bool end = i == text.size();
            const bool section = !end && text[i] == '-' && (i == 0 || kingdom::whitespace(text[i - 1])) &&
                                 i + 1 < text.size() && (i + 2 == text.size() || kingdom::whitespace(text[i + 2]));
            if (!end && !section) continue;
            if (in_extras) {
                const auto rules = text.substr(start, i - start);
                village |= rules.find("start_double_pile(village)") != rules.npos;
                market |= rules.find("start_double_pile(market)") != rules.npos;
            }
            if (end) break;
            in_extras = text[i + 1] == 'x' || text[i + 1] == 'X';
            if (in_extras) extras = "present";
            start = i + 2;
        }
    }
    char event[192];
    std::snprintf(event, sizeof event, "online %s %s: extras=%s village=%d market=%d",
                  stage, field, extras, static_cast<int>(village), static_cast<int>(market));
    log(event);
}

void __cdecl trace_online_before_filter(const std::byte* config) {
    log_online_text("before filter", "kingdomData", *reinterpret_cast<const char* const*>(config + 0x30));
}

void __cdecl trace_online_send(const std::byte* message) {
    log_online_text("send create", "setupData", *reinterpret_cast<const char* const*>(message + 0x28));
    log_online_text("send create", "kingdomData", *reinterpret_cast<const char* const*>(message + 0x30));
}

void __cdecl trace_online_receive(const std::byte* description) {
    log_online_text("receive description", "setupData", *reinterpret_cast<const char* const*>(description + 0x28));
    log_online_text("receive description", "kingdomData", *reinterpret_cast<const char* const*>(description + 0x30));
}

void __cdecl trace_online_load(const std::byte* save) {
    log_online_text("load network game", "setupData", *reinterpret_cast<const char* const*>(save + 0x38));
    log_online_text("load network game", "kingdomData", *reinterpret_cast<const char* const*>(save + 0x3C));
}

void __cdecl trace_online_effects(const std::byte* state) {
    const auto* client = *reinterpret_cast<const std::byte* const*>(image_base + 0x8C8D5C);
    if (!client || *reinterpret_cast<const DWORD*>(client + 0x5068) != 2) return;
    kingdom::Effects effects;
    std::memcpy(&effects, state + 0x1280, sizeof effects);
    log("online installed effects", &effects);
}

// Native call arguments and all registers are live until the original CALL.
__declspec(naked) void effect_editor_hook() {
    __asm {
        // The original setup resolver consumes ECX and one stack argument
        // which the caller still owns. Observe its initialized output.
        push dword ptr [esp + 4]
        call dword ptr [native_active_setup]
        add esp, 4
        pushfd
        pushad
        push dword ptr [ebp + 8]
        push eax
        call maybe_edit_effects
        add esp, 8
        popad
        popfd
        ret
    }
}

__declspec(naked) void search_parser_hook() {
    __asm {
        pushfd
        pushad
        push esi
        call validate_search
        add esp, 4
        test al, al
        jz reject_search
        popad
        popfd
        jmp dword ptr [native_parser]
    reject_search:
        popad
        popfd
        add esp, 4
        jmp dword ptr [search_cancel]
    }
}

__declspec(naked) void clipboard_hook() {
    __asm {
        pushfd
        pushad
        call begin_clipboard
        popad
        popfd
        call dword ptr [native_clipboard]
        pushfd
        pushad
        call end_clipboard
        test al, al
        jnz rejected
        popad
        popfd
        ret
    rejected:
        popad
        popfd
        xor al, al
        ret
    }
}

__declspec(naked) void clipboard_parser_hook() {
    __asm {
        pushfd
        pushad
        push ecx
        call validate_clipboard
        add esp, 4
        test al, al
        jz rejected
        popad
        popfd
        jmp dword ptr [native_parser]
    rejected:
        popad
        popfd
        ret
    }
}

__declspec(naked) void search_merge_hook() {
    __asm {
        pushfd
        pushad
        push dword ptr [esp + 40]
        push edx
        call prepare_merge
        add esp, 8
        test al, al
        jz rejected
        popad
        popfd
        jmp dword ptr [native_merge]
    rejected:
        popad
        popfd
        ret
    }
}

__declspec(naked) void clipboard_merge_hook() {
    __asm {
        pushfd
        pushad
        push dword ptr [esp + 40]
        push edx
        call prepare_merge
        add esp, 8
        test al, al
        jz rejected
        popad
        popfd
        jmp dword ptr [native_merge]
    rejected:
        popad
        popfd
        add esp, 8
        jmp dword ptr [create_cancel]
    }
}

__declspec(naked) void merge_commit_hook() {
    __asm {
        pushfd
        pushad
        push edi
        push esi
        call commit_merge
        add esp, 8
        test al, al
        jz original
        popad
        popfd
        mov dword ptr [ebp - 30h], 0
        jmp dword ptr [merge_serialization]
    original:
        popad
        popfd
        add edi, 1710h
        jmp dword ptr [merge_original_tail]
    }
}

__declspec(naked) void set_hook() {
    __asm {
        pushfd
        pushad
        lea eax, [ebp - 19c0h]
        push eax
        push edx
        push ecx
        call apply_saved
        add esp, 12
        popad
        popfd
        jmp dword ptr [native_serializer]
    }
}

__declspec(naked) void save_played_hook() {
    __asm {
        pushfd
        pushad
        call played_setup_with_effects
        test eax, eax
        jz original
        mov [esp + 20], eax
        popad
        popfd
        // DomSaveSetString receives an uninitialized XString output slot.
        // Use the native constructor with ECX=output and EDX=UTF-8 text.
        jmp dword ptr [native_string_construct]
    original:
        popad
        popfd
        jmp dword ptr [native_played_serializer]
    }
}

__declspec(naked) void save_prepared_hook() {
    __asm {
        pushfd
        pushad
        push edx
        call setup_has_effects
        add esp, 4
        test al, al
        jz original
        popad
        popfd
        push 0
        push 17h
        call dword ptr [native_setup_serializer]
        add esp, 8
        ret
    original:
        popad
        popfd
        jmp dword ptr [native_saved_serializer]
    }
}

__declspec(naked) void local_load_hook() {
    __asm {
        pushfd
        pushad
        push ecx
        call begin_automatic
        add esp, 4
        popad
        popfd
        // Forward both native stack arguments. ECX and EDX still hold the
        // local save pointer and the native load argument.
        push dword ptr [esp + 8]
        push dword ptr [esp + 8]
        call dword ptr [native_local_load]
        add esp, 8
        pushfd
        pushad
        call end_automatic
        popad
        popfd
        ret
    }
}

__declspec(naked) void local_dispatch_hook() {
    __asm {
        mov dword ptr [edi + 5044h], 0
        pushfd
        cmp eax, 1
        jne resume_dispatch
        pushad
        call allow_local_start
        test al, al
        jnz allowed
        popad
        popfd
        jmp dword ptr [local_dispatch_cancel]
    allowed:
        popad
    resume_dispatch:
        popfd
        jmp dword ptr [local_dispatch_continue]
    }
}

__declspec(naked) void automatic_extras_hook() {
    __asm {
        pushfd
        pushad
        push dword ptr [ebp - 5944h]
        lea eax, [ebp - 750h]
        push eax
        push edx
        call append_automatic
        add esp, 12
        popad
        popfd
        jmp dword ptr [native_extras_serializer]
    }
}

__declspec(naked) void online_filter_hook() {
    __asm {
        pushfd
        pushad
        push ecx
        call trace_online_before_filter
        add esp, 4
        popad
        popfd
        jmp dword ptr [native_online_filter]
    }
}

__declspec(naked) void online_send_hook() {
    __asm {
        pushfd
        pushad
        push dword ptr [esp + 44] // return address, then descriptor and message
        call trace_online_send
        add esp, 4
        popad
        popfd
        jmp dword ptr [native_create_message]
    }
}

__declspec(naked) void online_receive_hook() {
    __asm {
        pushfd
        pushad
        push ecx
        call trace_online_receive
        add esp, 4
        popad
        popfd
        jmp dword ptr [native_game_desc]
    }
}

__declspec(naked) void online_load_hook() {
    __asm {
        pushfd
        pushad
        push ecx
        call trace_online_load
        add esp, 4
        popad
        popfd
        jmp dword ptr [native_local_load]
    }
}

__declspec(naked) void online_effects_hook() {
    __asm {
        pushfd
        pushad
        push edx
        call trace_online_effects
        add esp, 4
        popad
        popfd
        jmp dword ptr [native_game_state_init]
    }
}

// Only the local start caller enables this scope. Native card and target
// validation runs unchanged; only campaign-effect expansion metadata is skipped.
__declspec(naked) void local_normalize_hook() {
    __asm {
        pushfd
        pushad
        call begin_local_normalize
        popad
        popfd
        push dword ptr [esp+8]
        push dword ptr [esp+8]
        call dword ptr [native_local_normalize]
        add esp,8
        pushfd
        pushad
        call end_local_normalize
        popad
        popfd
        ret
    }
}
__declspec(naked) void effect_requirements_hook() {
    __asm {
        pushfd
        pushad
        call keep_explicit_local_effects
        test al,al
        jnz keep_effect
        popad
        popfd
        mov ecx,[ebp-19A8h]
        mov ecx,[ecx]
        jmp dword ptr [native_effect_requirements]
    keep_effect:
        popad
        popfd
        jmp dword ptr [native_effect_advance]
    }
}

struct Site {
    std::uintptr_t rva;
    const char* original;
    std::size_t length;
    void (*hook)();
    bool jump = false;
    unsigned modes = 0; // 0 enables the site in every mode.
    const char* replacement = nullptr;
};

constexpr unsigned local_modes = (1u << 2) | (1u << 3);
constexpr unsigned online_mode = 1u << 4;

const Site sites[] = {
    // This caption caller used the command ID as an invalid mixed-case translation key.
    {0x19B3D2, "\xE8\x59\x13\x0F\x00",5,kingdom::effect_text_fix::hook_address(),false,local_modes|online_mode},
    {0x19B68C, "\xE8\x9F\x10\x0F\x00",5,kingdom::effect_text_fix::hook_address(),false,local_modes|online_mode},
    // Clear Kingdom must retain private-game status. Bit 0x10 keeps its native reset.
    {0x1F70B5, "\x83\x66\x18\xCF", 4, nullptr, false, local_modes | online_mode,
               "\x83\x66\x18\xEF"},
    {0x0B1E6B,"\xE8\x80\xA0\x02\x00",5,local_normalize_hook,false,local_modes},
    {0x0DC2CD,"\x8B\x8D\x58\xE6\xFF\xFF\x8B\x09",8,effect_requirements_hook,true,local_modes},
    {0x1F6F17, "\xE8\x94\x6C\xEE\xFF", 5, search_parser_hook},
    {0x1FDFF0, "\xE8\xAB\xDE\x02\x00", 5, clipboard_hook},
    {0x22BF93, "\xE8\x18\x1C\xEB\xFF", 5, clipboard_parser_hook},
    {0x1F6F2F, "\xE8\x5C\xFB\xFF\xFF", 5, search_merge_hook},
    {0x1FE032, "\xE8\x59\x8A\xFF\xFF", 5, clipboard_merge_hook},
    {0x1F6C74, "\x81\xC7\x10\x17\x00\x00", 6, merge_commit_hook, true},
    {0x20FB27, "\xE8\x84\x51\xED\xFF", 5, set_hook},
    {0x1FCD6D, "\xE8\xAE\x2D\xEE\xFF", 5, save_played_hook},
    {0x1FCDAE, "\xE8\x7D\x3B\xEE\xFF", 5, save_prepared_hook},
    {0x1FFF35, "\xE8\x96\x4A\xEE\xFF", 5, effect_editor_hook, false, local_modes | online_mode},
    {0x0B24BF, "\xC7\x87\x44\x50\x00\x00\x00\x00\x00\x00", 10, local_dispatch_hook, true, local_modes},
    {0x0B1EC6, "\xE8\xA5\x21\x17\x00", 5, local_load_hook, false, local_modes},
    {0x1E9F57, "\xE8\x14\xB4\xEF\xFF", 5, automatic_extras_hook, false, local_modes},
    {0x1FFA03, "\xE8\xE8\xC4\xED\xFF", 5, online_filter_hook, false, online_mode},
    {0x1FFAF5, "\xE8\x26\xBC\x08\x00", 5, online_send_hook, false, online_mode},
    {0x0B7965, "\xE8\x86\xCF\xFF\xFF", 5, online_receive_hook, false, online_mode},
    {0x0B0DF3, "\xE8\x78\x32\x17\x00", 5, online_load_hook, false, online_mode},
    {0x1EA206, "\xE8\x65\xEF\x0B\x00", 5, online_effects_hook, false, online_mode},
    // Zero uses the native unbounded category limits, as for local bans.
    {0x208DB8, "\x6A\x05\x6A\x05", 4, nullptr, false, local_modes | online_mode,
               "\x6A\x00\x6A\x00"},
    {0x208EA3, "\x6A\x05\x6A\x05", 4, nullptr, false, local_modes | online_mode,
               "\x6A\x00\x6A\x00"},
    // This formatter has one reference, in the native online ban counter.
    {0x465480, "(%d/%d)", 8, nullptr, false, local_modes | online_mode, "(%d)\0\0\0"},
};

bool change_site(std::byte* base, const Site& site, bool install) {
    auto* address = base + site.rva;
    DWORD protection;
    if (!VirtualProtect(address, site.length, PAGE_EXECUTE_READWRITE, &protection)) return false;
    if (install && site.replacement) {
        std::memcpy(address, site.replacement, site.length);
    } else if (install) {
        const auto next = reinterpret_cast<std::uintptr_t>(address + 5);
        const auto offset = reinterpret_cast<std::uintptr_t>(site.hook) - next;
        const auto opcode = site.jump ? std::byte{0xE9} : std::byte{0xE8};
        std::array<std::byte, 10> replacement;
        replacement.fill(std::byte{0x90});
        replacement[0] = opcode;
        std::memcpy(replacement.data() + 1, &offset, 4);
        std::memcpy(address, replacement.data(), site.length);
    } else {
        std::memcpy(address, site.original, site.length);
    }
    DWORD ignored;
    const bool restored = VirtualProtect(address, site.length, protection, &ignored) != FALSE;
    const bool flushed = FlushInstructionCache(GetCurrentProcess(), address, site.length) != FALSE;
    if (!restored || !flushed) {
        // Restore the bytes even when restoring the page flags failed.
        if (VirtualProtect(address, site.length, PAGE_EXECUTE_READWRITE, &ignored)) {
            std::memcpy(address, site.original, site.length);
            VirtualProtect(address, site.length, protection, &ignored);
            FlushInstructionCache(GetCurrentProcess(), address, site.length);
        }
        return false;
    }
    return true;
}

class ThreadPause {
public:
    ~ThreadPause() {
        for (auto it = threads_.rbegin(); it != threads_.rend(); ++it) {
            if (it->paused) ResumeThread(it->handle);
            CloseHandle(it->handle);
        }
    }

    bool pause(std::uintptr_t image) {
        const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return false;
        const DWORD own = GetCurrentThreadId();
        const DWORD pid = GetCurrentProcessId();
        THREADENTRY32 entry{sizeof entry};
        bool ok = Thread32First(snapshot, &entry) != FALSE;
        for (BOOL more = ok; more; more = Thread32Next(snapshot, &entry)) {
            if (entry.th32OwnerProcessID != pid || entry.th32ThreadID == own) continue;
            const HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT,
                                             FALSE, entry.th32ThreadID);
            if (!thread) {
                ok = false;
                break;
            }
            threads_.push_back({thread, false});
        }
        CloseHandle(snapshot);
        if (!ok) return false;
        for (auto& thread : threads_) {
            if (SuspendThread(thread.handle) == static_cast<DWORD>(-1)) return false;
            thread.paused = true;
            CONTEXT context{};
            context.ContextFlags = CONTEXT_CONTROL;
            if (!GetThreadContext(thread.handle, &context)) return false;
            for (const auto& site : sites) {
                const auto start = image + site.rva;
                if (context.Eip >= start && context.Eip < start + site.length) return false;
            }
        }
        return true;
    }

private:
    struct Thread {
        HANDLE handle;
        bool paused;
    };
    std::vector<Thread> threads_;
};

constexpr UINT_PTR ban_hint_timer=0x4B455042;
unsigned ban_hint_attempts=0;
void CALLBACK refresh_native_ban_hints(HWND owner,UINT,UINT_PTR timer,DWORD) {
    KillTimer(owner,timer);
    const auto result=kingdom::native_ui::apply_ban_hints(owner,reinterpret_cast<std::uintptr_t>(image_base));
    if(result.status==kingdom::native_ui::BanHintStatus::unavailable && ++ban_hint_attempts<40) {
        SetTimer(owner,ban_hint_timer,50,refresh_native_ban_hints);return;
    }
    char message[100];std::snprintf(message,sizeof message,"native ban help: status=%u updated=%u",
        static_cast<unsigned>(result.status),result.changed);log(message);
}
BOOL CALLBACK find_native_owner(HWND window,LPARAM value) {
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);
    if(pid==GetCurrentProcessId() && !GetWindow(window,GW_OWNER)) {
        *reinterpret_cast<HWND*>(value)=window;return FALSE;
    }
    return TRUE;
}
void schedule_native_ban_hints() {
    HWND owner=nullptr;EnumWindows(find_native_owner,reinterpret_cast<LPARAM>(&owner));
    if(!owner || !SetTimer(owner,ban_hint_timer,50,refresh_native_ban_hints))log("native ban help: scheduling failed");
}

}

extern "C" DWORD WINAPI install_patch(void* argument) {
    const auto mode = reinterpret_cast<std::uintptr_t>(argument);
    if (mode < 1 || mode > 4) return 1;
    if (InterlockedCompareExchange(&initialized, 1, 0) != 0) return 2;
    auto* base = reinterpret_cast<std::byte*>(GetModuleHandleW(nullptr));
    if (!base) return 3;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew >= 4096) return 3;
    const auto* pe = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        pe->OptionalHeader.SizeOfImage < 0x8C8D60) return 3;
    std::array<const Site*, std::size(sites)> enabled{};
    std::size_t enabled_count = 0;
    for (const auto& site : sites) {
        if (site.modes && !(site.modes & (1u << mode))) continue;
        if (std::memcmp(base + site.rva, site.original, site.length) != 0) return 4;
        enabled[enabled_count++] = &site;
    }
    if (std::memcmp(base + 0x1F6F40, "\xC7\x45\xFC\x01\x00\x00\x00", 7) != 0 ||
        std::memcmp(base + 0x1FF7D4, "\x32\xC0\x8B\x4D\xF4", 5) != 0 ||
        std::memcmp(base + 0x1F6CD3, "\x8B\x7D\xE4\x8D\x4D\xD0", 6) != 0) return 4;

    kingdom::ui::load_language();
    wchar_t temp[MAX_PATH];
    const DWORD size = GetTempPathW(MAX_PATH, temp);
    if (!size || size >= MAX_PATH ||
        swprintf_s(log_path, L"%lsDominionKingdomEffects-%lu.log", temp, GetCurrentProcessId()) < 0) {
        return 5;
    }
    const auto image = reinterpret_cast<std::uintptr_t>(base);
    kingdom::effect_text_fix::initialize(image);
    image_base = base;
    native_parser = image + 0x0DDBB0;
    native_local_normalize = image + 0x0DBEF0;
    native_effect_requirements = image + 0x0DC2D5;
    native_effect_advance = image + 0x0DC2F6;
    native_clipboard = image + 0x22BEA0;
    native_merge = image + 0x1F6A90;
    native_serializer = image + 0x0E4CB0;
    native_played_serializer = image + 0x0DFB20;
    native_saved_serializer = image + 0x0E0930;
    native_setup_serializer = image + 0x0E0FF0;
    native_string_construct = image + 0x23D720;
    native_local_load = image + 0x224070;
    native_extras_serializer = image + 0x0E5370;
    native_online_filter = image + 0x0DBEF0;
    native_create_message = image + 0x28B720;
    native_game_desc = image + 0x0B48F0;
    native_game_state_init = image + 0x2A9170;
    native_active_setup = image + 0x0E49D0;
    native_string_assign = image + 0x23D850;
    native_save_profiles = image + 0x0D8AD0;
    search_cancel = image + 0x1F6F40;
    create_cancel = image + 0x1FF7D4;
    local_dispatch_continue = image + 0x0B24C9;
    local_dispatch_cancel = image + 0x0B24EE;
    merge_original_tail = image + 0x1F6C7A;
    merge_serialization = image + 0x1F6CD3;
    measure_only = mode == 1;
    automatic_enabled = mode == 3;
    local_safety_enabled = mode == 2 || mode == 3;

    DWORD install_error = 0;
    {
        ThreadPause paused;
        if (!paused.pause(image)) return 7;
        std::uintptr_t client = 0;
        std::memcpy(&client, base + 0x8C8D5C, sizeof client);
        std::uint32_t active_game = 0;
        if (!client) return 8;
        std::memcpy(&active_game, reinterpret_cast<void*>(client + 0x5068), sizeof active_game);
        if (active_game) return 8;
        std::size_t installed = 0;
        for (std::size_t i = 0; i < enabled_count; ++i) {
            if (!change_site(base, *enabled[i], true)) {
                bool rollback_ok = std::memcmp(base + enabled[i]->rva,
                                           enabled[i]->original, enabled[i]->length) == 0;
                while (installed) {
                    const auto& previous = *enabled[--installed];
                    rollback_ok = change_site(base, previous, false) && rollback_ok;
                }
                // Do not resume game threads with a partially installed patch.
                // Termination is restricted to this verified Dominion process.
                if (!rollback_ok) {
                    TerminateProcess(GetCurrentProcess(), ERROR_PROCESS_ABORTED);
                    __fastfail(FAST_FAIL_FATAL_APP_EXIT); // Fallback if process termination failed.
                }
                install_error = 6;
                break;
            }
            ++installed;
        }
    }
    if (install_error) return install_error;
    if(!measure_only) kingdom::debt_stamp_supported.store(true,std::memory_order_release);
    if(!measure_only) schedule_native_ban_hints();
    log(mode == 4 ? "build 3315: patch active, online diagnostics"
                  : mode == 3 ? "build 3315: patch active, automatic local doubling"
                  : measure_only ? "build 3315: measure mode" : "build 3315: patch active");
    if (!measure_only) log("online ban category limits removed, native shared capacity 700");
    return 0;
}

#pragma comment(linker, "/EXPORT:InstallPatch=_install_patch@4")
