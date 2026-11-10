#define WIN32_LEAN_AND_MEAN
#include "working_copy.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

static_assert(sizeof(void*) == 4, "Build the background runner for x86");

namespace {

using namespace working_copy;
namespace fs = std::filesystem;

constexpr std::uintptr_t client_pointer = 0x8C8D5C;
constexpr std::uintptr_t game_state = 0x780AD8;
constexpr std::uintptr_t save_folder_cache = 0x15E3A98;
constexpr std::size_t card_capacity = 800;
constexpr DWORD copper = 0x104;
constexpr DWORD silver = 0x105;

enum class Metric { hand, supply, owned };

struct Probe {
    const char* command;
    DWORD id;
    std::array<unsigned, 4> expected;
    Metric metric = Metric::hand;
    DWORD card_type = 0;
};

constexpr Probe probes[] = {
    {"twist_big_ideas", 0x1774, {5, 6, 7, 8}},
    {"twist_cramped", 0x17DF, {5, 4, 3, 2}},
    {"twist_penny_party", 0x1775, {7, 8, 9, 10}, Metric::owned, copper},
    {"start_double_pile(village)", 0xDB8, {10, 20, 40, 80}, Metric::supply, 0x206},
    {"start_triple_pile(village)", 0xDBA, {10, 30, 90, 270}, Metric::supply, 0x206},
    {"start_add_card(silver)", 0xDB5, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_add_each_pile", 0xDB6, {10, 20, 30, 40}, Metric::owned},
    {"start_double_deck", 0xDB7, {10, 20, 40, 80}, Metric::owned},
    {"start_replace_copper(silver)", 0xDB1, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_replace_estate(silver)", 0xDB2, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_replace_copper_from_pile(silver)", 0xDB3, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_replace_estate_from_pile(silver)", 0xDB4, {0, 1, 2, 3}, Metric::owned, silver},
};

class HiddenDesktop {
public:
    explicit HiddenDesktop(const std::wstring& name)
        : handle_(CreateDesktopW(name.c_str(), nullptr, nullptr, 0,
                                  DESKTOP_CREATEWINDOW | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS, nullptr)) {
        require(handle_ != nullptr, "Cannot create a separate Windows desktop.");
    }
    ~HiddenDesktop() { CloseDesktop(handle_); }

private:
    HDESK handle_;
};

class Child {
public:
    Child(const fs::path& exe, std::wstring& desktop) {
        STARTUPINFOW startup{};
        startup.cb = sizeof startup;
        startup.lpDesktop = desktop.data();
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_HIDE;
        require(CreateProcessW(exe.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr,
                               exe.parent_path().c_str(), &startup, &info_), "Cannot start the test copy.");
    }

    ~Child() {
        TerminateProcess(info_.hProcess, 0);
        WaitForSingleObject(info_.hProcess, 5000);
        CloseHandle(info_.hThread);
        CloseHandle(info_.hProcess);
    }

    bool alive() const { return WaitForSingleObject(info_.hProcess, 0) == WAIT_TIMEOUT; }

    void resume() const {
        require(ResumeThread(info_.hThread) != static_cast<DWORD>(-1), "Cannot resume the test copy.");
    }

    void read(std::uintptr_t address, void* output, std::size_t size) const {
        SIZE_T amount = 0;
        require(ReadProcessMemory(info_.hProcess, reinterpret_cast<const void*>(address), output, size, &amount) &&
                amount == size, "Cannot read the test process.");
    }

    DWORD read(std::uintptr_t address) const {
        DWORD value;
        read(address, &value, sizeof value);
        return value;
    }

    void write(std::uintptr_t address, const void* data, std::size_t size) const {
        SIZE_T amount = 0;
        require(WriteProcessMemory(info_.hProcess, reinterpret_cast<void*>(address), data, size, &amount) &&
                amount == size, "Cannot write to the test process.");
    }

    std::uintptr_t image_base() const {
        // The 32-bit PEB holds ImageBaseAddress at +8 before WinMain runs.
        using Query = LONG (NTAPI*)(HANDLE, ULONG, void*, ULONG, ULONG*);
        const auto query = reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),
                                                                   "NtQueryInformationProcess"));
        std::array<std::uintptr_t, 6> pbi{};
        require(query && query(info_.hProcess, 0, pbi.data(), sizeof pbi, nullptr) == 0 && pbi[1],
                "Cannot read the test process PEB.");
        return read(pbi[1] + 8);
    }

private:
    PROCESS_INFORMATION info_{};
};

template<std::size_t Size>
void check_bytes(const Child& child, std::uintptr_t address, const std::array<unsigned char, Size>& expected) {
    std::array<unsigned char, Size> actual{};
    child.read(address, actual.data(), actual.size());
    require(actual == expected, "The mapped Dominion code differs from build 3315.");
}

void write_profile(const fs::path& folder, const Probe& probe, unsigned copies) {
    std::string kingdom = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop";
    for (unsigned i = 0; i < copies; ++i) {
        kingdom += i ? ", " : " -x ";
        kingdom += probe.command;
    }
    std::ofstream profile(folder / L"profiles.xml", std::ios::binary);
    profile << "<ProfileSettings_Save><profiles><playerName>Effekttest</playerName>"
               "<forceOffline>1</forceOffline><profileId>1</profileId><gameSetup>"
               "<players><playerType>Local</playerType><playerName>Effekttest</playerName></players>"
               "<players><playerType>Local</playerType><playerName>Vergleich</playerName></players>"
               "<config><version>23</version><kingdomData>" << kingdom << "</kingdomData></config></gameSetup>"
               "</profiles><lastProfileID>1</lastProfileID></ProfileSettings_Save>";
    profile.close();
    require(profile.good(), "Cannot create a disposable test profile.");
}

void redirect_save_folder(const Child& child, std::uintptr_t base, const fs::path& folder) {
    check_bytes(child, base + 0x23C090,
                std::array<unsigned char, 9>{0x55, 0x8B, 0xEC, 0x81, 0xEC, 0x08, 0x01, 0x00, 0x00});
    const auto target = folder.string() + "\\";
    const auto cache = base + save_folder_cache;
    require(target.size() < MAX_PATH && child.read(cache) == 0, "Save folder cache is unavailable.");
    child.write(cache, target.c_str(), target.size() + 1);
    std::vector<char> readback(target.size() + 1);
    child.read(cache, readback.data(), readback.size());
    require(std::equal(target.begin(), target.end(), readback.begin()) && readback.back() == '\0',
            "Cannot verify the disposable save folder.");
}

std::uintptr_t wait_for_client(const Child& child, std::uintptr_t base, const fs::path& folder) {
    const auto deadline = GetTickCount64() + 45000;
    do {
        require(child.alive(), "The test copy exited during startup.");
        const auto client = child.read(base + client_pointer);
        if (client && child.read(client + 0x5068) == 0 && child.read(client + 0x5044) == 0 &&
            fs::exists(folder / L"log.txt")) return client;
        Sleep(50);
    } while (GetTickCount64() < deadline);
    throw std::runtime_error("The disposable client did not reach the main menu.");
}

bool has_local_save(const fs::path& folder) {
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().extension() == L".xml" && entry.path().filename().native().find(L"gamelocal_") == 0) {
            return true;
        }
    }
    return false;
}

void start_local_game(const Child& child, std::uintptr_t base, std::uintptr_t client, const fs::path& folder) {
    check_bytes(child, base + 0x0B25A7, std::array<unsigned char, 5>{0x32, 0xD2, 0xC7, 0x05, 0x98});
    // DomCreateClick queues action 1 for the native local start dispatcher.
    const DWORD request = 1;
    child.write(client + 0x5044, &request, sizeof request);
    const auto deadline = GetTickCount64() + 10000;
    do {
        require(child.alive(), "The test copy exited during local setup.");
        if (child.read(client + 0x5068) == 1 && child.read(base + game_state + 0x19E0) == 1 &&
            has_local_save(folder)) return;
        Sleep(50);
    } while (GetTickCount64() < deadline);
    throw std::runtime_error("The disposable local game did not reach its first turn.");
}

unsigned hand_count(const Child& child, std::uintptr_t state, DWORD cards, DWORD who) {
    unsigned count = 0;
    for (DWORD i = 0; i < cards; ++i) {
        const auto card = state + 0x1A48 + i * 0x64;
        if (child.read(card + 8) == 0x3EA && child.read(card + 0x28) == who) ++count;
    }
    return count;
}

unsigned owned_count(const Child& child, std::uintptr_t state, DWORD cards, DWORD who, DWORD type) {
    unsigned count = 0;
    for (DWORD i = 1; i < cards; ++i) {
        const auto card = state + 0x1A48 + i * 0x64;
        if (child.read(card + 0x28) == who && (!type || child.read(card + 4) == type)) ++count;
    }
    return count;
}

unsigned supply_count(const Child& child, std::uintptr_t state, DWORD cards, DWORD type) {
    for (unsigned slot = 0x13; slot <= 0x21; ++slot) {
        const auto pile = state + 0x1524 + slot * 0x10;
        if (child.read(pile) != type) continue;
        unsigned count = 0;
        auto handle = child.read(pile + 8);
        while (handle) {
            const auto index = handle & 0xFFFF;
            require(index < cards && count < cards, "Invalid card list in the observed supply.");
            handle = child.read(state + 0x1A48 + index * 0x64 + 0x5C);
            ++count;
        }
        return count;
    }
    throw std::runtime_error("The required supply pile is absent.");
}

bool run_probe(const fs::path& exe, const fs::path& folder, std::wstring& desktop, const Probe& probe, unsigned copies) {
    require(fs::create_directory(folder), "The scratch folder already exists.");
    write_profile(folder, probe, copies);
    Child child(exe, desktop);
    const auto base = child.image_base();
    redirect_save_folder(child, base, folder);
    child.resume();
    const auto client = wait_for_client(child, base, folder);
    start_local_game(child, base, client, folder);
    const auto state = base + game_state;
    const auto players = child.read(state + 0xD38);
    const auto who = child.read(state + 0x19CC);
    const auto cards = child.read(state + 0x1520);
    require(players == 2 && who < players && cards > 2 && cards <= card_capacity,
            "The native first-turn state does not match the fixture.");
    unsigned installed = 0;
    while (installed < 32 && child.read(state + 0x1280 + installed * 0x14) == probe.id) ++installed;
    auto result = 0u;
    const char* metric = nullptr;
    switch (probe.metric) {
        case Metric::hand:
            metric = "hand";
            result = hand_count(child, state, cards, who);
            break;
        case Metric::supply:
            metric = "supply";
            result = supply_count(child, state, cards, probe.card_type);
            break;
        case Metric::owned:
            metric = probe.card_type ? "owned type" : "owned";
            result = owned_count(child, state, cards, who, probe.card_type);
            break;
    }
    const bool pass = installed == copies && result == probe.expected[copies];
    std::printf("%s\t%u\t%u\t%s\t%u\t%u\t%s\n", probe.command, copies, installed, metric, result,
                probe.expected[copies], pass ? "PASS" : "DIFFERENT");
    std::fflush(stdout);
    return pass;
}

}

int wmain(int argc, wchar_t** argv) {
    try {
        require(argc == 2, "Usage: background_effect_test.exe <test Dominion.exe>");
        const auto exe = fs::canonical(argv[1]);
        require(_wcsicmp(exe.filename().c_str(), L"Dominion.exe") == 0, "Expected Dominion.exe.");
        Handle working(CreateFileW(exe.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
        Handle original(CreateFileW(L"E:\\ProgrammeE\\Steam\\steamapps\\common\\Dominion\\Dominion.exe",
                                    FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    nullptr, OPEN_EXISTING, 0, nullptr));
        require(working.valid() && original.valid() && !same_file(identify(working.value), identify(original.value)),
                "The test requires a separate Dominion working copy.");
        verify_executable(working.value);

        auto name = L"DominionBatch_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
                    std::to_wstring(GetTickCount64());
        const auto run = fs::temp_directory_path() / name;
        HiddenDesktop isolated(name);
        require(fs::create_directory(run), "The scratch run directory already exists.");
        std::puts("effect\tcopies\tinstalled\tmetric\tmeasured\texpected\tstatus");
        unsigned case_number = 0;
        bool all_pass = true;
        for (const auto& probe : probes) {
            for (unsigned copies = 0; copies < probe.expected.size(); ++copies) {
                const auto number = std::to_wstring(++case_number);
                try {
                    if (!run_probe(exe, run / number, name, probe, copies)) all_pass = false;
                } catch (const std::exception& error) {
                    std::fprintf(stderr, "%s x%u (case %u): %s\n", probe.command, copies, case_number, error.what());
                    all_pass = false;
                }
            }
        }
        std::printf("Scratch logs: %ls\n", run.c_str());
        return all_pass ? 0 : 1;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
