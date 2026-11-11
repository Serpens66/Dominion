// Developer-only DLL, loaded in an isolated game process by roundtrip_test.
#include "patch.cpp"
#include "card_picker.hpp"
#include <fstream>
#include <filesystem>

namespace fixture {
enum class TargetKind { none,card,pile,kingdom_pile,event,landmark,way,prophecy,project,pile_list,duration,heirloom_parent };
constexpr unsigned kind_mask(TargetKind kind) { return 1u << unsigned(kind); }
using kingdom::CardTarget;
struct EffectDefinition {
    const char* id; const wchar_t *title,*description,*timing,*status,*verification;
    unsigned timing_mask; TargetKind first,second;
};
#include "effect_catalog.generated.hpp"
const char* target(TargetKind kind, bool second=false) {
    switch(kind) {
        case TargetKind::none: return "";
        case TargetKind::event: return "travelling_fair";
        case TargetKind::landmark: return "arena";
        case TargetKind::way: return "way_of_the_ox";
        case TargetKind::prophecy: return "approaching_army";
        case TargetKind::project: return "pageant";
        case TargetKind::duration: return "amulet";
        case TargetKind::heirloom_parent: return "pixie";
        case TargetKind::pile_list: return "village,smithy";
        default: return second ? "smithy" : "village";
    }
}
}

DWORD run_roundtrip(void* argument) {
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    native_parser=base+0xDDBB0; native_setup_serializer=base+0xE0FF0;
    native_active_setup=base+0xE49D0;
    native_string_assign=base+0x23D850;
    HMODULE self{}; GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&run_roundtrip),&self);
    wchar_t path[32768]{}; GetModuleFileNameW(self,path,32768);
    const auto phase=reinterpret_cast<std::uintptr_t>(argument);
    if (phase==1) return install_patch(reinterpret_cast<void*>(2));
    std::ofstream report(std::filesystem::path(path).parent_path()/L"effect-roundtrip.tsv",std::ios::binary|(phase?std::ios::app:std::ios::trunc));
    report << std::unitbuf;
    if (!phase) report << "phase\teffect\tcase\tparsed\tserialized\treparsed\tresult\n";
    using Parse=void (__fastcall*)(const char*,kingdom::Setup*);
    const auto parse=reinterpret_cast<Parse>(native_parser);
    unsigned failures=0;
    {
        for (const auto& definition:fixture::effect_catalog) {
            std::string command=definition.id;
            if (definition.first!=fixture::TargetKind::none) {
                command+="("; command+=fixture::target(definition.first);
                if (definition.second!=fixture::TargetKind::none) { command+=","; command+=fixture::target(definition.second,true); }
                command+=")";
            }
            for (int sequence=0;sequence<2;++sequence) {
                kingdom::EffectCommands wanted{command};
                if (sequence) wanted={"start_double_pile(market)",command,"twist_big_ideas"};
                kingdom::Setup first{},second{};
                const auto input=std::string("-k cellar,market,merchant,militia,mine,moat,remodel,smithy,village,workshop ")+kingdom::effect_command_text(wanted);
                parse(input.c_str(),&first);
                const NativeSetupText serialized(first,23);
                kingdom::EffectCommands emitted;
                const bool readable=kingdom::read_effect_commands(serialized.view(),emitted);
                const auto emitted_count=emitted.size();
                const auto text=std::string(serialized.view());
                std::array<std::byte,0x38> configuration{};
                *reinterpret_cast<DWORD*>(configuration.data()+0x28)=23;
                *reinterpret_cast<const char**>(configuration.data()+0x30)=text.c_str();
                const auto configured=resolve_config(configuration.data());
                auto* destination=&second; const auto* source=text.c_str();
                const auto decode=base+0xDEEB0;
                __asm {
                    push destination
                    mov edx,23
                    mov ecx,source
                    call decode
                    add esp,4
                }
                const bool ok=readable && kingdom::same_effect_commands(wanted,emitted) &&
                    kingdom::read_effects(first.data())==kingdom::read_effects(second.data()) &&
                    kingdom::read_effects(first.data())==kingdom::read_effects(configured.data()) &&
                    parsed_effects_match(configured,wanted);
                report << (phase ? "patched" : "vanilla") << '\t' << command << '\t' << (sequence?"sandwich":"single") << '\t'
                    << kingdom::count(kingdom::read_effects(first.data())) << '\t' << emitted_count << '\t'
                    << kingdom::count(kingdom::read_effects(second.data())) << '\t' << (ok?"PASS":"FAIL") << '\n';
                if (!ok) ++failures;
            }
        }
    }
    kingdom::Setup invalid{};
    parse("-x twist_big_ideas,twist_garbage_heap",&invalid);
    auto reversed=kingdom::read_effects(invalid.data()); std::swap(reversed[0],reversed[1]);
    kingdom::write_effects(invalid.data(),reversed);
    std::size_t mismatch=99;
    if(parsed_effects_match(invalid,{"twist_big_ideas","twist_garbage_heap"},&mismatch) || mismatch!=0) ++failures;
    parse("-x start_double_pile(village)",&invalid);
    if(parsed_effects_match(invalid,{"start_double_pile(market)"},&mismatch)) ++failures;
    parse("-x twist_big_ideas,twist_not_a_real_effect,twist_beakers",&invalid);
    if(parsed_effects_match(invalid,{"twist_big_ideas","twist_not_a_real_effect","twist_beakers"},&mismatch) || mismatch!=1) ++failures;
    return failures;
}
namespace {
HANDLE completed=nullptr;
void* probe_argument=nullptr;
DWORD probe_result=0;
void CALLBACK on_probe(HWND window,UINT,UINT_PTR timer,DWORD) {
    KillTimer(window,timer);
    probe_result=run_roundtrip(probe_argument);
    SetEvent(completed);
}
BOOL CALLBACK find_window(HWND window,LPARAM parameter) {
    DWORD pid=0; GetWindowThreadProcessId(window,&pid);
    if(pid==GetCurrentProcessId() && !GetWindow(window,GW_OWNER)) { *reinterpret_cast<HWND*>(parameter)=window;return FALSE; }
    return TRUE;
}
}
extern "C" DWORD WINAPI roundtrip_probe(void* argument) {
    if (reinterpret_cast<std::uintptr_t>(argument)==1) return run_roundtrip(argument);
    HWND window=nullptr; EnumWindows(find_window,reinterpret_cast<LPARAM>(&window));
    if(!window) return 201;
    completed=CreateEventW(nullptr,TRUE,FALSE,nullptr); if(!completed) return 202;
    probe_argument=argument;
    if(!SetTimer(window,0x7119,20,on_probe)) {CloseHandle(completed);return 203;}
    const auto waited=WaitForSingleObject(completed,25000);
    if(waited!=WAIT_OBJECT_0) return 204; // Child is discarded; callback may still own the event.
    CloseHandle(completed);completed=nullptr;return probe_result;
}
#pragma comment(linker,"/EXPORT:RoundtripProbe=_roundtrip_probe@4")
