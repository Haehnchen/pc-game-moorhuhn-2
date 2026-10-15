#pragma once

#include "contracts/draw.hpp"
#include "world/score_popup.hpp"

#include <optional>

namespace moorhuhn2::world {

struct Rose {
    int type{29};
    float world_x{497};
    float world_y{396};
    float screen_x{-1024};
    float screen_y{-1024};
    float animation_time{};
    int frame{};
    int frame_count{1};
    float animation_rate{3};
    int points{10};

    bool operator==(const Rose&) const = default;
};

using RosePopup = ScorePopup;

struct RoseImpact {
    int points{};
    RosePopup popup;
    float remaining_after{};

    bool operator==(const RoseImpact&) const = default;
};

// Reaction69 keeps frame0 and falls75 world pixels/second. Its retained screen y,
// including recoil, must exceed 480 before slot32 becomes available for reuse.
void update_rose(Rose& rose, float delta, float camera, int recoil);
// Both ammunition kinds agree. The caller applies score,popup,remaining,accuracy;
// ordinary impacts request no extra sound, debris or random draw.
[[nodiscard]] RoseImpact hit_rose(Rose& rose, float remaining);
[[nodiscard]] std::optional<contracts::SpriteDraw> rose_draw(const Rose& rose);
[[nodiscard]] std::optional<contracts::SpriteDraw> rose_collision(const Rose& rose);

} // namespace moorhuhn2::world
