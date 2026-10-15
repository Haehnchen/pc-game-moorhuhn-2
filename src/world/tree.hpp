#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstdint>

namespace moorhuhn2::world {

struct Tree {
    int type{26};
    float world_x{153};
    float world_y{};
    float screen_x{-1024};
    float screen_y{-1024};
    float velocity_x{};
    float velocity_y{};
    float animation_time{};
    float animation_rate{};
    int frame_count{1};
    int frame{1};
    int points{};

    bool operator==(const Tree&) const = default;
};

struct TreeUpdate {
    bool ground_sound{};

    bool operator==(const TreeUpdate&) const = default;
};

// Original slots15..18: left trunk/crown, right trunk/crown. Only slot 15 has
// initializer screen writes; fresh native storage starts the other three at zero.
[[nodiscard]] std::array<Tree, 4> initial_trees();
[[nodiscard]] TreeUpdate update_tree(Tree& tree, float delta, float camera, int recoil);
// The caller first requests three wood particles and runs the source leaf loop.
// Exactly four branch hits decrements every live piece, including zero-point
// trunks. True requests the ordered destruction effects before start_tree_fall.
[[nodiscard]] bool damage_tree(Tree& tree, int branch_hits);
// After four type 28, sixteen type27, thirty-two wood and one type 30 requests,
// the caller sets the frame0/2 flag and supplies these two CRT draws.
void start_tree_fall(Tree& tree, std::uint32_t random_x, std::uint32_t random_y);
[[nodiscard]] contracts::SpriteDraw tree_draw(const Tree& tree);

struct TreeFragment {
    int type{-1};
    int depth{2};
    float world_x{};
    float world_y{};
    float screen_x{-1024};
    float screen_y{-1024};
    float velocity_x{};
    float velocity_y{};
    float animation_time{};
    float animation_rate{};
    int frame_count{};
    int frame{};
    int points{};

    bool operator==(const TreeFragment&) const = default;
};

struct TreeFragmentUpdate {
    bool ground_sound{};
    bool highfly_sound{};

    bool operator==(const TreeFragmentUpdate&) const = default;
};

struct TreeLeafImpact {
    int points{};
    float popup_x{};
    float popup_y{};

    bool operator==(const TreeLeafImpact&) const = default;
};

// Scene allocation precedes RNG: a full pool consumes no initializer draws.
[[nodiscard]] TreeFragment make_tree_leaf(float camera, int pointer_x, const std::array<std::uint32_t, 6>& random);
[[nodiscard]] TreeFragment make_tree_branch(float camera, int pointer_x, const std::array<std::uint32_t, 5>& random);
[[nodiscard]] TreeFragment make_tree_highfly(float camera, int pointer_x);
// Updates consume no RNG. highfly_sound requests sfx/chickenhit2 once after y>-50.
[[nodiscard]] TreeFragmentUpdate update_tree_fragment(TreeFragment& fragment, float delta, float camera, int recoil);
// Only live type27 leaves qualify. The caller increments the shared leaf/accuracy
// counters and applies this score/popup. Reaction67 exceeds the tracker range.
[[nodiscard]] TreeLeafImpact hit_tree_leaf(TreeFragment& leaf);
[[nodiscard]] contracts::SpriteDraw tree_fragment_draw(const TreeFragment& fragment);

} // namespace moorhuhn2::world
