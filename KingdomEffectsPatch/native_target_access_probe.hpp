#pragma once
// Developer-only read checks, executed on the disposable client's UI thread.
#include "target_access_runtime.hpp"
#include "native_card_access.hpp"
#include <fstream>
#include <filesystem>

DWORD run_native_target_access_checks() {
    const auto* base = reinterpret_cast<const std::byte*>(GetModuleHandleW(nullptr));
    image_base = base;
    const auto* profile = active_profile();
    if (!profile) return 701;
    std::array<std::byte,0x7868> original{};
    std::memcpy(original.data(),profile,original.size());
    HMODULE self{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&run_native_target_access_checks),&self);
    wchar_t module_path[32768]{};
    GetModuleFileNameW(self,module_path,32768);
    std::ofstream report(std::filesystem::path(module_path).parent_path()/L"target-access-native.tsv",std::ios::binary);
    if (!report) return 702;
    report << "card\tnative_id\taccess\tdirect_native\tresult\n" << std::unitbuf;
    const auto initial = kingdom::NativeCardAccess::capture(base);
    const auto cache_count=*reinterpret_cast<const DWORD*>(profile+0x7450);
    if ((!cache_count && initial.known()) || std::memcmp(original.data(),profile,original.size())!=0) return 707;
    report<<"cold_snapshot\t0\t"<<(initial.known()?"warm":"unknown")<<"\tread_only\tPASS\n";
    // Explicit native fixture preparation may initialize Dominion's own cache.
    // This happens before the read-only baseline, not inside our adapter.
    using GetGrants = int(__fastcall*)(const DWORD**,const DWORD**);
    const DWORD *ids=nullptr,*sources=nullptr;
    const auto count=reinterpret_cast<GetGrants>(const_cast<std::byte*>(base)+0xDAF40)(&ids,&sources);
    std::memcpy(original.data(),profile,original.size());
    const auto snapshot = kingdom::NativeCardAccess::capture(base);
    if (!snapshot.known() || snapshot.profile()!=profile || !snapshot.grant_count()) return 703;
    if (count<1 || count>128 || !ids || !sources || static_cast<std::size_t>(count)!=snapshot.grant_count()) return 704;
    using Matches = bool(__fastcall*)(DWORD,DWORD);
    const auto matches=reinterpret_cast<Matches>(const_cast<std::byte*>(base)+0xDB7A0);
    for (int i=0;i<count;++i) if(ids[i]!=snapshot.grants()[static_cast<std::size_t>(i)]) return 705;
    unsigned failures=0,allowed=0,denied=0;
    bool base_allowed=false;
    const auto repeated=kingdom::NativeCardAccess::capture(base);
    if (!repeated.known() || repeated.grant_count()!=snapshot.grant_count()) return 706;
    for (const auto& card:kingdom::access_catalog::cards) {
        bool direct=false;
        for (int i=0;i<count;++i) if(matches(ids[i],card.native_id)){direct=true;break;}
        const auto result=snapshot.query_native(card.native_id);
        const auto expected=direct?kingdom::CardAccess::allowed:kingdom::CardAccess::denied;
        const bool ok=result==expected && repeated.query_native(card.native_id)==result;
        report<<card.id<<'\t'<<card.native_id<<'\t'<<(result==kingdom::CardAccess::allowed?"allowed":result==kingdom::CardAccess::denied?"denied":"unknown")
              <<'\t'<<(direct?"allowed":"denied")<<'\t'<<(ok?"PASS":"FAIL")<<'\n';
        if(!ok)++failures;
        if(result==kingdom::CardAccess::allowed)++allowed;
        if(result==kingdom::CardAccess::denied)++denied;
        if(std::string_view(card.id)=="village")base_allowed=result==kingdom::CardAccess::allowed;
    }
    if(!base_allowed || !allowed)++failures;
    const auto missing=kingdom::NativeCardAccess::capture(nullptr);
    if(missing.known() || missing.query_native(0)!=kingdom::CardAccess::unknown || snapshot.query_native(0)!=kingdom::CardAccess::unknown)++failures;
    // A warm adapter must leave the entire profile unchanged, including caches.
    for(std::size_t i=0;i<original.size();++i) {
        if(original[i]!=profile[i]){report<<"profile_changed\t"<<i<<"\tFAIL\n";++failures;break;}
    }
    // Exercise the actual production F8 apply boundary without opening any UI.
    // All mutation from here on belongs exclusively to this disposable fixture.
    const kingdom::CardTarget* unavailable=nullptr;
    for(const auto& card:fixture::target_catalog) {
        if(!(card.kinds & fixture::kind_mask(fixture::TargetKind::kingdom_pile)))continue;
        const auto* native=kingdom::access_native_card(card.id);
        if(native && snapshot.query_native(native->native_id)==kingdom::CardAccess::denied){unavailable=&card;break;}
    }
    if(!unavailable){report<<"denied_fixture\tmissing\tFAIL\n";return failures+1;}
    const std::string denied_id=unavailable->id;
    const auto denied_wide=std::wstring(denied_id.begin(),denied_id.end());
    const kingdom::EffectCommands original_effects{"twist_big_ideas"};
    using Parse=void(__fastcall*)(const char*,kingdom::Setup*);
    const auto parse=reinterpret_cast<Parse>(native_parser);
    const std::size_t offsets[]{0x10,0x11B8,0x2360};
    struct RestoreAccess {
        kingdom::CardAccessQuery native=kingdom::editor_card_access,test=test_card_access;
        kingdom::ui::Language language=kingdom::ui::language();
        ~RestoreAccess(){kingdom::editor_card_access=std::move(native);test_card_access=std::move(test);kingdom::ui::selected=language;}
    } restore_access;
    for(const auto language:{kingdom::ui::Language::de,kingdom::ui::Language::en}) {
        kingdom::ui::selected=language;
        const auto denied_title=kingdom::card_title(*unavailable);
        for(std::size_t context=0;context<3;++context) {
            auto* config=const_cast<std::byte*>(profile)+offsets[context];
            kingdom::Setup current{};
            parse("-k village,market -x twist_big_ideas",&current);
            const kingdom::EffectCommands requests[]{
                {"start_add_card("+denied_id+")","twist_big_ideas"},
                {"twist_typhoon("+denied_id+",village,market)","twist_big_ideas"},
                {"twist_typhoon(village,"+denied_id+",market)","twist_big_ideas"},
                {"twist_typhoon(village,market,"+denied_id+")","twist_big_ideas"},
                {"start_make_split_pile("+denied_id+",village)","twist_big_ideas"},
                {"start_make_split_pile(village,"+denied_id+")","twist_big_ideas"},
                {"start_double_pile("+denied_id+")","twist_big_ideas"}
            };
            for(std::size_t n=0;n<std::size(requests);++n) {
                std::array<std::byte,0x11A8> before{};std::memcpy(before.data(),config,before.size());
                const auto setup_before=current;
                std::wstring error;
                const bool accepted=apply_editor_effects(&current,config,original_effects,requests[n],error);
                const auto opening=requests[n].front().find('(');
                const auto effect=requests[n].front().substr(0,opening);
                const bool ok=!accepted && !error.empty() && error.find(std::wstring(effect.begin(),effect.end()))!=error.npos &&
                    (error.find(denied_title)!=error.npos || error.find(denied_wide)!=error.npos) &&
                    current==setup_before && std::memcmp(before.data(),config,before.size())==0;
                report<<"apply_reject_"<<context<<'_'<<n<<(language==kingdom::ui::Language::en?"_en":"_de")<<"\t0\tdenied\tpreserved\t"<<(ok?"PASS":"FAIL")<<'\n';
                if(!ok)++failures;
            }
            // The equal-draft fast path must not bypass a fresh target check.
            const kingdom::EffectCommands invalid_old{"start_add_card("+denied_id+")"};
            const auto setup_before=current;
            std::array<std::byte,0x11A8> before{};std::memcpy(before.data(),config,before.size());
            std::wstring error;
            bool ok=!apply_editor_effects(&current,config,invalid_old,invalid_old,error) && !error.empty() &&
                current==setup_before && std::memcmp(before.data(),config,before.size())==0;
            report<<"apply_equal_reject_"<<context<<(language==kingdom::ui::Language::en?"_en":"_de")<<"\t0\tdenied\tpreserved\t"<<(ok?"PASS":"FAIL")<<'\n';
            if(!ok)++failures;
            test_card_access=[](std::string_view){return kingdom::CardAccess::unknown;};
            error.clear();
            ok=!apply_editor_effects(&current,config,original_effects,{"start_add_card(village)"},error) && !error.empty() &&
                current==setup_before && std::memcmp(before.data(),config,before.size())==0;
            // Unavailable authorization must not turn into a blanket effect ban.
            std::wstring no_target_error;
            ok&=apply_editor_effects(&current,config,{"twist_garbage_heap"},{"twist_garbage_heap"},no_target_error) && no_target_error.empty();
            report<<"apply_unknown_"<<context<<(language==kingdom::ui::Language::en?"_en":"_de")<<"\t0\tunknown\tpreserved\t"<<(ok?"PASS":"FAIL")<<'\n';
            if(!ok)++failures;
            test_card_access={};
        }
    }
    // Exercise search/clipboard import validation and the same preflight used
    // by local and online creation. Messages are suppressed only in this test.
    {
        const auto old_suppress=test_suppress_access_messages;
        test_suppress_access_messages=true;
        std::array<std::byte,0x7868> profile_before{};
        std::memcpy(profile_before.data(),profile,profile_before.size());
        const std::string bodies[]{"start_add_card("+denied_id+")",
            "twist_typhoon("+denied_id+",village,market)",
            "twist_typhoon(village,"+denied_id+",market)",
            "twist_typhoon(village,market,"+denied_id+")"};
        for (const auto& body:bodies) {
            const auto text=std::string("-k village,market -x ")+body+",twist_big_ideas";
            std::array<std::byte,0x38> config{};
            *reinterpret_cast<DWORD*>(config.data()+0x28)=23;
            *reinterpret_cast<const char**>(config.data()+0x30)=text.c_str();
            const auto before=config;
            const bool ok=!validate_native_import(text.c_str()) && !validate_config_targets(config.data()) && config==before &&
                std::memcmp(profile_before.data(),profile,profile_before.size())==0;
            report<<"import_and_start_"<<body<<"\t0\tdenied\tpreserved\t"<<(ok?"PASS":"FAIL")<<'\n';
            if(!ok)++failures;
        }
        test_suppress_access_messages=old_suppress;
    }
    // A Typhoon targeting only genuine Base grants remains usable even though
    // its native effect metadata references a different expansion.
    native_save_profiles=reinterpret_cast<std::uintptr_t>(base)+0xD8AD0;
    auto* local_config=const_cast<std::byte*>(profile)+0x10;
    *reinterpret_cast<DWORD*>(local_config+0x28)=23;
    kingdom::Setup current{};parse("-k village,market -x twist_big_ideas",&current);
    const kingdom::EffectCommands legal_typhoon{"twist_typhoon(village,market)","twist_big_ideas"};
    std::wstring legal_error;
    const bool legal=apply_editor_effects(&current,local_config,original_effects,legal_typhoon,legal_error);
    const bool legal_ok=legal && legal_error.empty() && parsed_effects_match(current,legal_typhoon);
    report<<"apply_legal_typhoon\t0\tallowed\tcommitted\t"<<(legal_ok?"PASS":"FAIL")<<'\n';
    if(!legal_ok)++failures;
    report<<"summary\tgrants="<<count<<"\tallowed="<<allowed<<"\tdenied="<<denied<<"\t"<<(failures?"FAIL":"PASS")<<'\n';
    return failures;
}
