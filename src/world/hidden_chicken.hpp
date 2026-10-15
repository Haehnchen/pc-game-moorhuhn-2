#pragma once

#include "contracts/draw.hpp"
#include "world/score_popup.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace moorhuhn2::world {

struct HiddenChicken {
    int type{34};
    float world_x{1394};
    float world_y{105};
    float screen_x{-1024};
    float screen_y{-1024};
    float animation_time{};
    int mode{};
    float wait{4};
    int frame{};
    int frame_count{16};
    float animation_rate{3};
    int points{250};

    bool operator==(const HiddenChicken&) const = default;
};

// The initial slot33 trigger uses gfx/Leaf cell 0, not gfx/rose.
struct HiddenLeaf {
    int type{35};
    float world_x{2350};
    float world_y{290};
    float screen_x{-1024};
    float screen_y{-1024};
    int frame{};
    int points{50};

    bool operator==(const HiddenLeaf&) const = default;
};

enum class HiddenSound {
    chicken_shot_close,
    chicken_shot_mid,
    chicken_shot_far,
    magic_balloons,
    score_50
};

using HiddenPopup = ScorePopup;

struct HiddenImpact {
    int points{};
    std::optional<HiddenPopup> popup;
    std::array<HiddenSound, 4> sounds{};
    std::size_t sound_count{};
    int particle_count{};
    int balloon_count{};

    bool operator==(const HiddenImpact&) const = default;
};

// Call before consuming RNG. The decision samples the preceding screen_x.
// Only mode0 emergence and mode3 completion consume one original CRT draw.
[[nodiscard]] bool hidden_chicken_needs_random(const HiddenChicken& chicken, float delta, int pointer_x, bool enabled);
void update_hidden_chicken(HiddenChicken& chicken, float delta, float camera, int recoil, int pointer_x, bool enabled, std::optional<std::uint32_t> random = std::nullopt);
void update_hidden_leaf(HiddenLeaf& leaf, float camera, int recoil);

// Both ammunition kinds agree. The caller credits one accuracy hit even when
// the disabled chicken produces no effect. Type35 itself never increments leaves.
[[nodiscard]] HiddenImpact hit_hidden_leaf(HiddenLeaf& leaf, bool& enabled, int leaf_hits, int spider_hits, int branch_hits);
// Enabled impact order: four sounds,32 generic particles,24 balloons,score,popup,
// then reaction74. The caller owns the first-free pools and original RNG order.
[[nodiscard]] HiddenImpact hit_hidden_chicken(HiddenChicken& chicken, bool enabled);

[[nodiscard]] std::optional<contracts::SpriteDraw> hidden_chicken_draw(const HiddenChicken& chicken);
// Mode0 returns an engaged empty-image61x117 scratch: black can select it.
// Mode3 crops the hide2 sheet to the unchanged61x117 collision rectangle.
[[nodiscard]] std::optional<contracts::SpriteDraw> hidden_chicken_collision(const HiddenChicken& chicken);
[[nodiscard]] std::optional<contracts::SpriteDraw> hidden_leaf_draw(const HiddenLeaf& leaf);
[[nodiscard]] std::optional<contracts::SpriteDraw> hidden_leaf_collision(const HiddenLeaf& leaf);

} // namespace moorhuhn2::world
