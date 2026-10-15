#pragma once

#include "contracts/draw.hpp"

namespace moorhuhn2::ui {

[[nodiscard]] constexpr contracts::SpriteDraw tile_draw(std::string_view image, int frame, contracts::ScreenPoint position) {
    return {image, {frame % 32 * 20, frame / 32 * 20, 20, 20}, position};
}

} // namespace moorhuhn2::ui
