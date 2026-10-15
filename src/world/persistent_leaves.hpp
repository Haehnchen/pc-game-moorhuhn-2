#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace moorhuhn2::world {

inline constexpr std::size_t persistent_leaf_count = 32;
using PersistentLeafRandom = std::function<std::uint32_t()>;

struct PersistentLeaf {
    float world_x{700};
    float world_y{240};
    // The initializer does not write screen fields. Normalize fresh storage
    // offscreen; the first hunting update supplies their first visible pose.
    float screen_x{-1024};
    float screen_y{-1024};
    float phase{};
    float amplitude{};
    int depth{3};
    int frame{1};

    bool operator==(const PersistentLeaf&) const = default;
};

// Original particle slots 0..31 consume four consecutive CRT draws each.
[[nodiscard]] PersistentLeaf make_persistent_leaf(const std::array<std::uint32_t, 4>& random);
[[nodiscard]] std::array<PersistentLeaf, persistent_leaf_count> make_persistent_leaves(const PersistentLeafRandom& random);
// Evaluate the old phase, advance it and pose the old origin/amplitude, then
// request one draw. Residue 2 alone requests another draw to replace amplitude.
// Even zero delta consumes RNG. A rejected draw leaves the leaf unchanged.
// Leaves never expire, wrap or recycle.
void update_persistent_leaf(PersistentLeaf& leaf, float delta, float camera, int recoil, const PersistentLeafRandom& random);
[[nodiscard]] contracts::SpriteDraw persistent_leaf_draw(const PersistentLeaf& leaf);

} // namespace moorhuhn2::world
