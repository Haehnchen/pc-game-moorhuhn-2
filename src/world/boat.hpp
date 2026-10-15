#pragma once

#include "contracts/draw.hpp"

namespace moorhuhn2::world {

struct Boat {
    int special_hits{};
    bool sinking{};
    float animation_time{};
    int frame{};

    bool operator==(const Boat&) const = default;
};

void update_boat(Boat& boat, float delta);
[[nodiscard]] bool hit_boat(Boat& boat, bool special_ammo);

// Normal ammunition leaves the live boat at cell zero, including after a hit.
[[nodiscard]] contracts::SpriteDraw boat_draw(float camera, int recoil);
[[nodiscard]] contracts::SpriteDraw boat_draw(const Boat& boat, float camera, int recoil);
[[nodiscard]] contracts::SpriteDraw boat_collision(float camera, int recoil);

} // namespace moorhuhn2::world
