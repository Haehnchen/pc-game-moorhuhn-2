#pragma once

#include <compare>

namespace moorhuhn2::contracts {

inline constexpr int logical_width = 640;
inline constexpr int logical_height = 480;

struct ScreenPoint {
    int x{};
    int y{};

    auto operator<=>(const ScreenPoint&) const = default;
};

} // namespace moorhuhn2::contracts
