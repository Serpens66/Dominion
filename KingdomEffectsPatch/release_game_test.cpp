#define wmain attach_cli
#include "attach.cpp"
#undef wmain

int wmain(int argc, wchar_t** argv) {
    try {
        require(argc == 5, "Usage: release_game_test.exe <copy Dominion.exe> <package bin> <original Dominion.exe> <0..3>");
        const std::filesystem::path path = std::filesystem::canonical(argv[1]);
        const auto package = std::filesystem::canonical(argv[2]);
        Handle original(CreateFileW(argv[3], FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
        require(original.valid(), "Original executable missing.");
        Handle executable(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
        require(executable.valid(), "Working copy missing.");
        verify_executable(executable.value);
        require(!same_file(identify(executable.value), identify(original.value)), "Refusing the original executable.");
        const auto name = L"DominionBatch_" + std::to_wstring(GetCurrentProcessId());
        const auto scratch = std::filesystem::temp_directory_path() / name;
        require(std::filesystem::create_directory(scratch), "Scratch already exists.");
        auto profile = scratch / L"profiles.xml";
        Handle profile_file(CreateFileW(profile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, 0, nullptr));
        const int multiplicity = static_cast<int>(std::wcstol(argv[4], nullptr, 10));
        require(multiplicity >= 0 && multiplicity <= 3, "Expected multiplicity 0 to 3.");
        std::string kingdom = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop";
        for (int i = 0; i < multiplicity; ++i) {
            kingdom += i == 0 ? " -x " : ", ";
            kingdom += "twist_big_ideas";
        }
        const auto xml = "<ProfileSettings_Save><profiles><playerName>Effekttest</playerName>"
            "<forceOffline>1</forceOffline><profileId>1</profileId><gameSetup>"
            "<players><playerType>Local</playerType><playerName>Effekttest</playerName></players>"
            "<players><playerType>Local</playerType><playerName>Vergleich</playerName></players>"
            "<config><version>23</version><kingdomData>" + kingdom + "</kingdomData></config></gameSetup>"
            "</profiles><lastProfileID>1</lastProfileID></ProfileSettings_Save>";
        DWORD size;
        require(profile_file.valid() && WriteFile(profile_file.value, xml.data(), static_cast<DWORD>(xml.size()), &size, nullptr) && size == xml.size(), "Could not create scratch profile.");
        FlushFileBuffers(profile_file.value);
        CloseHandle(profile_file.value);
        profile_file.value = nullptr;
        const auto target = scratch.string() + "\\";
        require(target.size() < MAX_PATH, "Scratch path exceeds game's path buffer.");

        HDESK desktop = CreateDesktopW(name.c_str(), nullptr, nullptr, 0, DESKTOP_CREATEWINDOW | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS, nullptr);
        if (!desktop) {
            USEROBJECTFLAGS flags{};
            DWORD needed = 0;
            require(GetUserObjectInformationW(GetProcessWindowStation(), UOI_FLAGS, &flags, sizeof flags, &needed),
                    "Cannot inspect current window station.");
            require(!(flags.dwFlags & WSF_VISIBLE), "Cannot create a separate desktop from the visible window station.");
            std::puts("Using the sandbox's non-visible window station.");
        }
        STARTUPINFOW startup{};
        startup.cb = sizeof startup;
        if (desktop) startup.lpDesktop = const_cast<wchar_t*>(name.c_str());
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION child{};
        if (!CreateProcessW(path.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr,
                            path.parent_path().c_str(), &startup, &child)) {
            if (desktop) CloseDesktop(desktop);
            throw std::runtime_error("Cannot launch the working copy on a separate desktop.");
        }
        Handle process(child.hProcess);
        Handle thread(child.hThread);
        struct Cleanup {
            HANDLE process;
            HDESK desktop;
            ~Cleanup() { TerminateProcess(process, 0); WaitForSingleObject(process, 5000); if (desktop) CloseDesktop(desktop); }
        } cleanup{process.value, desktop};

        using Query = LONG (NTAPI*)(HANDLE, ULONG, void*, ULONG, ULONG*);
        const auto query = reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
        std::array<std::uintptr_t, 6> pbi{};
        require(query && query(process.value, 0, pbi.data(), sizeof pbi, nullptr) == 0, "Could not read child PEB.");
        std::uintptr_t base = 0;
        SIZE_T amount = 0;
        require(ReadProcessMemory(process.value, reinterpret_cast<void*>(pbi[1] + 8), &base, 4, &amount) && amount == 4 && base,
                "Could not read image base.");
        constexpr std::array<unsigned char, 9> expected{0x55,0x8b,0xec,0x81,0xec,0x08,0x01,0x00,0x00};
        std::array<unsigned char, 9> actual{};
        require(ReadProcessMemory(process.value, reinterpret_cast<void*>(base + 0x23C090), actual.data(), actual.size(), &amount) &&
                amount == actual.size() && actual == expected, "Save folder routine differs.");
        const auto cache = reinterpret_cast<void*>(base + 0x15E3A98);
        char empty = 'x';
        require(ReadProcessMemory(process.value, cache, &empty, 1, &amount) && amount == 1 && !empty, "Save folder cache is already initialized.");
        require(WriteProcessMemory(process.value, cache, target.c_str(), target.size() + 1, &amount) && amount == target.size() + 1,
                "Cannot set disposable save folder.");
        std::vector<char> checked(target.size() + 1);
        require(ReadProcessMemory(process.value, cache, checked.data(), checked.size(), &amount) && amount == checked.size() &&
                !std::memcmp(checked.data(), target.c_str(), checked.size()), "Could not verify disposable save folder.");
        std::puts("Background desktop and disposable save folder ready.");
        std::fflush(stdout);
        require(ResumeThread(thread.value) != static_cast<DWORD>(-1), "Could not start child thread.");
        wait_for_menu(process.value, child.dwProcessId);
        require(std::filesystem::exists(scratch / L"log.txt"), "Game did not write its log to disposable folder.");
        std::puts("The actual Dominion engine reached its main menu without using the foreground desktop.");
        const auto invoke_launcher = [&](const std::filesystem::path& source) {
            const auto exe = package / L"attach.exe";
            std::wstring command = L"\"" + exe.wstring() + L"\" --local --apply " +
                std::to_wstring(child.dwProcessId) + L" \"" + path.wstring() + L"\" \"" + source.wstring() + L"\"";
            STARTUPINFOW info{sizeof info};
            PROCESS_INFORMATION attached{};
            require(CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, package.c_str(), &info, &attached), "Cannot launch the packaged injector.");
            Handle launcher(attached.hProcess), launch_thread(attached.hThread);
            if (WaitForSingleObject(launcher.value, 75000) != WAIT_OBJECT_0) {
                TerminateProcess(launcher.value, 1);
                throw std::runtime_error("Packaged injector did not terminate.");
            }
            DWORD result = 0;
            require(GetExitCodeProcess(launcher.value, &result), "Cannot read injector result.");
            return result;
        };
        require(invoke_launcher(path) != 0, "Injector did not reject original/copy identity collision.");
        require(invoke_launcher(std::filesystem::canonical(argv[3])) == 0, "Packaged injector failed.");
        unsigned char limit = 0xFF;
        require(ReadProcessMemory(process.value, reinterpret_cast<void*>(base + 0x208EA4), &limit, 1, &amount) &&
                amount == 1 && limit == 0, "Online ban patch is not installed.");
        std::wprintf(L"Scratch: %ls\n", scratch.c_str());
        constexpr std::array<unsigned char, 5> native_start{0xC7,0x87,0x44,0x50,0x00};
        std::array<unsigned char, 5> start_check{};
        require(ReadProcessMemory(process.value, reinterpret_cast<void*>(base + 0x0B24BF),
                                  start_check.data(), start_check.size(), &amount) && amount == start_check.size() &&
                start_check != native_start && start_check[0] == 0xE9, "Patched local start dispatcher missing.");
        const auto read = [&](std::uintptr_t address) {
            DWORD result;
            require(ReadProcessMemory(process.value, reinterpret_cast<void*>(address), &result, 4, &amount) && amount == 4,
                    "Could not read native game state.");
            return result;
        };
        const auto client = read(base + 0x8C8D5C);
        require(client && read(client + 0x5068) == 0 && read(client + 0x5044) == 0,
                "The disposable client is not idle.");
        const DWORD request = 1;
        require(WriteProcessMemory(process.value, reinterpret_cast<void*>(client + 0x5044), &request, 4, &amount) && amount == 4,
                "Could not queue the native local start.");
        const auto state = base + 0x780AD8;
        const auto deadline = GetTickCount64() + 10000;
        while (GetTickCount64() < deadline) {
            require(WaitForSingleObject(process.value, 0) == WAIT_TIMEOUT, "Game exited before first turn.");
            if (read(client + 0x5068) == 1 && read(state + 0x19E0) > 0) break;
            Sleep(50);
        }
        Sleep(600);
        const DWORD turn = read(state + 0x19E0);
        const DWORD players = read(state + 0xD38);
        const DWORD cards = read(state + 0x1520);
        std::printf("State: turn=%lu, players=%lu, cards=%lu.\n", turn, players, cards);
        require(turn == 1 && players == 2 && cards > 2 && cards <= 800, "The fixture did not reach a two-player first turn.");
        const DWORD who = read(state + 0x19CC);
        require(who < players, "No active first player.");
        unsigned hand = 0;
        for (DWORD i = 0; i < cards; ++i) {
            const auto record = state + 0x1A48 + i * 0x64;
            if (read(record + 8) == 0x3EA && read(record + 0x28) == who) ++hand;
        }
        unsigned installed = 0;
        for (DWORD i = 0; i < 32 && read(state + 0x1280 + i * 0x14) == 0x1774; ++i) ++installed;
        std::printf("Big Ideas x%d: first turn, hand=%u, installed=%u.\n", multiplicity, hand, installed);
        require(installed == static_cast<unsigned>(multiplicity) && hand == 5u + installed,
                "Unexpected effect count or first-turn hand size.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s (Win32 %lu)\n", error.what(), GetLastError());
        return 1;
    }
}
