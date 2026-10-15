#include "world/hunting_stand.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace moorhuhn2::world {
namespace {

bool live(int type) {
    return type >= 13 && type <= 15;
}

bool valid_state(const HuntingStand& stand) {
    const bool first = stand.type == 13 || stand.type == 53;

    return (live(stand.type) || (stand.type >= 53 && stand.type <= 55)) && std::isfinite(stand.world_x) && std::isfinite(stand.world_y) && std::isfinite(stand.screen_x)
           && std::isfinite(stand.screen_y) && std::isfinite(stand.animation_time) && stand.animation_time >= 0 && stand.frame_count == (first ? 2 : 10) && stand.frame >= 0
           && stand.frame < stand.frame_count && stand.ladder_special_hits >= 0 && stand.ladder_special_hits <= 2 && stand.stand_special_hits >= 0 && stand.stand_special_hits <= 3;
}

void validate_pose(const HuntingStand& stand) {
    if (!valid_state(stand) || !detail::in_int_range(stand.screen_x) || !detail::in_int_range(stand.screen_y)) {
        throw std::invalid_argument("Invalid hunting stand drawing state");
    }
}

} // namespace

HuntingStand initial_hunting_stand() {
    return {};
}

void update_hunting_stand(HuntingStand& stand, float delta, float camera, int recoil) {
    if (!valid_state(stand) || !detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid hunting stand state, time, camera or recoil");
    }

    HuntingStand next = stand;
    const auto offset = detail::camera_offset(camera, 1.4F);
    next.screen_x = static_cast<float>(static_cast<long double>(stand.world_x) - offset);
    next.screen_y = static_cast<float>(static_cast<long double>(stand.world_y) + recoil * static_cast<long double>(0.8F));

    if (!live(stand.type)) {
        const long double animation = static_cast<long double>(stand.animation_time) + static_cast<long double>(delta) * 50.0F;
        next.animation_time = static_cast<float>(animation);
        // FST stores binary32, while FCOM and FSUB retain the x87 sum. Unlike
        // generic sprite animation, this branch performs no _ftol on the clock.
        if (animation >= 6.0F) {
            next.animation_time = static_cast<float>(animation - 6.0F);
            ++next.frame;

            if (next.frame >= next.frame_count) {
                if (stand.type == 55) {
                    next.frame = next.frame_count - 1;
                } else {
                    next.type = stand.type == 53 ? 14 : 15;
                    next.frame = 0;
                    next.animation_time = 0;
                    next.frame_count = 10;
                }
            }
        }
    }

    if (!valid_state(next)) {
        throw std::invalid_argument("Hunting stand update exceeds finite original storage");
    }

    stand = next;
}

HuntingStandImpact hit_hunting_stand(HuntingStand& stand, bool special_ammo) {
    if (!valid_state(stand) || !live(stand.type)) {
        throw std::invalid_argument("Hunting stand impact requires a live state");
    }

    HuntingStand next = stand;
    HuntingStandImpact impact;
    bool awarded = stand.type == 13;

    if (stand.type == 13) {
        impact.sounds[0] = HuntingStandSound::chicken_shot_far;
    } else {
        impact.sounds[0] = HuntingStandSound::wood_shot;

        if (special_ammo) {
            int& hits = stand.type == 14 ? next.ladder_special_hits : next.stand_special_hits;
            const int threshold = stand.type == 14 ? 2 : 3;

            if (hits >= threshold) {
                throw std::invalid_argument("Hunting stand special-hit count exceeds live source lifetime");
            }

            awarded = ++hits == threshold;

            if (awarded) {
                impact.sounds[1] = stand.type == 14 ? HuntingStandSound::guardhouse_ladder_shot : HuntingStandSound::guardhouse_breaks_down;
            }
        }
    }

    impact.sound_count = awarded && stand.type != 13 ? 2 : 1;

    if (awarded) {
        const HuntingStandPopup popup{static_cast<float>(static_cast<long double>(stand.world_x) + 60 - 20), static_cast<float>(static_cast<long double>(stand.world_y) + 50), 25};

        if (!std::isfinite(popup.world_x) || !std::isfinite(popup.world_y)) {
            throw std::invalid_argument("Hunting stand popup exceeds finite original storage");
        }

        impact.points = 25;
        impact.popup = popup;
        next.type += 40;
        next.frame = 0;
        // The hit branch never resets animation_time. Live stages normally have
        // zero from the preceding transition, but retained source input is kept.
    }

    stand = next;

    return impact;
}

std::array<contracts::SpriteDraw, 2> hunting_stand_draw(const HuntingStand& stand) {
    validate_pose(stand);
    const long double cover_x = static_cast<long double>(stand.screen_x) + 1920.0F;

    if (!detail::in_int_range(cover_x)) {
        throw std::invalid_argument("Hunting stand cover exceeds original integer storage");
    }

    const std::string_view image = stand.type == 13 || stand.type == 53 ? "gfx/hochsitz3" : stand.type == 14 || stand.type == 54 ? "gfx/hochsitz1" : "gfx/hochsitz2";

    return {contracts::SpriteDraw{image, {0, stand.frame * 100, 120, 100}, {static_cast<int>(stand.screen_x), static_cast<int>(stand.screen_y)}, true},
        contracts::SpriteDraw{"gfx/hochsitz4", {0, 0, 120, 100}, {static_cast<int>(cover_x), static_cast<int>(stand.screen_y)}, true}};
}

contracts::SpriteDraw hunting_stand_collision(const HuntingStand& stand) {
    validate_pose(stand);

    if (!live(stand.type)) {
        throw std::invalid_argument("Hunting stand reactions have no collision draw");
    }

    return {"gfx/hochsitzcol", {0, (stand.type - 13) * 100, 120, 100}, {static_cast<int>(stand.screen_x), static_cast<int>(stand.screen_y)}, false};
}

} // namespace moorhuhn2::world
