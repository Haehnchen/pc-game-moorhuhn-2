#include "render/frame_catalog.hpp"

#include <limits>
#include <stdexcept>

namespace moorhuhn2::render {

FrameCatalog::FrameCatalog(std::span<const FrameGrid> grids) {
    for (const auto& grid : grids) {
        add(grid);
    }
}

FrameCatalog::FrameCatalog(const assets::AssetStore& assets) {
    for (const auto& id : assets.image_ids()) {
        const auto grid = assets.sprite_grid(id);

        if (!grid) {
            continue;
        }

        add({id, static_cast<int>(grid->columns), static_cast<int>(grid->rows), static_cast<int>(grid->cell_width), static_cast<int>(grid->cell_height)});
    }
}

void FrameCatalog::add(FrameGrid grid) {
    if (grid.image.empty() || grid.columns <= 0 || grid.rows <= 0 || grid.cell_width <= 0 || grid.cell_height <= 0 || grid.columns > std::numeric_limits<int>::max() / grid.rows
        || grid.cell_width > std::numeric_limits<int>::max() / grid.columns || grid.cell_height > std::numeric_limits<int>::max() / grid.rows) {
        throw std::invalid_argument("Invalid renderer frame grid");
    }

    if (!grids_.emplace(grid.image, Grid{grid.columns, grid.rows, grid.cell_width, grid.cell_height}).second) {
        throw std::invalid_argument("Duplicate renderer frame grid");
    }
}

contracts::ImageRect FrameCatalog::frame_source(std::string_view image, int frame) const {
    const auto iterator = grids_.find(image);

    if (iterator == grids_.end()) {
        throw std::out_of_range("Unknown renderer frame grid: " + std::string(image));
    }

    if (frame < 0) {
        throw std::out_of_range("Negative renderer frame");
    }

    const auto& grid = iterator->second;
    // Stored cells wrap; the active animation range belongs to the game object.
    const auto index = frame % (grid.columns * grid.rows);

    return {index % grid.columns * grid.cell_width, index / grid.columns * grid.cell_height, grid.cell_width, grid.cell_height};
}

} // namespace moorhuhn2::render
