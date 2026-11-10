#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "working_copy.hpp"
#include <tlhelp32.h>

#include <array>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

static_assert(sizeof(void*) == 4, "Build the launcher for x86");

namespace {

using namespace working_copy;

std::uintptr_t find_remote_module(DWORD pid, const wchar_t* name) {
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
    if (!snapshot.valid()) return 0;
    MODULEENTRY32W entry{sizeof entry};
    for (BOOL more = Module32FirstW(snapshot.value, &entry); more;
         more = Module32NextW(snapshot.value, &entry)) {
        if (_wcsicmp(name, entry.szModule) == 0) {
            return reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
        }
    }
    return 0;
}

std::uintptr_t remote_load_address(DWORD pid) {
    const auto address = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    HMODULE owner{};
    require(address && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                          reinterpret_cast<LPCWSTR>(address), &owner),
            "Cannot resolve LoadLibraryW.");
    wchar_t path[MAX_PATH];
    const DWORD size = GetModuleFileNameW(owner, path, MAX_PATH);
    require(size && size < MAX_PATH, "Cannot identify the Windows loader module.");
    const auto name = std::filesystem::path(path).filename();
    const auto remote = find_remote_module(pid, name.c_str());
    require(remote != 0, "Target does not have the expected Windows loader module.");
    return remote + reinterpret_cast<std::uintptr_t>(address)
           - reinterpret_cast<std::uintptr_t>(owner);
}

void wait_for_menu(HANDLE process, DWORD pid) {
    std::uintptr_t image = 0;
    const auto deadline = GetTickCount64() + 45000;
    std::puts("Waiting for Dominion's main menu...");
    std::fflush(stdout);
    do {
        require(WaitForSingleObject(process, 0) == WAIT_TIMEOUT,
                "The test copy exited before the patch could attach.");
        if (!image) image = find_remote_module(pid, L"Dominion.exe");
        std::uintptr_t client = 0;
        SIZE_T read = 0;
        if (image && ReadProcessMemory(process, reinterpret_cast<void*>(image + 0x8C8D5C),
                              &client, sizeof client, &read) && read == sizeof client && client) {
            DWORD active = 0;
            if (ReadProcessMemory(process, reinterpret_cast<void*>(client + 0x5068),
                                  &active, sizeof active, &read) && read == sizeof active && !active) {
                return;
            }
        }
        Sleep(200);
    } while (GetTickCount64() < deadline);
    throw std::runtime_error("Main menu not ready. Close any active game and restart the test copy.");
}

DWORD remote_call(HANDLE process, std::uintptr_t address, void* argument) {
    Handle thread(CreateRemoteThread(process, nullptr, 0,
                  reinterpret_cast<LPTHREAD_START_ROUTINE>(address), argument, 0, nullptr));
    require(thread.valid(), "Cannot start the initialization thread.");
    require(WaitForSingleObject(thread.value, INFINITE) == WAIT_OBJECT_0,
            "Initialization thread did not finish.");
    DWORD result = 0;
    require(GetExitCodeThread(thread.value, &result) != FALSE, "Cannot read initialization result.");
    return result;
}

void attach(HANDLE process, DWORD pid, DWORD mode, const std::filesystem::path& dll) {
    const auto probe = LoadLibraryExW(dll.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
    require(probe != nullptr, "Cannot inspect the x86 patch DLL.");
    const auto install = GetProcAddress(probe, "InstallPatch");
    const auto offset = reinterpret_cast<std::uintptr_t>(install) -
                        reinterpret_cast<std::uintptr_t>(probe);
    FreeLibrary(probe);
    require(install != nullptr, "InstallPatch export is missing.");
    const auto loader = remote_load_address(pid);
    const auto bytes = (dll.native().size() + 1) * sizeof(wchar_t);
    const auto target = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    require(target != nullptr, "Cannot allocate the initialization path.");
    DWORD remote_base = 0;
    try {
        SIZE_T written = 0;
        require(WriteProcessMemory(process, target, dll.c_str(), bytes, &written) && written == bytes,
                "Cannot copy the DLL path to the target.");
        remote_base = remote_call(process, loader, target);
    } catch (...) {
        VirtualFreeEx(process, target, 0, MEM_RELEASE);
        throw;
    }
    VirtualFreeEx(process, target, 0, MEM_RELEASE);
    require(remote_base != 0, "The game could not load the x86 patch DLL.");
    const auto result = remote_call(process, remote_base + offset,
                                    reinterpret_cast<void*>(static_cast<std::uintptr_t>(mode)));
    if (result) {
        std::fprintf(stderr, "InstallPatch returned %lu. Restart the test copy before retrying.\n", result);
        throw std::runtime_error("The patch did not initialize.");
    }
}

}

int wmain(int argc, wchar_t** argv) {
    const DWORD requested_mode = argc != 5 ? 0 : std::wcscmp(argv[2], L"--measure") == 0 ? 1
        : std::wcscmp(argv[2], L"--apply") == 0 ? 2
        : std::wcscmp(argv[2], L"--apply-auto") == 0 ? 3 : 0;
    const bool online = requested_mode && std::wcscmp(argv[1], L"--online") == 0;
    const bool local = requested_mode &&
        (std::wcscmp(argv[1], L"--local") == 0 || std::wcscmp(argv[1], L"--offline") == 0);
    if (!local && (!online || requested_mode != 2)) {
        std::fputs("Usage: attach.exe --local --measure|--apply|--apply-auto <PID> <test Dominion.exe>\n"
                   "       attach.exe --online --apply <PID> <test Dominion.exe>\n", stderr);
        return 2;
    }
    const DWORD mode = online ? 4 : requested_mode;
    try {
        wchar_t* end = nullptr;
        const auto pid = std::wcstoul(argv[3], &end, 10);
        require(pid != 0 && end && *end == L'\0', "Invalid PID.");
        const auto wanted = std::filesystem::canonical(argv[4]);
        require(_wcsicmp(wanted.filename().c_str(), L"Dominion.exe") == 0, "Expected Dominion.exe.");
        constexpr auto access = PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                                PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD | SYNCHRONIZE;
        Handle process(OpenProcess(access, FALSE, pid));
        require(process.valid(), "Cannot open the Dominion test process.");
        const auto actual = std::filesystem::canonical(process_path(process.value));
        require(_wcsicmp(actual.c_str(), wanted.c_str()) == 0, "PID does not belong to the specified copy.");
        Handle executable(CreateFileW(actual.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr));
        require(executable.valid(), "Cannot open the test executable.");
        Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",
                                    FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        require(original.valid(), "Cannot verify that the working copy is separate from Steam's original.");
        require(!same_file(identify(executable.value), identify(original.value)),
                "Use a separate working copy. The process is running the original Steam executable.");
        verify_executable(executable.value);

        wchar_t self[MAX_PATH];
        const DWORD size = GetModuleFileNameW(nullptr, self, MAX_PATH);
        require(size && size < MAX_PATH, "Cannot determine the launcher's directory.");
        const auto dll = std::filesystem::canonical(std::filesystem::path(self).parent_path() /
                                                    L"KingdomEffectsPatch.dll");
        wait_for_menu(process.value, pid);
        attach(process.value, pid, mode, dll);
        std::printf("Dominion 3315, PID %lu, %s. Log: %%TEMP%%\\DominionKingdomEffects-%lu.log\n",
                    pid, online ? "patch active, online diagnostics" : mode == 1 ? "measure mode"
                        : mode == 2 ? "patch active" : "patch active, automatic local doubling", pid);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
