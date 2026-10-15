#include "world/blimp.hpp"

#include "contracts/random.hpp"
#include "world/numeric.hpp"

#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

bool valid(const Blimp& blimp) {
    return (blimp.type == -1 || blimp.type == 24 || blimp.type == 25 || blimp.type == 64 || blimp.type == 65) && blimp.depth == 4 && blimp.frame >= 0 && blimp.frame < 4
           && blimp.frame_count == 1 && blimp.animation_rate == 5 && blimp.points == -35 && detail::strictly_in_int_range(blimp.world_x)
           && detail::strictly_in_int_range(blimp.world_y) && detail::strictly_in_int_range(blimp.screen_x) && detail::strictly_in_int_range(blimp.screen_y)
           && std::isfinite(blimp.animation_time) && blimp.animation_time >= 0 && std::isfinite(blimp.hit_offset_x) && std::isfinite(blimp.hit_offset_y);
}

} // namespace

std::optional<Blimp> initial_blimp(float camera, BlimpProgress& progress, const std::function<std::uint32_t()>& random) {
    if (!detail::valid_camera(camera)) {
        throw std::invalid_argument("Invalid blimp allocation camera");
    }

    if (progress.active) {
        return std::nullopt;
    }

    Blimp blimp;

    if (contracts::draw_random(random) % 2 == 0) {
        blimp.type = 24;
        const int offset = detail::camera_offset(camera, 1.2F);
        blimp.world_x = static_cast<float>(offset - static_cast<int>(contracts::draw_random(random) % 32) - 150);
    } else {
        blimp.type = 25;
        blimp.frame = 1;
        const int offset = static_cast<int>(static_cast<long double>(camera) * -1.2F);
        blimp.world_x = static_cast<float>(670 - offset);
    }

    if (progress.alternate) {
        blimp.frame += 2;
    }

    blimp.world_y = static_cast<float>(60 - contracts::draw_random(random) % 60);
    blimp.hit_offset_x = static_cast<float>(contracts::draw_random(random) % 16);
    progress.active = true;

    return blimp;
}

bool update_blimp(Blimp& blimp, float delta, float camera, int recoil, BlimpProgress& progress) {
    if (!valid(blimp) || blimp.type == -1 || !detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid blimp update state, time, camera or recoil");
    }

    auto next = blimp;
    const bool rightward = next.type == 24 || next.type == 64;
    const bool falling = next.type == 64 || next.type == 65;
    const int camera_offset = detail::camera_offset(camera, 1.2F);
    const long double elapsed = delta;
    const long double world_x = static_cast<long double>(next.world_x) + elapsed * (rightward ? 15.0F : -15.0F);
    next.world_x = stored(world_x);
    // Live motion retains the x87 sum for screen x; falling motion reloads the
    // binary32 world field. This can differ by one ULP near the camera offset.
    next.screen_x = stored((falling ? static_cast<long double>(next.world_x) : world_x) - camera_offset);

    if (falling) {
        next.world_y = stored(static_cast<long double>(next.world_y) + elapsed * 10.0F);
    }

    next.screen_y = stored(static_cast<long double>(next.world_y) + static_cast<long double>(recoil) * 0.6F);
    const bool removed = (rightward ? next.world_x > 2400 : next.world_x < -120) || (falling && next.world_y > 250);

    if (removed) {
        next.type = -1;
    }

    bool smoke{};

    if (falling) {
        const long double time = static_cast<long double>(next.animation_time) + elapsed * 5.0F;
        next.animation_time = stored(time);
        smoke = time > 1;

        if (smoke) {
            // Type 64 reloads after its FST; type 65 subtracts from the retained
            // sum. Neither branch catches up multiple emission intervals.
            next.animation_time = stored((rightward ? static_cast<long double>(next.animation_time) : time) - 1.0F);
        }
    }

    if (!valid(next)) {
        throw std::invalid_argument("Blimp update exceeds original finite field range");
    }

    blimp = next;

    if (removed) {
        progress.active = false;
    }

    return smoke;
}

BlimpImpact hit_blimp(Blimp& blimp, contracts::ScreenPoint pointer, BlimpProgress& progress) {
    if (!valid(blimp) || (blimp.type != 24 && blimp.type != 25) || pointer.x < 0 || pointer.x > 639 || pointer.y < 0 || pointer.y > 479) {
        throw std::invalid_argument("Blimp impact requires a live blimp and a logical pointer");
    }

    const BlimpImpact impact{blimp.points, {stored(static_cast<long double>(blimp.world_x) + 30), stored(static_cast<long double>(blimp.world_y) + 30), blimp.points}};
    blimp.type += 40;
    blimp.animation_time = 0;
    blimp.hit_offset_x = stored(static_cast<long double>(pointer.x) + 16 - blimp.screen_x);
    blimp.hit_offset_y = stored(static_cast<long double>(pointer.y) + 16 - blimp.screen_y);
    progress.hits = std::bit_cast<int>(std::bit_cast<std::uint32_t>(progress.hits) + 1U);

    return impact;
}

bool update_blimp_progress(BlimpProgress& progress) {
    if (progress.hits != 2 || progress.alternate) {
        return false;
    }

    progress.alternate = true;

    return true;
}

std::optional<contracts::SpriteDraw> blimp_draw(const Blimp& blimp) {
    if (!valid(blimp)) {
        throw std::invalid_argument("Invalid blimp draw state");
    }

    if (blimp.type == -1) {
        return std::nullopt;
    }

    return contracts::SpriteDraw{"gfx/blimp", {0, blimp.frame * 60, 120, 60}, {static_cast<int>(blimp.screen_x), static_cast<int>(blimp.screen_y)}, true};
}

std::optional<contracts::SpriteDraw> blimp_collision(const Blimp& blimp) {
    if (blimp.type != 24 && blimp.type != 25) {
        if (!valid(blimp)) {
            throw std::invalid_argument("Invalid blimp collision state");
        }

        return std::nullopt;
    }

    return blimp_draw(blimp);
}

} // namespace moorhuhn2::world
