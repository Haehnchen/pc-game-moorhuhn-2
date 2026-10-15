#include "world/stone.hpp"

#include "world/numeric.hpp"

#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

} // namespace

void update_stone(Stone& stone, float delta, float camera, int recoil) {
    if (!detail::valid_update(delta, camera, recoil) || (stone.type != 10 && stone.type != 50)) {
        throw std::invalid_argument("Invalid stone update state, time, camera or recoil");
    }

    const long double elapsed = delta;
    const auto offset = detail::camera_offset(camera, 1.6F);

    if (stone.type == 50) {
        const int direction = stone.velocity_x > 0 ? 1 : -1;
        stone.velocity_x = stored(stone.velocity_x + direction * elapsed * 0.75F);
        stone.velocity_y = stored(stone.velocity_y + elapsed * 3.0F);
        // MH2 adds the stored velocities once per update, without another delta.
        stone.world_x = stored(static_cast<long double>(stone.world_x) + stone.velocity_x);
        stone.world_y = stored(static_cast<long double>(stone.world_y) + stone.velocity_y);
    }

    stone.screen_x = stored(static_cast<long double>(stone.world_x) - offset);
    stone.screen_y = stored(static_cast<long double>(stone.world_y) + recoil);

    if (stone.type == 50) {
        // The displayed y belongs to the unclamped position for this update.
        if (stone.world_y > 480.0F) {
            stone.world_y = 500.0F;
        }

        const long double animation = stone.animation_time + elapsed * 4.0F * 5.0F;
        stone.animation_time = stored(animation);
        // FST stores binary32 but _ftol consumes the retained x87 expression.
        stone.frame = static_cast<int>(animation);

        if (stone.frame >= 21) {
            stone.frame = 0;
            stone.animation_time = 0;
        }
    }
}

void hit_stone(Stone& stone, std::uint32_t random_x, std::uint32_t random_y) {
    if (stone.type != 10 || random_x > 32767 || random_y > 32767) {
        throw std::invalid_argument("Stone impact requires a live stone and two CRT random draws");
    }

    stone.type = 50;
    stone.frame = 0;
    stone.velocity_x = stored(static_cast<long double>(static_cast<int>(random_x % 20) - 10) * 0.1F);
    stone.velocity_y = stored(static_cast<long double>(-5 - static_cast<int>(random_y % 20)) * 0.1F);
}

contracts::SpriteDraw stone_draw(const Stone& stone) {
    if ((stone.type != 10 && stone.type != 50) || stone.frame < 0 || stone.frame >= 21) {
        throw std::invalid_argument("Cannot draw an inactive or unsupported stone frame");
    }

    return {"gfx/stone", {0, stone.frame * 110, 146, 110}, {static_cast<int>(stone.screen_x), static_cast<int>(stone.screen_y)}, true};
}

} // namespace moorhuhn2::world
