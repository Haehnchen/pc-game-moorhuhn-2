#include "world/village.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

bool valid_target(VillageTarget target) {
    return target == VillageTarget::first_church || target == VillageTarget::second_church || target == VillageTarget::house;
}

VillagePopup popup(VillageTarget target, int points) {
    const int shift = points < -9 ? -30 : points < 10 ? -10 : -20;

    if (target == VillageTarget::first_church) {
        return {static_cast<float>(750 + shift), 177, points};
    }

    if (target == VillageTarget::second_church) {
        return {static_cast<float>(653 + shift), 182, points};
    }

    return {static_cast<float>(556 + shift), 231, points};
}

} // namespace

VillageImpact hit_village(Village& village, VillageTarget target, int frog_hits) {
    if (!valid_target(target) || frog_hits < 0 || village.house_points < 0 || village.house_points > 25 || village.house_points % 5 != 0) {
        throw std::invalid_argument("Invalid village impact target, state or frog counter");
    }

    VillageImpact impact;
    impact.sound_count = 1;

    switch (target) {
        case VillageTarget::first_church:
            impact.sounds[0] = VillageSound::churchbell;

            if (!village.first_church_hit) {
                village.first_church_hit = true;
                impact.extra_seconds = 10;
                impact.points = -25;
                impact.popup = popup(target, impact.points);
            }

            break;
        case VillageTarget::second_church:
            impact.sounds[0] = VillageSound::churchbell_2;

            if (!village.second_church_hit) {
                village.second_church_hit = true;
                impact.points = 15;
                impact.popup = popup(target, impact.points);
            }

            break;
        case VillageTarget::house:
            impact.sounds[0] = VillageSound::house_shot;

            if (!village.house_hit) {
                village.house_hit = true;
                impact.points = village.house_points;
                impact.popup = popup(target, impact.points);
            } else if (village.first_church_hit && !village.second_church_hit && frog_hits == 0 && village.house_points > 0) {
                village.house_points -= 5;
                impact.points = village.house_points;
                impact.popup = popup(target, impact.points);
                impact.sounds[1] = VillageSound::churchbell;
                impact.sounds[2] = VillageSound::churchbell_2;
                impact.sound_count = 3;
                // The final zero-point award still creates its popup and balloon.
                impact.spawn_balloon = true;
            }

            break;
    }

    return impact;
}

contracts::SpriteDraw village_draw(VillageTarget target, float camera, int recoil) {
    if (!valid_target(target) || !std::isfinite(camera) || camera < 0 || camera > 1280 || recoil < 0 || recoil > 8) {
        throw std::invalid_argument("Invalid village draw target, camera or recoil");
    }

    const int offset = detail::camera_offset(camera, 1.2F);
    const int world_x = target == VillageTarget::first_church ? 740 : target == VillageTarget::second_church ? 643 : 550;
    const int world_y = target == VillageTarget::first_church ? 157 : target == VillageTarget::second_church ? 162 : 221;
    // The original stores binary32 screen y before draw and collision truncate it.
    const float screen_y = static_cast<float>(world_y + static_cast<long double>(recoil) * 0.6F);

    if (target == VillageTarget::house) {
        // The loader wraps original frame 1 to the house sheet's only cell.
        return {"gfx/house", {0, 0, 13, 20}, {world_x - offset, static_cast<int>(screen_y)}, true};
    }

    return {"gfx/church", {0, target == VillageTarget::first_church ? 0 : 40, 20, 40}, {world_x - offset, static_cast<int>(screen_y)}, true};
}

} // namespace moorhuhn2::world
