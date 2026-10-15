#pragma once

#include "contracts/draw.hpp"
#include "world/score_popup.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace moorhuhn2::world {

struct HuntingStand {
    int type{13};
    float world_x{252};
    float world_y{158};
    float screen_x{-1024};
    float screen_y{-1024};
    float animation_time{};
    int frame_count{2};
    int frame{};
    int ladder_special_hits{};
    int stand_special_hits{};

    bool operator==(const HuntingStand&) const = default;
};

enum class HuntingStandSound {
    chicken_shot_far,
    wood_shot,
    guardhouse_ladder_shot,
    guardhouse_breaks_down
};

using HuntingStandPopup = ScorePopup;

struct HuntingStandImpact {
    int points{};
    std::optional<HuntingStandPopup> popup;
    std::array<HuntingStandSound, 2> sounds{};
    std::size_t sound_count{};

    bool operator==(const HuntingStandImpact&) const = default;
};

// Original slot4; no independent record exists for the right-side cover.
[[nodiscard]] HuntingStand initial_hunting_stand();
// Reactions advance at most one frame per consumed update, preserving remainder.
// Neither live nor reaction states consume RNG or release their scene slot.
void update_hunting_stand(HuntingStand& stand, float delta, float camera, int recoil);
// Live 13/14/15 only. The caller credits one accuracy hit and allocates any popup.
// The two counters reproduce the original round globals; ordinary hits skip them.
[[nodiscard]] HuntingStandImpact hit_hunting_stand(HuntingStand& stand, bool special_ammo);
// Submit both draws in this order at depth3/slot4. The cover uses its sole cell.
[[nodiscard]] std::array<contracts::SpriteDraw, 2> hunting_stand_draw(const HuntingStand& stand);
// Live 13/14/15 use fixed mask0/1/2, regardless of their current animation frame.
[[nodiscard]] contracts::SpriteDraw hunting_stand_collision(const HuntingStand& stand);

} // namespace moorhuhn2::world
