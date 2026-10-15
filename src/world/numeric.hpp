#pragma once

#include "contracts/time.hpp"

#include <cmath>
#include <limits>

namespace moorhuhn2::world::detail {

inline float stored(long double value) {
    return static_cast<float>(value);
}

// Inclusive checks for retained arithmetic; stored poses in some source records
// exclude the two endpoints. Keep that distinction explicit at call sites.
[[nodiscard]] inline bool in_int_range(long double value) {
    return std::isfinite(value) && value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}

[[nodiscard]] inline bool strictly_in_int_range(long double value) {
    return std::isfinite(value) && value > std::numeric_limits<int>::min() && value < std::numeric_limits<int>::max();
}

[[nodiscard]] inline bool valid_camera(float camera) {
    return std::isfinite(camera) && camera >= 0 && camera <= 1280;
}

// The x87 product reaches _ftol without a binary32 store.
[[nodiscard]] inline int camera_offset(float camera, float factor) {
    return static_cast<int>(static_cast<long double>(camera) * factor);
}

[[nodiscard]] inline bool valid_update(float delta, float camera, int recoil) {
    return contracts::valid_delta(delta) && valid_camera(camera) && recoil >= 0 && recoil <= 8;
}

} // namespace moorhuhn2::world::detail
