#pragma once
#include "import_setup_options.hpp"
#include "target_access_runtime.hpp"
#include "native_card_access.hpp"

namespace kingdom {
inline const TargetAccessNativeCard* import_native_card(std::uint32_t numeric) {
    for (const auto& card:access_catalog::cards) if(card.native_id==numeric)return &card;
    return nullptr;
}
// Only real card IDs from the parsed native structures are visited. Counts,
// random descriptors and negative numeric assignment shorthands are not cards.
inline std::vector<std::uint32_t> imported_card_ids(const void* parsed,const ImportedSetupOptions& options) {
    const auto* data=static_cast<const std::byte*>(parsed);
    std::vector<std::uint32_t> ids;
    const auto read=[&](std::size_t at){std::uint32_t v=0;std::memcpy(&v,data+at,4);return v;};
    const auto add=[&](std::uint32_t id){if(id && std::find(ids.begin(),ids.end(),id)==ids.end())ids.push_back(id);};
    const auto descriptor=[&](std::size_t at){if(read(at)==1)add(read(at+4));};
    for(unsigned i=0;i<10;++i)descriptor(i*0x10);
    for(unsigned i=0;i<4;++i){
        const auto at=0xA0+i*0x48;
        descriptor(at);
        if(read(at)==1)for(unsigned j=0;j<14;++j){const auto id=read(at+0x10+j*4);if(static_cast<std::int32_t>(id)>0)add(id);}
    }
    // Typed supplemental card assignments. Typhoon/Room for More are effect
    // target storage and are already covered by the existing effect validator.
    for(unsigned i=0;i<107;++i){const auto at=0x1C0+i*0x10;if((at>=0x710 && at<0x850)||at==0x860)continue;descriptor(at);}
    const auto option_card=[&](const char* name){const auto* card=access_native_card(name);if(card)add(card->native_id);else add(0xFFFFFFFFu);};
    if(options.colonies && read(import_colonies_offset)==1){option_card("platinum");option_card("colony");}
    if(options.shelters && read(import_shelters_offset)==1){option_card("hovel");option_card("necropolis");option_card("overgrown_estate");}
    return ids;
}
}
