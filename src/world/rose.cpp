#include "world/rose.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

bool valid(const Rose& rose) {
    return (rose.type == -1 || rose.type == 29 || rose.type == 69) && rose.world_x == 497 && detail::strictly_in_int_range(rose.world_y) && rose.world_y >= 396
           && (rose.type != 29 || (rose.world_y == 396 && rose.points == 10)) && detail::strictly_in_int_range(rose.screen_x) && detail::strictly_in_int_range(rose.screen_y)
           && std::isfinite(rose.animation_time) && rose.animation_time >= 0 && rose.frame == 0 && rose.frame_count == 1 && rose.animation_rate == 3
           && (rose.points == 10 || rose.points == 50);
}

void check(const Rose& rose) {
    if (!valid(rose)) {
        throw std::invalid_argument("Invalid rose state");
    }
}

} // namespace

void update_rose(Rose& rose, float delta, float camera, int recoil) {
    check(rose);

    if (!detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid rose elapsed time, camera or recoil");
    }

    if (rose.type == -1) {
        return;
    }

    auto next = rose;

    if (rose.type == 69) {
        next.world_y = static_cast<float>(static_cast<long double>(rose.world_y) + static_cast<long double>(delta) * 75.0F);
    }

    next.screen_x = static_cast<float>(static_cast<long double>(rose.world_x) - detail::camera_offset(camera, 1.8F));
    const long double screen_y = static_cast<long double>(next.world_y) + static_cast<long double>(recoil) * 1.2F;
    next.screen_y = static_cast<float>(screen_y);
    // FST stores binary32 but keeps the extended sum for the strict comparison.
    if (rose.type == 69 && screen_y > 480.0F) {
        next.type = -1;
    }

    if (!valid(next)) {
        throw std::invalid_argument("Rose update exceeds the original finite coordinate range");
    }

    rose = next;
}

RoseImpact hit_rose(Rose& rose, float remaining) {
    check(rose);

    if (rose.type != 29 || !std::isfinite(remaining)) {
        throw std::invalid_argument("Rose impact requires a live target and finite remaining time");
    }

    float remaining_after = remaining;

    if (remaining >= 85.0F && remaining <= 90.0F) {
        remaining_after = static_cast<float>(static_cast<long double>(remaining) + 5.0F);
    } else if (remaining >= 0.0F && remaining <= 5.0F) {
        rose.points *= 5;
    }

    const RoseImpact result{rose.points, {486, 409, rose.points}, remaining_after};
    rose.type = 69;
    rose.animation_time = 0;

    return result;
}

std::optional<contracts::SpriteDraw> rose_draw(const Rose& rose) {
    check(rose);

    if (rose.type == -1) {
        return std::nullopt;
    }

    return contracts::SpriteDraw{"gfx/rose", {0, 0, 18, 26}, {static_cast<int>(rose.screen_x), static_cast<int>(rose.screen_y)}, true};
}

std::optional<contracts::SpriteDraw> rose_collision(const Rose& rose) {
    check(rose);

    if (rose.type != 29) {
        return std::nullopt;
    }

    return rose_draw(rose);
}

} // namespace moorhuhn2::world
