#pragma once

#include "contracts/draw.hpp"
#include "world/score_popup.hpp"

#include <cstdint>
#include <optional>

namespace moorhuhn2::world {

struct BigChicken {
    int type{-1};
    float world_x{};
    float world_y{100};
    float screen_x{-1024};
    float screen_y{-1024};
    float animation_time{};
    int frame{};
    int frame_count{15};
    float animation_rate{5};
    int points{25};

    bool operator==(const BigChicken&) const = default;
};

using BigChickenPopup = ScorePopup;

struct BigChickenImpact {
    int points{};
    BigChickenPopup popup;

    bool operator==(const BigChickenImpact&) const = default;
};

// Call only after the first-free scene allocation succeeds. Consumes one CRT
// draw (%2272); no prerequisites. The screen y sentinel lasts until update.
[[nodiscard]] BigChicken initial_big_chicken(std::uint32_t random, float camera);
// The spawn's sfx_big_chicken_pops_up channel receives this pan once. Its channel
// is neither panned again nor stopped by the original live/death branches.
[[nodiscard]] float big_chicken_spawn_pan(const BigChicken& chicken);
// Live type4 uses the opening/holding/retreat table at 20 entries/second. Type44
// uses the unchanged world pose and 25 death cells/second, then releases its slot.
void update_big_chicken(BigChicken& chicken, float delta, float camera, int recoil);
// Both ammunition kinds agree. Original order: score,one shared type2 popup,
// reaction44 fields,then sfx/chickenhit2. No debris, RNG or immediate notice.
[[nodiscard]] BigChickenImpact hit_big_chicken(BigChicken& chicken);
[[nodiscard]] std::optional<contracts::SpriteDraw> big_chicken_draw(const BigChicken& chicken);
// Collision keeps the original310x384 live rectangle; reaction44 is ineligible.
[[nodiscard]] std::optional<contracts::SpriteDraw> big_chicken_collision(const BigChicken& chicken);

} // namespace moorhuhn2::world
