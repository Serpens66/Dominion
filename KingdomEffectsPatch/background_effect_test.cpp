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
constexpr DWORD gold = 0x106;
constexpr DWORD estate = 0x101;
constexpr DWORD province = 0x103;
constexpr DWORD chapel = 0x201;
constexpr DWORD village = 0x206;
constexpr DWORD smithy = 0x20E;
constexpr DWORD goat = 0xD23;
constexpr DWORD search = 0x1104;
constexpr DWORD horse = 0xF46;
constexpr DWORD spoils = 0x92E;
constexpr DWORD arena = 0xC33;
constexpr DWORD pageant = 0xE20;
constexpr DWORD loot = 0x1128;
constexpr DWORD rewards = 0x718;
constexpr DWORD patrician = 0xC06;
constexpr DWORD curse = 0x100;
constexpr DWORD travelling_fair = 0xB23;
constexpr DWORD dominate_event = 0xC29;

constexpr char base_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop";
constexpr char pixie_kingdom[] = "-k pixie, market, merchant, militia, mine, moat, remodel, smithy, village, workshop";
constexpr char search_kingdom[] = "-k search, market, merchant, militia, mine, moat, remodel, smithy, village, workshop";
constexpr char fair_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                "travelling_fair, -m Landscapes(0,1)";
constexpr char delve_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                 "delve, -m Landscapes(0,1)";
constexpr char triumph_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                   "triumph, -m Landscapes(0,1)";
constexpr char dominate_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                    "dominate, travelling_fair, -m Landscapes(0,2)";
constexpr char wedding_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                   "wedding, travelling_fair, -m Landscapes(0,2)";
constexpr char arena_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                 "arena, -m Landscapes(0,1)";
constexpr char aqueduct_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                    "aqueduct, -m Landscapes(0,1)";
constexpr char shrine_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                  "defiled_shrine, -m Landscapes(0,1)";
constexpr char pageant_kingdom[] = "-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, "
                                   "pageant, -m Landscapes(0,1)";
constexpr char loot_kingdom[] = "-k pickaxe, market, merchant, militia, mine, moat, remodel, smithy, village, workshop";
constexpr char joust_kingdom[] = "-k joust, market, merchant, militia, mine, moat, remodel, smithy, village, workshop";
constexpr char patrician_kingdom[] = "-k patrician, market, merchant, militia, mine, moat, remodel, smithy, village, workshop";
constexpr char free_fair[] = "stamp_got_it_already(travelling_fair)";

struct Purchase {
    const char* effects;
    unsigned buys;
    DWORD gained_type = 0;
    unsigned min_owned = 0;
};

constexpr Purchase fair{free_fair, 3};
constexpr Purchase fair_silver{
    "stamp_got_it_already(travelling_fair), stamp_also_silver(travelling_fair)", 3, silver, 1};
constexpr Purchase delve{"stamp_got_it_already(delve)", 2, silver, 1};
constexpr Purchase triumph{"stamp_got_it_already(triumph)", 1, estate, 4};
constexpr Purchase dominate{"stamp_got_it_already(dominate)", 1, province, 1};
constexpr Purchase wedding{"stamp_got_it_already(wedding)", 1, gold, 1};
constexpr Purchase delve_duration{
    "stamp_got_it_already(delve), stamp_durationified(silver)", 2, silver, 1};

enum class Metric {
    hand, supply, pile_types, owned, location, token, token_face, pile_tokens, project_cube, money, actions, buys,
    price, card_type, mixed_pile
};

struct Probe {
    const char* command;
    DWORD id;
    std::array<unsigned, 4> expected;
    Metric metric = Metric::hand;
    DWORD card_type = 0;
    DWORD qualifier = 0;
    unsigned max_copies = 3;
    const char* kingdom = base_kingdom;
    const Purchase* purchase = nullptr;
};

constexpr Probe probes[] = {
    {"twist_big_ideas", 0x1774, {5, 6, 7, 8}},
    {"twist_cramped", 0x17DF, {5, 4, 3, 2}},
    {"twist_penny_party", 0x1775, {7, 8, 9, 10}, Metric::owned, copper},
    {"twist_busy_busy", 0x1771, {1, 2, 3, 4}, Metric::actions},
    {"twist_shoppers", 0x1772, {1, 2, 3, 4}, Metric::buys},
    {"twist_extra_oomph", 0x1773, {0, 1, 2, 3}, Metric::money},
    {"start_double_pile(village)", 0xDB8, {10, 20, 40, 80}, Metric::supply, village},
    {"start_triple_pile(village)", 0xDBA, {10, 30, 90, 270}, Metric::supply, village},
    {"start_add_card(silver)", 0xDB5, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_add_each_pile", 0xDB6, {10, 20, 30, 40}, Metric::owned},
    {"start_double_deck", 0xDB7, {10, 20, 40, 80}, Metric::owned},
    {"start_replace_copper(silver)", 0xDB1, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_replace_estate(silver)", 0xDB2, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_replace_copper_from_pile(silver)", 0xDB3, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_replace_estate_from_pile(silver)", 0xDB4, {0, 1, 2, 3}, Metric::owned, silver},
    {"start_5_coffers", 0xDC3, {0, 5, 10, 15}, Metric::token, 0xA00},
    {"start_10_villagers", 0xDC4, {0, 10, 20, 30}, Metric::token, 0xE00},
    {"start_10_favor", 0xDC5, {0, 10, 20, 30}, Metric::token, 0x1000},
    {"twist_forever_favored", 0x183A, {0, 1, 2, 3}, Metric::token, 0x1000},
    {"start_journey_facedown", 0xDBD, {0, 1, 0, 1}, Metric::token_face, 0xB01},
    {"start_trash_curses", 0xDBB, {0, 10, 20, 30}, Metric::location, 0x100, 2},
    {"start_trash_pile(silver)", 0xDBC, {0, 40, 80, 120}, Metric::location, silver, 2},
    {"start_native_village(silver)", 0xDBE, {0, 1, 2, 3}, Metric::location, silver, 0x45F},
    {"start_tavern(copper)", 0xDBF, {0, 1, 2, 3}, Metric::location, copper, 0x461},
    {"start_make_split_pile(village,smithy)", 0xDC6, {0, 5, 5, 5}, Metric::supply, village, smithy},
    {"start_in_play(search)", 0xDB9, {0, 1, 2, 3}, Metric::location, search, 0x3E9, 3, search_kingdom},
    {"start_double_heirloom(pixie)", 0xDC0, {1, 2, 3, 4}, Metric::owned, goat, 0, 3, pixie_kingdom},
    {free_fair, 0x141E, {1, 3, 5, 7}, Metric::buys, 0, 0, 3, fair_kingdom},
    {"stamp_bonus_money(travelling_fair)", 0x141F, {0, 1, 2, 3}, Metric::money, 0, 0, 3, fair_kingdom, &fair},
    {"stamp_also_silver(travelling_fair)", 0x1425, {0, 1, 1, 1}, Metric::owned, silver, 0, 3, fair_kingdom, &fair},
    {"twist_favored_wealthy", 0x1839, {0, 1, 2, 3}, Metric::token, 0x1000, 0, 3, fair_kingdom, &fair_silver},
    {"twist_favored_landlords", 0x1838, {0, 1, 2, 3}, Metric::token, 0x1000, 0, 3, triumph_kingdom, &triumph},
    {"twist_rebates", 0x1777, {0, 1, 2, 3}, Metric::money, 0, 0, 3, delve_kingdom, &delve},
    {"twist_double_up", 0x17E0, {1, 2, 3, 4}, Metric::owned, silver, 0, 3, delve_kingdom, &delve},
    {"twist_incoming", 0x17D9, {0, 1, 1, 1}, Metric::location, silver, 0x3EB, 3, delve_kingdom, &delve},
    {"stamp_with_villager(silver)", 0x1B67, {0, 1, 2, 3}, Metric::token, 0xE00, 0, 3, delve_kingdom, &delve},
    {"stamp_with_horse(silver)", 0x1B68, {0, 1, 2, 3}, Metric::owned, horse, 0, 3, delve_kingdom, &delve},
    {"stamp_with_spoils(silver)", 0x1B66, {0, 1, 2, 3}, Metric::owned, spoils, 0, 3, delve_kingdom, &delve},
    {"stamp_two_for_one(silver)", 0x1B5A, {1, 2, 3, 4}, Metric::owned, silver, 0, 3, delve_kingdom, &delve},
    {"stamp_fawning_service(travelling_fair)", 0x1422, {1, 3, 3, 3}, Metric::buys, 0, 0, 3, dominate_kingdom, &dominate},
    {"stamp_perks_of_wealth(travelling_fair)", 0x1426, {1, 3, 3, 3}, Metric::buys, 0, 0, 3, wedding_kingdom, &wedding},
    {"start_double_vp_tokens(arena)", 0x1451, {12, 24, 36, 48}, Metric::pile_tokens, 0x601, arena, 3, arena_kingdom},
    {"start_plus_8_vp_tokens(aqueduct)", 0x1452, {8, 16, 24, 32}, Metric::pile_tokens, 0x601, silver, 3, aqueduct_kingdom},
    {"start_plus_2_vp_tokens(defiled_shrine)", 0x1453, {2, 4, 6, 8}, Metric::pile_tokens, 0x601, village, 3, shrine_kingdom},
    {"start_with_cube(pageant)", 0x14B5, {0, 1}, Metric::project_cube, 0xE01, pageant, 1, pageant_kingdom},
    {"twist_mirror_loot", 0x17E2, {15, 1, 1, 1}, Metric::pile_types, loot, 30, 3, loot_kingdom},
    {"twist_room_for_more(chapel)", 0x17D8, {0, 10}, Metric::supply, chapel, 0, 1},
    {"start_same_reward", 0xDC7, {6, 1, 1, 1}, Metric::pile_types, rewards, 6, 3, joust_kingdom},
    {"start_shuffle_pile(patrician)", 0xDC1, {0, 1, 1, 1}, Metric::mixed_pile, patrician, 10, 3, patrician_kingdom},
    {"start_curse_cost_3", 0xDC2, {0, 300, 600, 900}, Metric::price, curse},
    {"twist_sale", 0x17D7, {300, 200, 100, 0}, Metric::price, village},
    {"stamp_discounted(village)", 0x1B5B, {300, 200, 100, 0}, Metric::price, village},
    {"stamp_pay_later(village)", 0x1B69, {300, 3, 3, 3}, Metric::price, village},
    {"stamp_cost_zero(travelling_fair)", 0x1420, {200, 0, 0, 0}, Metric::price, travelling_fair, 0, 3, fair_kingdom},
    {"stamp_cost_2_less(dominate)", 0x1424, {1400, 1200, 1000, 800}, Metric::price, dominate_event, 0, 3, dominate_kingdom},
    {"stamp_two_for_one(village)", 0x1B5A, {300, 400, 500, 600}, Metric::price, village},
    {"stamp_treasurified(village)", 0x1B5E, {0, 1, 1, 1}, Metric::card_type, village, 0x2},
    {"stamp_victorified(village)", 0x1B5C, {0, 1, 1, 1}, Metric::card_type, village, 0x8},
    {"stamp_durationified(village)", 0x1B5D, {0, 1, 1, 1}, Metric::card_type, village, 0x80},
    {"stamp_reactionified(village)", 0x1B5F, {0, 1, 1, 1}, Metric::card_type, village, 0x40},
    {"stamp_shadowified(village)", 0x1B60, {0, 1, 1, 1}, Metric::card_type, village, 0x1000},
    {"twist_fast_track", 0x17DC, {0, 1, 0, 0}, Metric::location, silver, 0x46D, 3, delve_kingdom, &delve_duration},
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

    template<std::size_t Size, std::size_t Count>
    DWORD query(const std::array<BYTE, Size>& code, const std::array<DWORD, Count>& args, DWORD timeout=3000) const {
        // Give the disposable child one data page and a separate executable page for the query.
        constexpr SIZE_T page = 4096;
        static_assert(Size < page && sizeof(args) < page);
        const auto memory = reinterpret_cast<std::uintptr_t>(
            VirtualAllocEx(info_.hProcess, nullptr, page * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        require(memory, "Cannot allocate the native query.");
        write(memory, code.data(), code.size());
        write(memory + page, args.data(), sizeof(args));
        DWORD protection;
        require(VirtualProtectEx(info_.hProcess, reinterpret_cast<void*>(memory), page, PAGE_EXECUTE_READ, &protection) &&
                FlushInstructionCache(info_.hProcess, reinterpret_cast<void*>(memory), code.size()),
                "Cannot prepare the native query.");
        Handle thread(CreateRemoteThread(info_.hProcess, nullptr, 0,
                                        reinterpret_cast<LPTHREAD_START_ROUTINE>(memory),
                                        reinterpret_cast<void*>(memory + page), 0, nullptr));
        require(thread.valid() && WaitForSingleObject(thread.value, timeout) == WAIT_OBJECT_0,
                "The native query did not finish.");
        DWORD result;
        require(GetExitCodeThread(thread.value, &result) && alive(), "The native query failed.");
        require(VirtualFreeEx(info_.hProcess, reinterpret_cast<void*>(memory), 0, MEM_RELEASE),
                "Cannot release the native query.");
        return result;
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
    std::string kingdom = probe.kingdom;
    if (probe.purchase) {
        kingdom += " -x ";
        kingdom += probe.purchase->effects;
    }
    for (unsigned i = 0; i < copies; ++i) {
        kingdom += (i || probe.purchase) ? ", " : " -x ";
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

unsigned location_count(const Child& child, std::uintptr_t state, DWORD cards, DWORD who,
                        DWORD type, DWORD where) {
    unsigned count = 0;
    for (DWORD i = 1; i < cards; ++i) {
        const auto card = state + 0x1A48 + i * 0x64;
        if (child.read(card + 8) == where && child.read(card + 4) == type &&
            (where == 2 || child.read(card + 0x28) == who)) ++count;
    }
    return count;
}

unsigned token_count(const Child& child, std::uintptr_t state, DWORD who, DWORD type, bool face, DWORD where = 0) {
    const auto records = child.read(state + 0x19A4);
    require(records <= 256, "Invalid token count.");
    unsigned count = 0;
    for (DWORD i = 0; i < records; ++i) {
        const auto token = state + 0x152C8 + i * 0x20;
        if (child.read(token) == type && child.read(token + 4) == who && (!where || child.read(token + 8) == where)) {
            count += face ? (child.read(token + 0x10) & 0xFF) : child.read(token + 0x14);
        }
    }
    return count;
}

DWORD pile_slot(const Child& child, std::uintptr_t state, DWORD type) {
    for (DWORD slot = 7; slot < 0x48; ++slot) {
        const auto pile = state + 0x1524 + slot * 0x10;
        if (child.read(pile) == type || child.read(pile + 4) == type) return slot;
    }
    throw std::runtime_error("The test pile is missing from the fixture.");
}

unsigned observed_price(const Child& child, DWORD base, DWORD state, DWORD who, DWORD card) {
    const auto cost_query = base + 0x176E90;
    check_bytes(child, cost_query, std::array<BYTE, 6>{0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0});
    constexpr std::array<BYTE, 29> call{
        0x8B, 0x44, 0x24, 4, // mov eax, [esp+4]    (query arguments)
        0x83, 0xEC, 4,       // sub esp, 4          (packed price output)
        0x8B, 0xCC,          // mov ecx, esp
        0x6A, 1,             // push 1              (skip card callbacks)
        0xFF, 0x70, 12,      // push [eax+12]       (card)
        0xFF, 0x70, 8,       // push [eax+8]        (player)
        0x8B, 0x50, 4,       // mov edx, [eax+4]    (game state)
        0xFF, 0x10,          // call [eax]
        0x83, 0xC4, 12,      // add esp, 12
        0x58,                // pop eax
        0xC2, 4, 0           // ret 4
    };
    const auto price = child.query(call, std::array<DWORD, 4>{cost_query, state, who, card});
    return (price & 0xFFFF) * 100 + (price >> 24);
}

unsigned observed_type(const Child& child, DWORD base, DWORD state, DWORD card, DWORD mask) {
    const auto type_query = base + 0x1754F0;
    check_bytes(child, type_query, std::array<BYTE, 5>{0x55, 0x8B, 0xEC, 0x81, 0xEC});
    constexpr std::array<BYTE, 27> call{
        0x8B, 0x44, 0x24, 4, // mov eax, [esp+4]    (query arguments)
        0xFF, 0x70, 16,      // push [eax+16]       (type mask high)
        0xFF, 0x70, 12,      // push [eax+12]       (type mask low)
        0x8B, 0x50, 8,       // mov edx, [eax+8]    (card)
        0x8B, 0x48, 4,       // mov ecx, [eax+4]    (game state)
        0xFF, 0x10,          // call [eax]
        0x83, 0xC4, 8,       // add esp, 8
        0x0F, 0xB6, 0xC0,    // movzx eax, al
        0xC2, 4, 0           // ret 4
    };
    const bool high = mask == 0x1000;
    return child.query(call, std::array<DWORD, 5>{type_query, state, card, high ? 0 : mask, high ? mask : 0});
}

struct PileComposition {
    unsigned types;
    unsigned groups;
};

PileComposition pile_composition(const Child& child, std::uintptr_t state, DWORD cards, DWORD type, DWORD expected_cards) {
    const auto slot = pile_slot(child, state, type);
    auto handle = child.read(state + 0x152C + slot * 0x10);
    std::vector<DWORD> types;
    DWORD visited = 0, previous_type = 0;
    unsigned groups = 0;
    while (handle) {
        const auto index = handle & 0xFFFF;
        require(index < cards && visited++ < cards, "Invalid card list in the observed pile.");
        const auto card = state + 0x1A48 + index * 0x64;
        const auto card_type = child.read(card + 4);
        if (card_type != previous_type) ++groups;
        previous_type = card_type;
        if (std::find(types.begin(), types.end(), card_type) == types.end()) types.push_back(card_type);
        handle = child.read(card + 0x5C);
    }
    require(visited == expected_cards, "The observed pile has an unexpected size.");
    return {static_cast<unsigned>(types.size()), groups};
}

unsigned supply_count(const Child& child, std::uintptr_t state, DWORD cards, DWORD type, DWORD filter) {
    for (unsigned slot = 0x13; slot <= 0x21; ++slot) {
        const auto pile = state + 0x1524 + slot * 0x10;
        if (child.read(pile) != type) continue;
        unsigned count = 0, visited = 0;
        auto handle = child.read(pile + 8);
        while (handle) {
            const auto index = handle & 0xFFFF;
            require(index < cards && visited++ < cards, "Invalid card list in the observed supply.");
            const auto card = state + 0x1A48 + index * 0x64;
            if (!filter || child.read(card + 4) == filter) ++count;
            handle = child.read(card + 0x5C);
        }
        return count;
    }
    return 0;
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
    const auto resources = state + 0x174F4 + who * 0x5A30;
    unsigned installed = 0, event_purchase = 0;
    for (unsigned i = 0; i < 32; ++i) {
        const auto id = child.read(state + 0x1280 + i * 0x14);
        if (id == probe.id) ++installed;
        if (id == 0x141E) ++event_purchase;
    }
    if (probe.purchase) {
        require(event_purchase == 1 && (probe.metric == Metric::buys || child.read(resources + 8) == probe.purchase->buys),
                "The free event purchase is missing from the fixture.");
        require(!probe.purchase->gained_type ||
                    owned_count(child, state, cards, who, probe.purchase->gained_type) >= probe.purchase->min_owned,
                "The event's card gain is missing from the fixture.");
    }
    auto result = 0u;
    const char* metric = nullptr;
    switch (probe.metric) {
        case Metric::hand:
            metric = "hand";
            result = hand_count(child, state, cards, who);
            break;
        case Metric::supply:
            metric = "supply";
            result = supply_count(child, state, cards, probe.card_type, probe.qualifier);
            break;
        case Metric::pile_types:
            metric = "distinct pile cards";
            result = pile_composition(child, state, cards, probe.card_type, probe.qualifier).types;
            break;
        case Metric::mixed_pile: {
            metric = "split pile interleaved";
            const auto pile = pile_composition(child, state, cards, probe.card_type, probe.qualifier);
            require(pile.types == 2, "Expected two card types in the split pile.");
            result = pile.groups > 2;
            break;
        }
        case Metric::owned:
            metric = probe.card_type ? "owned type" : "owned";
            result = owned_count(child, state, cards, who, probe.card_type);
            break;
        case Metric::location:
            metric = "cards at location";
            result = location_count(child, state, cards, who, probe.card_type, probe.qualifier);
            break;
        case Metric::token:
        case Metric::token_face:
            metric = probe.metric == Metric::token ? "tokens" : "token face";
            result = token_count(child, state, who, probe.card_type, probe.metric == Metric::token_face);
            break;
        case Metric::pile_tokens:
            metric = "pile tokens";
            result = token_count(child, state, DWORD(-1), probe.card_type, false,
                                 pile_slot(child, state, probe.qualifier));
            break;
        case Metric::project_cube:
            metric = "assigned project cubes";
            result = token_count(child, state, who, probe.card_type, false,
                                 pile_slot(child, state, probe.qualifier));
            break;
        case Metric::price:
            metric = "coins*100+debt";
            result = observed_price(child, base, state, who, probe.card_type);
            break;
        case Metric::card_type:
            metric = "card type present";
            result = observed_type(child, base, state, probe.card_type, probe.qualifier);
            break;
        case Metric::money:
        case Metric::actions:
        case Metric::buys: {
            const auto resource = probe.metric == Metric::money ? 0 : probe.metric == Metric::actions ? 1 : 2;
            metric = probe.metric == Metric::money ? "money" : probe.metric == Metric::actions ? "actions" : "buys";
            result = child.read(resources + resource * sizeof(DWORD));
            break;
        }
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
        require(argc == 2 || argc == 3, "Usage: background_effect_test.exe <test Dominion.exe> [effect-prefix]");
        const std::wstring_view prefix = argc == 3 ? argv[2] : L"";
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
            const std::string_view command(probe.command);
            if (std::wstring(command.begin(), command.end()).compare(0, prefix.size(), prefix) != 0) continue;
            for (unsigned copies = 0; copies <= probe.max_copies; ++copies) {
                try {
                    // A shuffle can leave the two groups intact by chance. Check fresh games in that case.
                    const unsigned attempts = probe.metric == Metric::mixed_pile && copies ? 3 : 1;
                    bool pass = false;
                    for (unsigned attempt = 0; attempt < attempts && !pass; ++attempt) {
                        pass = run_probe(exe, run / std::to_wstring(++case_number), name, probe, copies);
                    }
                    if (!pass) all_pass = false;
                } catch (const std::exception& error) {
                    std::fprintf(stderr, "%s x%u (case %u): %s\n", probe.command, copies, case_number, error.what());
                    all_pass = false;
                }
            }
        }
        require(case_number, "No test cases matched the effect prefix.");
        std::printf("Scratch logs: %ls\n", run.c_str());
        return all_pass ? 0 : 1;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
