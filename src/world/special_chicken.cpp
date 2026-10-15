#include "world/special_chicken.hpp"

#include "world/numeric.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

// The original frame tables end with -1 padding. The initializer's
// frame_count describes neither this scripted emergence nor its death sequence.
constexpr std::array live_frames{4, 3, 2, 1, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 7, 8, 9, 9, 9, 9, 8, 7, 6, 5, 5, 5, 5, 5, 10, 11, 12, 13, 14, 14, 14, 14, 13, 12, 11, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 16, 17, 18, 19, 18, 17, 16, 1, 2, 3, 4};
constexpr std::array death_frames{20, 21, 22, 23, 24};

bool valid(const SpecialChicken& chicken) {
    if ((chicken.type != -1 && chicken.type != 5 && chicken.type != 45) || chicken.depth != 2 || chicken.world_x != 1447 || chicken.world_y != 72
        || !detail::strictly_in_int_range(chicken.screen_x) || !detail::strictly_in_int_range(chicken.screen_y) || !std::isfinite(chicken.animation_time)
        || chicken.animation_time < 0 || chicken.frame_count != 5 || chicken.animation_rate != 5 || chicken.points != 20) {
        return false;
    }

    if (chicken.type == -1) {
        return chicken.frame >= -1 && chicken.frame < 25;
    }

    if (chicken.type == 5) {
        return chicken.frame >= 0 && chicken.frame < 20;
    }

    // Impact resets frame0 before the next update selects death cell 20.
    return chicken.frame == 0 || (chicken.frame >= 20 && chicken.frame < 25);
}

void check(const SpecialChicken& chicken) {
    if (!valid(chicken)) {
        throw std::invalid_argument("Invalid special chicken state");
    }
}

} // namespace

std::optional<SpecialChicken> make_special_chicken(bool left_crown_destroyed, bool right_crown_destroyed, bool& spawned) {
    if (spawned || !left_crown_destroyed || !right_crown_destroyed) {
        return std::nullopt;
    }

    SpecialChicken chicken;
    chicken.type = 5;
    spawned = true;

    return chicken;
}

void update_special_chicken(SpecialChicken& chicken, float delta, float camera, int recoil) {
    check(chicken);

    if (!detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid special chicken time, camera or recoil");
    }

    if (chicken.type == -1) {
        return;
    }

    auto next = chicken;
    const long double animation = static_cast<long double>(delta) * 10.0F + chicken.animation_time;

    if (animation >= std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Special chicken animation exceeds the original signed frame range");
    }

    next.animation_time = static_cast<float>(animation);
    // FST preserves the extended sum for _ftol. Reading back the float store
    // would choose cell 1 prematurely at a single 6.1f-second live update.
    const auto sample = static_cast<std::size_t>(animation);

    if (chicken.type == 5) {
        next.frame = sample < live_frames.size() ? live_frames[sample] : -1;
    } else {
        next.frame = sample < death_frames.size() ? death_frames[sample] : -1;
    }

    if (next.frame == -1) {
        next.type = -1;
    }

    next.screen_x = static_cast<float>(static_cast<long double>(next.world_x) - detail::camera_offset(camera, 1.6F));
    next.screen_y = static_cast<float>(static_cast<long double>(next.world_y) + recoil);
    chicken = next;
}

SpecialChickenImpact hit_special_chicken(SpecialChicken& chicken) {
    check(chicken);

    if (chicken.type != 5) {
        throw std::invalid_argument("Special chicken impact requires a live target");
    }

    const SpecialChickenImpact impact{20, {1527, 138, 20}};
    chicken.type = 45;
    chicken.frame = 0;
    chicken.animation_time = 0;

    return impact;
}

std::optional<contracts::SpriteDraw> special_chicken_draw(const SpecialChicken& chicken) {
    check(chicken);

    if (chicken.type == -1) {
        return std::nullopt;
    }

    return contracts::SpriteDraw{
        "gfx/specialhuhn", {(chicken.frame % 5) * 200, (chicken.frame / 5) * 133, 200, 133}, {static_cast<int>(chicken.screen_x), static_cast<int>(chicken.screen_y)}, true};
}

std::optional<contracts::SpriteDraw> special_chicken_collision(const SpecialChicken& chicken) {
    check(chicken);

    return chicken.type == 5 ? special_chicken_draw(chicken) : std::nullopt;
}

} // namespace moorhuhn2::world
