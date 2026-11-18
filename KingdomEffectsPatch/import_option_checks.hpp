#pragma once
#include "import_setup_options.hpp"
#include <cassert>
#include <cstdio>

namespace import_options_fixture {
using namespace kingdom;
ImportedSetupOptions scan(std::string_view text) {
    ImportedSetupOptions result;
    assert(scan_import_setup_options(text, result) == ImportOptionsError::none);
    return result;
}
void write(Setup& setup, std::size_t offset, std::uint32_t value) { std::memcpy(setup.data() + offset, &value, 4); }
std::uint32_t read(const Setup& setup, std::size_t offset) { std::uint32_t value; std::memcpy(&value, setup.data() + offset, 4); return value; }
}
inline void run_import_option_checks() {
    using namespace import_options_fixture;
    using namespace kingdom;
    for (const auto text : {"-k village,colonies,shelters", "-K VILLAGE, CoLoNiEs, SHELTERS", "village, colonies, shelters",
        "-m Colonies,Shelters", "-m NoColonies,NoShelters", "-k colonies -m NoColonies,Shelters"}) {
        const auto value = scan(text); assert(value.colonies && value.shelters && !value.landscapes);
    }
    for (const auto text : {"-k village", "-k young_witch(colonies),druid(shelters,colonies)",
        "-x twist_typhoon(colonies,shelters) -e colonies,shelters", "-m NoColoniesExtra,SheltersExtra,LandscapesExtra(4,4)",
        "-k colony,landscapes(4,4)", "-e colonies -x shelters"}) {
        const auto value = scan(text); assert(!value.colonies && !value.shelters && !value.landscapes);
    }
    for (const auto text : {"-m Landscapes(4,4)", "-m landscapes(0,4)", "-m LANDSCAPES(0)",
        "-k colonies -m Landscapes( 2 , 3 ), NoShelters", "-m Landscapes(1,1),Landscapes(4,4)"}) assert(scan(text).landscapes);
    for (const auto text : {"-m Landscapes", "-m Landscapes()", "-m Landscapes(,1)", "-m Landscapes(1,)",
        "-m Landscapes(-1,2)", "-m Landscapes(5,5)", "-m Landscapes(3,2)", "-m Landscapes(1,2,3)",
        "-m Landscapes(99999999999999999999999,4)", "-m Landscapes(1abc,2)", "-m Colonies(1)",
        "-k village)", "-k druid(shelters", "-m Landscapes((1),2)"}) {
        ImportedSetupOptions unchanged{true,true,true};
        assert(scan_import_setup_options(text, unchanged) == ImportOptionsError::malformed);
        assert(unchanged.colonies && unchanged.shelters && unchanged.landscapes);
    }
    const auto flags = scan("-k colonies,shelters -m Landscapes(4,4),NoColonies,NoShelters");
    Setup current{}, parsed{};
    current.fill(std::byte{0xA5}); parsed.fill(std::byte{0xCC});
    write(parsed, import_colonies_offset, 2); write(parsed, import_shelters_offset, 2);
    write(parsed, import_landscapes_offset, 4); write(parsed, import_landscapes_offset+4, 4);
    const auto before = current;
    assert(apply_import_setup_options(current,parsed,flags) == ImportOptionsError::none);
    assert(read(current,import_colonies_offset)==2 && read(current,import_shelters_offset)==2);
    for(std::size_t i=0;i<current.size();++i) {
        if((i>=import_colonies_offset && i<import_colonies_offset+4) ||
           (i>=import_shelters_offset && i<import_shelters_offset+4) ||
           (i>=import_landscapes_offset && i<import_landscapes_offset+8)) continue;
        assert(current[i]==before[i]);
    }
    // Omitted fields are preserved, and zero/automatic native defaults are not explicit.
    current=before;
    assert(apply_import_setup_options(current,parsed,scan("-k village")) == ImportOptionsError::none && current==before);
    for (unsigned colonies : {1u,2u}) for(unsigned shelters : {1u,2u}) {
        write(parsed,import_colonies_offset,colonies);write(parsed,import_shelters_offset,shelters);
        current=before;
        assert(apply_import_setup_options(current,parsed,flags)==ImportOptionsError::none);
        assert(read(current,import_colonies_offset)==colonies && read(current,import_shelters_offset)==shelters);
    }
    // Validate everything before writing any field: invalid quota or option is atomic.
    for(unsigned value : {0u,3u,0xFFFFFFFFu}) {
        write(parsed,import_colonies_offset,value);current=before;
        assert(apply_import_setup_options(current,parsed,flags)==ImportOptionsError::invalid_native_value && current==before);
    }
    write(parsed,import_colonies_offset,1);write(parsed,import_shelters_offset,2);
    write(parsed,import_landscapes_offset,3);write(parsed,import_landscapes_offset+4,2);current=before;
    assert(apply_import_setup_options(current,parsed,flags)==ImportOptionsError::invalid_native_value && current==before);
    write(parsed,import_landscapes_offset,0);write(parsed,import_landscapes_offset+4,5);
    assert(apply_import_setup_options(current,parsed,flags)==ImportOptionsError::invalid_native_value && current==before);
    // An invalid omitted native field is irrelevant.
    assert(apply_import_setup_options(current,parsed,scan("-k colonies"))==ImportOptionsError::none);
    assert(read(current,import_colonies_offset)==1 && read(current,import_shelters_offset)==read(before,import_shelters_offset));
    assert(apply_import_setup_options(nullptr,parsed.data(),flags)==ImportOptionsError::invalid_native_value);
    std::puts("PASS: explicit setup option presence, native-value copying, omitted fields and atomic validation.");
}
