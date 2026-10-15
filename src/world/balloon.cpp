#include "world/balloon.hpp"

#include "world/numeric.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

} // namespace

Balloon make_balloon(float camera, const std::array<std::uint32_t, 5>& random) {
    if (!detail::valid_camera(camera) || std::ranges::any_of(random, [](auto value) {
            return value > 32767;
        })) {
        throw std::invalid_argument("Balloon creation requires a valid camera and five CRT draws");
    }

    Balloon balloon;
    balloon.type = 21;
    balloon.world_x = stored(static_cast<long double>(camera) * 1.4F + random[0] % 640);
    balloon.world_y = 340;
    balloon.frame = static_cast<int>(random[1] % 4);
    balloon.points = balloon.frame + 1;
    balloon.velocity_x = static_cast<float>(static_cast<int>(random[2] % 32) - 8);
    balloon.velocity_y = static_cast<float>(random[3] % 60 + 20);

    if (random[4] % 100 == 0) {
        balloon.frame = 4;
        balloon.points = 100;
    }

    return balloon;
}

void update_balloon(Balloon& balloon, float delta, float camera, int recoil, int drift) {
    if ((balloon.type != 21 && balloon.type != 61) || !detail::valid_update(delta, camera, recoil) || drift < -1 || drift > 1) {
        throw std::invalid_argument("Invalid balloon update state, time, camera, recoil or drift");
    }

    const long double elapsed = delta;
    const auto offset = detail::camera_offset(camera, 1.4F);

    if (balloon.type == 21) {
        balloon.world_x = stored(balloon.world_x - static_cast<long double>(balloon.velocity_x) * elapsed);
        balloon.world_y = stored(balloon.world_y - static_cast<long double>(balloon.velocity_y) * elapsed);
    } else {
        balloon.world_x = stored(static_cast<long double>(balloon.world_x) + drift);
        balloon.world_y = stored(balloon.world_y + elapsed * 25.0F);
    }

    balloon.screen_x = stored(static_cast<long double>(balloon.world_x) - offset);
    balloon.screen_y = stored(static_cast<long double>(balloon.world_y) + static_cast<long double>(recoil) * 0.8F);

    if (balloon.type == 21 && balloon.world_y < -42.0F) {
        balloon.type = -1;
    }
}

void hit_balloon(Balloon& balloon, std::uint32_t random_x, std::uint32_t random_y) {
    if (balloon.type != 21 || balloon.frame < 0 || balloon.frame > 4 || random_x > 32767 || random_y > 32767) {
        throw std::invalid_argument("Balloon impact requires a live balloon and two CRT draws");
    }

    balloon.type = 61;
    balloon.velocity_x = static_cast<float>(random_x % 10);
    balloon.velocity_y = static_cast<float>(random_y % 20);
    balloon.frame += 5;
}

contracts::SpriteDraw balloon_draw(const Balloon& balloon) {
    if ((balloon.type != 21 && balloon.type != 61) || balloon.frame < 0 || balloon.frame >= 10) {
        throw std::invalid_argument("Cannot draw an inactive or unsupported balloon frame");
    }

    return {"gfx/ballon", {(balloon.frame % 5) * 22, (balloon.frame / 5) * 42, 22, 42}, {static_cast<int>(balloon.screen_x), static_cast<int>(balloon.screen_y)}, true};
}

} // namespace moorhuhn2::world
