// Search latency on an isolated desktop, without physical input or user preferences.
#define KINGDOM_UI_TEST
#define UNICODE
#define EFFECT_TABLE_PERF_COUNTERS
#include "effect_editor.cpp"
#include "card_picker.cpp"
#include <chrono>
#include <cstdio>
#include <fstream>

namespace {
std::ofstream report;
bool inspected=false;
bool failed=false;
UINT_PTR search_timer=0;
using Clock=std::chrono::steady_clock;
BOOL CALLBACK benchmark(HWND dialog,LPARAM) {
    if(!GetDlgItem(dialog,IDC_CATALOG))return TRUE;
    KillTimer(nullptr,search_timer);
    try {
        const auto list=GetDlgItem(dialog,IDC_CATALOG);
        // Visit every row once to verify that returning to a retained field
        // never allocates it again. Heights must already be complete.
        for(int row=0;row<89;++row)SendMessageW(list,LB_SETTOPINDEX,row,0);
        SendMessageW(list,WM_VSCROLL,SB_TOP,0);
        std::vector<double> samples;
        for(int round=0;round<3;++round) {
            const std::vector<std::wstring> queries{L"",L"t",L"tw",L"twi",L"twist",L"twist_",L"twist_b",L"twist_big",L"twist_big_ideas",
                L"twist_big",L"twist_",L"tw",L"",L"Ungetestet",L"untested",L"Zugbeginn",L"Spielaufbau",L"no-such-effect",L"",L"Große Ideen",L"Big Ideas",L""};
            for(const auto& query:queries) {
                const auto counters=kingdom::effect_table_detail::performance;
                const auto begin=Clock::now();
                SetDlgItemTextW(dialog,IDC_QUERY,query.c_str());
                UpdateWindow(list);
                const auto elapsed=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();
                if(round){
                    samples.push_back(elapsed);
                    const auto& after=kingdom::effect_table_detail::performance;
                    if(after.created!=counters.created || after.destroyed!=counters.destroyed || after.measurements!=counters.measurements || after.measurement_windows!=counters.measurement_windows)
                        throw std::runtime_error("Warm filtering creates windows or measures text again.");
                }
                report<<(kingdom::ui::english()?"en":"de")<<'\t'<<round<<"\tquery\t"<<elapsed<<'\n';
            }
            for(int category:{0,1,4,5,0}) {
                const auto counters=kingdom::effect_table_detail::performance;
                const auto begin=Clock::now();
                SendDlgItemMessageW(dialog,IDC_CATEGORY,CB_SETCURSEL,category,0);
                SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(IDC_CATEGORY,CBN_SELCHANGE),0);
                UpdateWindow(list);
                const auto elapsed=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();
                if(round){
                    samples.push_back(elapsed);
                    const auto& after=kingdom::effect_table_detail::performance;
                    if(after.created!=counters.created || after.destroyed!=counters.destroyed || after.measurements!=counters.measurements || after.measurement_windows!=counters.measurement_windows)
                        throw std::runtime_error("Warm filtering creates windows or measures text again.");
                }
                report<<(kingdom::ui::english()?"en":"de")<<'\t'<<round<<"\tcategory\t"<<elapsed<<'\n';
            }
        }
        std::sort(samples.begin(),samples.end());
        std::printf("%s: warmed updates=%zu median=%.2f ms p95=%.2f ms max=%.2f ms\n",
            kingdom::ui::english()?"en":"de",samples.size(),samples[samples.size()/2],samples[(samples.size()*95)/100],samples.back());
        std::fflush(stdout);
        if(samples.back()>100.0)throw std::runtime_error("Warm search exceeded 100 ms.");
        inspected=true;
    }catch(const std::exception& error){std::fprintf(stderr,"Search check: %s\n",error.what());failed=true;}
    PostMessageW(dialog,WM_COMMAND,IDCANCEL,0);return FALSE;
}
void CALLBACK poll(HWND,UINT,UINT_PTR,DWORD){EnumThreadWindows(GetCurrentThreadId(),benchmark,0);}
}
int wmain(int argc,wchar_t** argv) {
    kingdom::editor_card_access=[](std::string_view){return kingdom::CardAccess::allowed;};
    if(argc!=2)return 2;
    report.open(argv[1],std::ios::binary);if(!report)return 3;
    report<<"language\tround\taction\tmilliseconds\n";
    const auto desktop=CreateDesktopW((L"DominionSearch_"+std::to_wstring(GetCurrentProcessId())).c_str(),nullptr,nullptr,0,GENERIC_ALL,nullptr);
    if(!desktop || !SetThreadDesktop(desktop))return 4;
    wchar_t package[32768]{};
    if(GetEnvironmentVariableW(L"DOMINION_RESOURCE_TEST_PACKAGE",package,32768))kingdom::set_resource_package_for_test(package);
    kingdom::register_effect_table(GetModuleHandleW(nullptr));
    const std::function<bool(const kingdom::EffectCommands&)> apply=[](const auto&){return true;};
    for(const auto language:{kingdom::ui::Language::de,kingdom::ui::Language::en}) {
        kingdom::ui::selected=language;inspected=false;
        kingdom::Draft draft{{},false,apply,{},kingdom::EditorContext::local};
        search_timer=SetTimer(nullptr,0,100,poll);
        DialogBoxParamW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(IDD_EFFECT_EDITOR),nullptr,kingdom::dialog_proc,reinterpret_cast<LPARAM>(&draft));
        KillTimer(nullptr,search_timer);if(!inspected || failed)return 5;
    }
    return 0;
}
