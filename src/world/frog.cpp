#include "world/frog.hpp"

#include "world/numeric.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

bool valid_state(const Frog& frog) {
    return (frog.type == -1 || frog.type == 11 || frog.type == 51) && std::isfinite(frog.world_x) && std::isfinite(frog.world_y) && std::isfinite(frog.screen_x)
           && std::isfinite(frog.screen_y) && std::isfinite(frog.animation_time) && frog.animation_time >= 0 && (frog.direction == 1 || frog.direction == 2)
           && (frog.points == 5 || (frog.type != 11 && frog.points == 150))
           && ((frog.type != 11) || (frog.frame >= 0 && frog.frame <= 5 && frog.animation_rate == 3 && frog.frame_count == 0))
           && ((frog.type != 51) || (frog.frame >= 12 && frog.frame < 18 && frog.animation_rate == 4 && frog.frame_count == 18));
}

} // namespace

FrogUpdate update_frog(Frog& frog, float delta, float camera, int recoil, std::optional<std::uint32_t> random) {
    if (!valid_state(frog) || !detail::valid_update(delta, camera, recoil) || (frog.type == 11) != random.has_value() || (random && *random > 32767)) {
        throw std::invalid_argument("Invalid frog update state, time, camera, recoil or CRT draw");
    }

    FrogUpdate result;

    if (frog.type == -1) {
        return result;
    }

    const long double elapsed = delta;
    const auto offset = detail::camera_offset(camera, 1.6F);

    if (frog.type == 11) {
        const long double animation = frog.animation_time + elapsed * 50.0F;
        frog.animation_time = stored(animation);

        if (animation >= 4.0F) {
            // MH2 advances only one cell even when this update spans many cells.
            frog.animation_time = stored(animation - 4.0F);
            frog.frame += frog.direction == 1 ? 1 : -1;

            if (frog.frame == 5) {
                frog.direction = 2;
            } else if (frog.frame == 0) {
                frog.direction = 1;
            }
        }

        if (*random % 512 == 3) {
            const float pan = stored((static_cast<long double>(frog.screen_x) + 60 - 320.0F) * 0.0015625F);
            result.croak_pan = std::clamp(pan, -1.0F, 1.0F);
        }
    } else {
        const long double animation = frog.animation_time + static_cast<long double>(frog.animation_rate) * elapsed * 5.0F;

        if (animation >= std::numeric_limits<int>::max()) {
            throw std::invalid_argument("Frog death animation exceeds the original signed frame range");
        }

        frog.world_x = stored(static_cast<long double>(frog.world_x) + elapsed * 100.0F);
        frog.world_y = stored(frog.world_y - elapsed * 100.0F);
        frog.animation_time = stored(animation);
        frog.frame = static_cast<int>(animation);

        if (frog.frame >= frog.frame_count) {
            frog.type = -1;
        }
    }

    frog.screen_x = stored(static_cast<long double>(frog.world_x) - offset);
    frog.screen_y = stored(static_cast<long double>(frog.world_y) + recoil);

    return result;
}

FrogImpact hit_frog(Frog& frog, bool left_tree_destroyed, bool right_tree_destroyed, float remaining) {
    if (!valid_state(frog) || frog.type != 11 || !std::isfinite(remaining)) {
        throw std::invalid_argument("Frog impact requires a live frog");
    }

    const bool tree_bonus = left_tree_destroyed && right_tree_destroyed && remaining > 80;

    if (tree_bonus) {
        frog.points *= 30;
    }

    const FrogImpact result{frog.points,
        {stored(static_cast<long double>(frog.world_x) + (tree_bonus ? 30 : 50)), stored(static_cast<long double>(frog.world_y) + 40), frog.points}, tree_bonus ? 20 : 0};
    frog.type = 51;
    frog.frame = 12;
    frog.animation_time = 12;
    frog.frame_count = 18;
    frog.animation_rate = 4;

    return result;
}

contracts::SpriteDraw frog_draw(const Frog& frog) {
    if (!valid_state(frog) || frog.type == -1) {
        throw std::invalid_argument("Cannot draw an inactive or unsupported frog frame");
    }

    return {"gfx/frog", {(frog.frame % 6) * 120, (frog.frame / 6) * 80, 120, 80}, {static_cast<int>(frog.screen_x), static_cast<int>(frog.screen_y)}, true};
}

bool apply_frog_time_bonus(int& frog_hits, int stone_hits, float& remaining) {
    if (frog_hits < 0 || stone_hits < 0 || !std::isfinite(remaining)) {
        throw std::invalid_argument("Invalid frog/stone bonus counter or clock");
    }

    if (stone_hits != 4 || frog_hits != 1 || remaining >= 60) {
        return false;
    }

    frog_hits = 2;
    remaining = stored(static_cast<long double>(remaining) + 5.0F);

    return true;
}

} // namespace moorhuhn2::world
