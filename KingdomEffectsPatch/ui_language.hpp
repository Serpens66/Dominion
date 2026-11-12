#pragma once
#include <windows.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <atomic>
namespace kingdom::ui {
enum class Language { de, en };
inline std::atomic<Language> selected{Language::de};
inline Language language() { return selected.load(); }
inline bool english() { return language()==Language::en; }
inline const wchar_t* pick(const wchar_t* de,const wchar_t* en) { return english()?en:de; }
#ifdef KINGDOM_UI_TEST
inline std::filesystem::path test_preferences;
#endif
inline std::filesystem::path preferences_path() {
#ifdef KINGDOM_UI_TEST
return test_preferences;
#else
wchar_t path[32768]{};if(!GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768)) return {};
return std::filesystem::path(path)/L"DominionKingdomEffectsPatch"/L"editor.ini";
#endif
}
inline void load_language() {const auto p=preferences_path();if(p.empty())return;wchar_t value[8]{};GetPrivateProfileStringW(L"editor",L"language",L"de",value,8,p.c_str());selected=std::wstring_view(value)==L"en"?Language::en:Language::de;}
inline bool set_language(Language value) {selected=value;const auto p=preferences_path();if(p.empty())return true;std::error_code error;std::filesystem::create_directories(p.parent_path(),error);return !error && WritePrivateProfileStringW(L"editor",L"language",value==Language::en?L"en":L"de",p.c_str());}
inline const wchar_t* tr(const wchar_t* text) { if(!english()) return text;
struct Pair {std::wstring_view de; const wchar_t* en;};

#include "ui_translations.generated.hpp"
for(const auto& pair:pairs) if(pair.de==text) return pair.en;return text;}
}
