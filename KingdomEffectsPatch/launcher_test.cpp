// Exercise initialization completion and timeout without launching Dominion.
#define wmain launcher_entry_for_test
#include "attach.cpp"
#undef wmain
#include <cassert>

DWORD WINAPI return_value(void*) { return 42; }
DWORD WINAPI wait_for_event(void* event) {
    return WaitForSingleObject(static_cast<HANDLE>(event), 5000);
}

int main() {
    assert(remote_call(GetCurrentProcess(), reinterpret_cast<std::uintptr_t>(&return_value), nullptr, 5000) == 42);
    Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    assert(event.valid());
    bool pending = false;
    try {
        remote_call(GetCurrentProcess(), reinterpret_cast<std::uintptr_t>(&wait_for_event), event.value, 20);
    } catch (const InitializationPending&) { pending = true; }
    assert(pending);
    // A timeout must leave the remote worker alive and its argument valid.
    assert(SetEvent(event.value));
    Sleep(100);
    bool invalid = false;
    try { remote_call(nullptr, reinterpret_cast<std::uintptr_t>(&return_value), nullptr, 20); }
    catch (const std::runtime_error&) { invalid = true; }
    assert(invalid);
    std::puts("Launcher completion, timeout and failure checks passed.");
}
