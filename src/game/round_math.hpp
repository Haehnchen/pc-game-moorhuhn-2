#pragma once

#include "world/numeric.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace moorhuhn2::game::detail {

inline bool slot_in_range(int slot, int first, std::size_t count) {
    return slot >= first && static_cast<std::size_t>(slot - first) < count;
}

inline constexpr std::array<float, 3> parallax{1.6F, 1.4F, 1.2F};
inline constexpr std::array<float, 3> recoil_scale{1.0F, 0.8F, 0.6F};

// Binary32 stores and integer truncation occur at distinct original instruction
// boundaries; callers retain intermediate expressions until these conversions.
using world::detail::stored;

inline int truncated(long double value) {
    return static_cast<int>(value);
}

inline std::size_t depth_index(int depth) {
    return static_cast<std::size_t>(depth - 2);
}

// The original adds scores with 32-bit wraparound; the next pre-input grants
// pass clamps negative totals to zero.
inline void add_score(int& score, int points) {
    score = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(score) + static_cast<std::uint32_t>(points));
}

} // namespace moorhuhn2::game::detail
