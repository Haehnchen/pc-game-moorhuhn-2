#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstdint>

namespace moorhuhn2::world {

struct Branch {
    int type{22};
    float world_x{87};
    float world_y{5};
    float screen_x{-1024};
    float screen_y{-1024};
    float velocity_x{};
    float velocity_y{};
    float animation_time{};
    float animation_rate{3};
    int frame_count{1};
    int frame{};
    int points{};

    bool operator==(const Branch&) const = default;
};

struct BranchUpdate {
    bool ground_sound{};

    bool operator==(const BranchUpdate&) const = default;
};

// Original slots 20/21 are type 22; slots 22/23 are type 23. Partners have no
// initializer screen writes; this factory starts their fresh storage at zero.
[[nodiscard]] std::array<Branch, 4> initial_branches();
// Live, falling and removed branch updates consume no random draws.
[[nodiscard]] BranchUpdate update_branch(Branch& branch, float delta, float camera, int recoil);
// First request ten shared particles, then obtain these two CRT draws. The caller
// increments the shared branch counter, credits accuracy and plays sfx_tree_shot.
// Ordinary and special ammunition use the same zero-score branch.
void hit_branch(Branch& branch, std::uint32_t random_x, std::uint32_t random_y);
[[nodiscard]] contracts::SpriteDraw branch_draw(const Branch& branch);

} // namespace moorhuhn2::world
