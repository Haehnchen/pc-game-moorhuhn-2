#pragma once

#include "contracts/draw.hpp"
#include "world/notice.hpp"
#include "world/score_popup.hpp"

#include <optional>

namespace moorhuhn2::world {

struct Spider {
    int type{20};
    float world_x{1224};
    float world_y{-4};
    float screen_x{-1024};
    float screen_y{-1024};
    float animation_time{};
    float animation_rate{3};
    int frame_count{25};
    int frame{8};
    float countdown{5};
    float mode{1};
    int points{50};

    bool operator==(const Spider&) const = default;
};

using SpiderPopup = ScorePopup;

struct SpiderImpact {
    int points{};
    std::optional<SpiderPopup> popup;
    bool swings_sound{};

    bool operator==(const SpiderImpact&) const = default;
};

enum class SpiderBonusKind {
    none,
    first,
    hero
};

struct SpiderBonus {
    SpiderBonusKind kind{};
    int points{};
    int balloon_count{};
    std::optional<Notice> notice;

    bool operator==(const SpiderBonus&) const = default;
};

// Spider initialization, either live phase and either reaction consume no RNG.
void update_spider(Spider& spider, float delta, float camera, int recoil);
// Every selected hit credits accuracy. Only ordinary ammunition increments the
// shared counter and plays swings; special ammunition causes permanent removal.
[[nodiscard]] SpiderImpact hit_spider(Spider& spider, int& spider_hits, bool special_ammunition);
[[nodiscard]] contracts::SpriteDraw spider_draw(const Spider& spider);
// Type 20 always samples spinne1, even when its visible live phase draws spinne2.
[[nodiscard]] contracts::SpriteDraw spider_collision(const Spider& spider);
// Run after the eight-stone grant and before the frog bonus, composition or fire.
// First: allocate 16 balloons, then magic sound. Hero: magic, score sound,
// allocate 20 balloons, allocate notice101, then add 100 regardless of pool space.
[[nodiscard]] SpiderBonus apply_spider_bonus(int& spider_hits, bool& hero_awarded);

} // namespace moorhuhn2::world
