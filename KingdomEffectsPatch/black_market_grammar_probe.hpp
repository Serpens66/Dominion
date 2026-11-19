// Developer-only grammar probe. No game start, input devices or user profile.
#pragma once

inline DWORD run_black_market_grammar_probe() {
    using Parse = void (__fastcall*)(const char*, kingdom::Setup*);
    const auto parse = reinterpret_cast<Parse>(native_parser);
    constexpr char cards[] = "cellar,chapel,moat,village,workshop,bureaucrat,gardens,militia,moneylender,remodel,smithy,throne_room,festival,laboratory,library";
    constexpr char main[] = "-k cellar,market,merchant,militia,mine,moat,remodel,smithy,village,workshop";
    const std::string effect = " -x start_add_card(black_market)";
    struct Case { const char* name; std::string text; };
    const Case cases[]{
        {"baseline", std::string(main) + effect},
        {"main-parent-control", std::string("-k cellar,market,merchant,militia,mine,moat,remodel,smithy,village,black_market(") + cards + ")" + effect},
        {"standalone-deck", std::string(main) + ",black_market_deck(" + cards + ")" + effect},
        {"eleventh-parent", std::string(main) + ",black_market(" + cards + ")" + effect},
        {"modifier-candidate", std::string(main) + " -m BlackMarket(" + cards + ")" + effect},
        {"nested-effect-target", std::string(main) + " -x start_add_card(black_market(" + cards + "))"},
    };
    HMODULE module{}; wchar_t path[32768]{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&run_black_market_grammar_probe), &module);
    GetModuleFileNameW(module, path, 32768);
    std::ofstream report(std::filesystem::path(path).parent_path() / L"black-market-grammar.tsv", std::ios::binary);
    if (!report) return 881;
    report << "case\tmain_before\tmain_after\taux_before\taux_after\teffect_before\teffect_after\tunchanged_main\tunchanged_aux\tstandalone_roundtrip\tserialized\n" << std::unitbuf;
    const auto read_ids = [](const kingdom::Setup& setup, unsigned start, unsigned maximum, unsigned stride) {
        std::vector<DWORD> result;
        for (unsigned n = 0; n < maximum; ++n) {
            const auto* item = reinterpret_cast<const DWORD*>(setup.data() + start + n * stride);
            if (item[0] == 1 && item[1]) result.push_back(item[1]);
        }
        return result;
    };
    const auto main_ids = [&](const kingdom::Setup& setup) { return read_ids(setup, 0, 10, 16); };
    // Black Market's native type-D record: supplemental index 24, up to 60.
    const auto aux_ids = [&](const kingdom::Setup& setup) { return read_ids(setup, 0x1C0 + 24 * 16, 60, 16); };
    const auto list = [&](const std::vector<DWORD>& ids) {
        for (std::size_t i = 0; i < ids.size(); ++i) { if (i) report << ','; report << ids[i]; }
    };
    kingdom::Setup baseline{}; parse((std::string(main) + effect).c_str(), &baseline);
    const auto wanted_main = main_ids(baseline);
    unsigned failures = 0, candidates = 0;
    for (const auto& test : cases) {
        kingdom::Setup before{}, after{}; parse(test.text.c_str(), &before);
        const NativeSetupText serialized(before, 23);
        const auto text = std::string(serialized.view());
        parse(text.c_str(), &after);
        const auto m1 = main_ids(before), m2 = main_ids(after), a1 = aux_ids(before), a2 = aux_ids(after);
        const auto f1 = kingdom::read_effects(before.data()), f2 = kingdom::read_effects(after.data());
        const bool effect_ok = kingdom::count(f2) == 1 && f2[0][0] == 0xDB5 && f2[0][1] == 0x1300 && f1 == f2;
        const bool standalone = effect_ok && a1.size() >= 15 && a1 == a2 && m1 == wanted_main && m2 == wanted_main;
        if (standalone) ++candidates;
        if (std::string_view(test.name) == "main-parent-control" &&
            (a1.size() != 15 || a1 != a2 || m1 != m2 || !effect_ok)) ++failures;
        report << test.name << '\t'; list(m1); report << '\t'; list(m2);
        report << '\t' << a1.size() << '\t' << a2.size() << '\t' << kingdom::count(f1) << '\t' << kingdom::count(f2)
            << '\t' << (m1 == m2) << '\t' << (a1 == a2) << '\t' << standalone << '\t' << text << '\n';
    }
    report << "# standalone_candidates=" << candidates << " control_failures=" << failures << '\n';
    return failures ? 882 : 0;
}
