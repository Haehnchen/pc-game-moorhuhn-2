#include "world/boat.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {

void update_boat(Boat& boat, float delta) {
    if (!std::isfinite(delta) || delta < 0 || !std::isfinite(boat.animation_time) || boat.animation_time < 0) {
        throw std::invalid_argument("Invalid boat animation or delta");
    }

    if (!boat.sinking) {
        return;
    }

    const long double animation = (2.0L * delta) * 5.0F + boat.animation_time;
    boat.animation_time = static_cast<float>(animation);
    // FST stores binary32; _ftol consumes the retained x87 value before clamp.
    boat.frame = animation >= 11.0L ? 10 : static_cast<int>(animation);
}

bool hit_boat(Boat& boat, bool special_ammo) {
    if (boat.sinking || !special_ammo) {
        return false;
    }

    if (boat.special_hits < 0 || boat.special_hits == std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Invalid boat special-hit count");
    }

    ++boat.special_hits;

    if (boat.special_hits != 3) {
        return false;
    }

    boat.sinking = true;

    return true;
}

contracts::SpriteDraw boat_draw(float camera, int recoil) {
    if (!std::isfinite(camera) || camera < 0 || camera > 1280 || recoil < 0 || recoil > 8) {
        throw std::invalid_argument("Invalid boat camera or recoil");
    }

    const auto offset = detail::camera_offset(camera, 1.4F);
    const auto y = static_cast<float>(264.0L + static_cast<long double>(recoil) * 0.8F);

    return {"gfx/boat", {0, 0, 144, 79}, {626 - offset, static_cast<int>(y)}, true};
}

contracts::SpriteDraw boat_draw(const Boat& boat, float camera, int recoil) {
    if (boat.frame < 0 || boat.frame > 10) {
        throw std::invalid_argument("Invalid boat frame");
    }

    auto draw = boat_draw(camera, recoil);
    draw.source.y = boat.frame * 79;

    return draw;
}

contracts::SpriteDraw boat_collision(float camera, int recoil) {
    auto draw = boat_draw(camera, recoil);
    draw.image = "gfx/boatcol";
    // Its original black key leaves black scratch pixels. Copying those zeros
    // gives the same full-word comparison; this image is never drawn on screen.
    draw.color_key = false;

    return draw;
}

} // namespace moorhuhn2::world
