#pragma once

#include "contracts/display.hpp"

#include <string_view>

namespace moorhuhn2::contracts {

struct ImageRect {
    int x{};
    int y{};
    int width{};
    int height{};
};

// Source pixels and logical screen coordinates are integers. Commands are drawn
// in submission order; drawing never advances animation or simulation state.
struct SpriteDraw {
    std::string_view image;
    ImageRect source;
    ScreenPoint destination;
    bool color_key{true};
    bool mirror_x{};
};

inline constexpr int scene_pool_capacity = 256;

// Scene-pool order applies within each depth. Particles follow every scene slot.
struct DepthDraw {
    int depth{};
    SpriteDraw sprite;
    int scene_slot{scene_pool_capacity};
};

} // namespace moorhuhn2::contracts
