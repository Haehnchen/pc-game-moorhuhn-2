#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstdint>

namespace moorhuhn2::world {

struct Balloon {
    int type{-1};
    float world_x{};
    float world_y{};
    float screen_x{-1024};
    float screen_y{-1024};
    float velocity_x{};
    float velocity_y{};
    int frame{};
    int points{};

    bool operator==(const Balloon&) const = default;
};

// The allocator supplies five CRT draws only after finding a free scene slot.
[[nodiscard]] Balloon make_balloon(float camera, const std::array<std::uint32_t, 5>& random);
// For type 61 the caller consumes rand()%5, and supplies rand()%3-1 only on zero.
// Dying balloons retain their scene slot and consume those draws even offscreen.
void update_balloon(Balloon& balloon, float delta, float camera, int recoil, int drift = 0);
void hit_balloon(Balloon& balloon, std::uint32_t random_x, std::uint32_t random_y);
[[nodiscard]] contracts::SpriteDraw balloon_draw(const Balloon& balloon);

} // namespace moorhuhn2::world
