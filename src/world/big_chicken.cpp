#include "world/big_chicken.hpp"

#include "world/numeric.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

bool valid(const BigChicken& chicken) {
    if ((chicken.type != -1 && chicken.type != 4 && chicken.type != 44) || !std::isfinite(chicken.world_x) || chicken.world_x < 0 || chicken.world_x > 2271
        || std::trunc(chicken.world_x) != chicken.world_x || chicken.world_y != 100 || !detail::strictly_in_int_range(chicken.screen_x)
        || !detail::strictly_in_int_range(chicken.screen_y) || !std::isfinite(chicken.animation_time) || chicken.animation_time < 0 || chicken.animation_rate != 5
        || chicken.points != 25) {
        return false;
    }

    if (chicken.type == 4) {
        return chicken.frame_count == 15 && chicken.frame >= 0 && chicken.frame < 12;
    }

    if (chicken.type == 44) {
        return chicken.frame_count == 16 && chicken.frame >= 0 && chicken.frame < 16;
    }

    return (chicken.frame_count == 15 && chicken.frame >= -1 && chicken.frame < 12) || (chicken.frame_count == 16 && chicken.frame >= 0);
}

void check_camera(float camera, int recoil) {
    if (!std::isfinite(camera) || camera < 0 || camera > 1280 || recoil < 0 || recoil > 8) {
        throw std::invalid_argument("Invalid big chicken camera or recoil");
    }
}

std::optional<contracts::SpriteDraw> draw(const BigChicken& chicken, bool collision) {
    if (!valid(chicken)) {
        throw std::invalid_argument("Invalid big chicken draw state");
    }

    if (chicken.type == -1 || (collision && chicken.type != 4)) {
        return std::nullopt;
    }

    const bool dead = chicken.type == 44;

    return contracts::SpriteDraw{
        dead ? "gfx/big_hit" : "gfx/big_appear", {0, chicken.frame * 384, dead ? 330 : 310, 384}, {static_cast<int>(chicken.screen_x), static_cast<int>(chicken.screen_y)}, true};
}

} // namespace

BigChicken initial_big_chicken(std::uint32_t random, float camera) {
    check_camera(camera, 0);

    if (random > 32767) {
        throw std::invalid_argument("Big chicken initialization requires one original CRT draw");
    }

    BigChicken chicken;
    chicken.type = 4;
    chicken.world_x = static_cast<float>(random % 2272);
    chicken.screen_x = stored(static_cast<long double>(chicken.world_x) - detail::camera_offset(camera, 1.6F));

    return chicken;
}

float big_chicken_spawn_pan(const BigChicken& chicken) {
    if (!valid(chicken) || chicken.type != 4) {
        throw std::invalid_argument("Big chicken spawn pan requires a live chicken");
    }

    // The original stores this intermediate before comparing against +/-1.
    const float pan = stored((static_cast<long double>(chicken.screen_x) + 155 - 320) * 0.0015625F);

    return std::clamp(pan, -1.0F, 1.0F);
}

void update_big_chicken(BigChicken& chicken, float delta, float camera, int recoil) {
    check_camera(camera, recoil);

    if (!valid(chicken) || !std::isfinite(delta) || delta < 0) {
        throw std::invalid_argument("Invalid big chicken state or elapsed time");
    }

    if (chicken.type == -1) {
        return;
    }

    auto next = chicken;
    const long double elapsed = chicken.type == 4 ? static_cast<long double>(delta) * 20.0F + chicken.animation_time
                                                  : static_cast<long double>(chicken.animation_rate) * delta * 5.0F + chicken.animation_time;
    next.animation_time = stored(elapsed);
    // FST stores binary32, but _ftol consumes the still-retained x87 operand.
    if (chicken.type == 4) {
        if (elapsed < 11) {
            next.frame = static_cast<int>(elapsed);
        } else if (elapsed < 51) {
            next.frame = 11;
        } else if (elapsed < 62) {
            next.frame = 61 - static_cast<int>(elapsed);
        } else {
            // The table has ten -1 sentinels. Clamp larger valid times at the
            // first sentinel instead of reading unrelated original memory.
            next.frame = -1;
            next.type = -1;
        }
    } else {
        if (elapsed > std::numeric_limits<int>::max()) {
            throw std::invalid_argument("Big chicken death exceeds the original frame range");
        }

        next.frame = static_cast<int>(elapsed);

        if (next.frame >= next.frame_count) {
            next.type = -1;
        }
    }

    const auto camera_offset = detail::camera_offset(camera, 1.6F);
    next.screen_x = stored(static_cast<long double>(next.world_x) - camera_offset);
    next.screen_y = stored(static_cast<long double>(next.world_y) + recoil);

    if (!valid(next)) {
        throw std::invalid_argument("Big chicken update exceeds finite original storage");
    }

    chicken = next;
}

BigChickenImpact hit_big_chicken(BigChicken& chicken) {
    if (!valid(chicken) || chicken.type != 4) {
        throw std::invalid_argument("Big chicken impact requires a live chicken");
    }

    const BigChickenImpact impact{25, {stored(static_cast<long double>(chicken.world_x) + 135), 292, 25}};
    chicken.type = 44;
    chicken.frame = 0;
    chicken.frame_count = 16;
    chicken.animation_time = 0;

    return impact;
}

std::optional<contracts::SpriteDraw> big_chicken_draw(const BigChicken& chicken) {
    return draw(chicken, false);
}

std::optional<contracts::SpriteDraw> big_chicken_collision(const BigChicken& chicken) {
    return draw(chicken, true);
}

} // namespace moorhuhn2::world
