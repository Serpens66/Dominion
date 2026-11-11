#pragma once
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <filesystem>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace kingdom {
using ResourceImage = std::shared_ptr<Gdiplus::Bitmap>;
ResourceImage game_art(std::string_view name);
ResourceImage game_card(std::string_view id);
std::size_t resource_cache_bytes();
// Developer checks can select an installation without launching Dominion.
bool set_resource_package_for_test(const std::filesystem::path& path);
class CardPreviewLoader {
public:
    CardPreviewLoader();
    ~CardPreviewLoader();
    void request(std::string id);
    bool take(std::string& id, ResourceImage& image);
    std::pair<std::uint64_t, std::uint64_t> counts() const;
private:
    struct State;
    std::unique_ptr<State> state_;
};
}
