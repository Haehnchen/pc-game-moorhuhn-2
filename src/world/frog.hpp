#pragma once

#include "contracts/draw.hpp"
#include "world/score_popup.hpp"

#include <cstdint>
#include <optional>

namespace moorhuhn2::world {

struct Frog {
    int type{11};
    float world_x{440};
    float world_y{330};
    float screen_x{-1024};
    float screen_y{-1024};
    float animation_time{};
    float animation_rate{3};
    int frame_count{};
    int frame{};
    int direction{1};
    int points{5};

    bool operator==(const Frog&) const = default;
};

struct FrogUpdate {
    // The original croak uses the preceding update's screen position.
    std::optional<float> croak_pan;

    bool operator==(const FrogUpdate&) const = default;
};

using FrogPopup = ScorePopup;

struct FrogImpact {
    int points{};
    FrogPopup popup;
    int balloon_count{};

    bool operator==(const FrogImpact&) const = default;
};

// Every live update consumes exactly one original CRT draw, including delta zero
// and offscreen updates. Dying/inactive updates require no random draw.
[[nodiscard]] FrogUpdate update_frog(Frog& frog, float delta, float camera, int recoil, std::optional<std::uint32_t> random = std::nullopt);
// The caller credits accuracy, increments the shared frog counter, applies the
// score/popup and plays sfx_frog_shot. Both destroyed trees and remaining > 80
// request twenty balloons and the magic-balloon sound first. Both ammo types agree.
[[nodiscard]] FrogImpact hit_frog(Frog& frog, bool left_tree_destroyed = false, bool right_tree_destroyed = false, float remaining = 0);
[[nodiscard]] contracts::SpriteDraw frog_draw(const Frog& frog);

// Run after clock/object/particle updates and before composition and firing.
// A true result requests sfx_50_point_score; displayed clock fields are unchanged.
[[nodiscard]] bool apply_frog_time_bonus(int& frog_hits, int stone_hits, float& remaining);

} // namespace moorhuhn2::world
