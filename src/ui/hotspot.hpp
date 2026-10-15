#pragma once

#include "contracts/display.hpp"

namespace moorhuhn2::ui {

struct Hotspot {
    int left, top, right, bottom;

    [[nodiscard]] constexpr bool contains(contracts::ScreenPoint point) const {
        return point.x >= left && point.x < right && point.y >= top && point.y < bottom;
    }
};

} // namespace moorhuhn2::ui
