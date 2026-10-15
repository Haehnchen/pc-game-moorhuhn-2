#pragma once

#include "assets/asset_store.hpp"
#include "contracts/draw.hpp"

#include <map>
#include <span>
#include <string>
#include <string_view>

namespace moorhuhn2::render {

struct FrameGrid {
    std::string_view image;
    int columns{}, rows{}, cell_width{}, cell_height{};
};

class FrameCatalog final {
public:
    explicit FrameCatalog(std::span<const FrameGrid> grids);
    explicit FrameCatalog(const assets::AssetStore& assets);
    [[nodiscard]] contracts::ImageRect frame_source(std::string_view image, int frame) const;

private:
    struct Grid {
        int columns{}, rows{}, cell_width{}, cell_height{};
    };

    void add(FrameGrid grid);
    std::map<std::string, Grid, std::less<>> grids_;
};

} // namespace moorhuhn2::render
