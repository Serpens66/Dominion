// Clipboard API boundary test. GlobalAlloc/Lock are real; the user's clipboard
// is NEVER opened. Tests payload, ownership transfer, and busy/failing APIs.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <string>
namespace {
HGLOBAL stored=nullptr;
bool busy=false, fail_set=false;
int emptied=0, closed=0;
BOOL WINAPI test_open(HWND) { return !busy; }
BOOL WINAPI test_empty() { ++emptied; if (stored) GlobalFree(stored); stored=nullptr; return TRUE; }
HANDLE WINAPI test_set(UINT format,HANDLE memory) {
    if (fail_set || format!=CF_UNICODETEXT) return nullptr;
    stored=static_cast<HGLOBAL>(memory); return memory;
}
BOOL WINAPI test_close() { ++closed; return TRUE; }
}
#define OpenClipboard test_open
#define EmptyClipboard test_empty
#define SetClipboardData test_set
#define CloseClipboard test_close
#include "effect_table.hpp"
#undef OpenClipboard
#undef EmptyClipboard
#undef SetClipboardData
#undef CloseClipboard

int main() {
    using kingdom::effect_table_detail::copy_text;
    using kingdom::effect_table_detail::plain_copy;
    const std::wstring text=L"Müllhaufen (twist_\u200Bgarbage_\u200Bheap)\tUngetestet\r\nZugbeginn";
    if (!copy_text(nullptr,text) || !stored || emptied!=1 || closed!=1) return 1;
    const auto* value=static_cast<const wchar_t*>(GlobalLock(stored));
    const bool unicode=value && std::wstring(value)==plain_copy(text);
    if (value) GlobalUnlock(stored);
    if (!unicode) return 2;
    const auto original=stored;
    busy=true;
    if (copy_text(nullptr,L"busy") || stored!=original || emptied!=1 || closed!=1) return 3;
    busy=false; fail_set=true;
    if (copy_text(nullptr,L"failed") || stored || emptied!=2 || closed!=2) return 4;
    std::puts("PASS: Unicode payload, ownership transfer, busy clipboard and failed publication (isolated API boundary).");
    return 0;
}
