#pragma once

#include "contracts/draw.hpp"

#include <cstdint>

namespace moorhuhn2::world {

struct Stone {
    int type{10};
    float world_x{};
    float world_y{};
    float screen_x{};
    float screen_y{};
    float velocity_x{};
    float velocity_y{};
    float animation_time{};
    int frame{};

    bool operator==(const Stone&) const = default;
};

// Stones retain their scene slot while dying, including after sinking offscreen.
void update_stone(Stone& stone, float delta, float camera, int recoil);
// The caller supplies the original CRT's two draws, each in the range 0..32767.
void hit_stone(Stone& stone, std::uint32_t random_x, std::uint32_t random_y);
[[nodiscard]] contracts::SpriteDraw stone_draw(const Stone& stone);

} // namespace moorhuhn2::world
