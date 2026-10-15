#pragma once

#include "contracts/draw.hpp"
#include "world/score_popup.hpp"

#include <optional>

namespace moorhuhn2::world {

struct SpecialChicken {
    int type{-1};
    int depth{2};
    float world_x{1447};
    float world_y{72};
    float screen_x{-1024};
    float screen_y{-1024};
    float animation_time{};
    int frame{4};
    int frame_count{5};
    float animation_rate{5};
    int points{20};

    bool operator==(const SpecialChicken&) const = default;
};

using SpecialChickenPopup = ScorePopup;

struct SpecialChickenImpact {
    int points{};
    SpecialChickenPopup popup;

    bool operator==(const SpecialChickenImpact&) const = default;
};

// Call only after finding a free scene slot. The both-crowns gate consumes no
// RNG; successful allocation sets the round's one-shot latch until its reset.
[[nodiscard]] std::optional<SpecialChicken> make_special_chicken(bool left_crown_destroyed, bool right_crown_destroyed, bool& spawned);
// Both live 5 and reaction45 use original table samples at 10/second, regardless
// of their retained rate 5/count5 fields. They remove at samples 65 and 5.
void update_special_chicken(SpecialChicken& chicken, float delta, float camera, int recoil);
// Caller applies score,popup,then chickenhit2 sound and one accuracy hit. Both
// ammunition kinds agree; no time,notice,bonus transition or RNG is requested.
[[nodiscard]] SpecialChickenImpact hit_special_chicken(SpecialChicken& chicken);
[[nodiscard]] std::optional<contracts::SpriteDraw> special_chicken_draw(const SpecialChicken& chicken);
[[nodiscard]] std::optional<contracts::SpriteDraw> special_chicken_collision(const SpecialChicken& chicken);

} // namespace moorhuhn2::world
