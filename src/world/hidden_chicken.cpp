#include "world/hidden_chicken.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

bool valid_pose(float x, float y) {
    return std::isfinite(x) && std::isfinite(y) && x > std::numeric_limits<int>::min() && x < std::numeric_limits<int>::max() && y > std::numeric_limits<int>::min()
           && y < std::numeric_limits<int>::max();
}

bool valid(const HiddenChicken& chicken) {
    if ((chicken.type != -1 && chicken.type != 34 && chicken.type != 74) || (chicken.world_x != 1394 && chicken.world_x != 1395) || chicken.world_y != 105
        || !valid_pose(chicken.screen_x, chicken.screen_y) || !std::isfinite(chicken.animation_time) || chicken.animation_time < 0 || !std::isfinite(chicken.wait)
        || chicken.wait < 0 || chicken.mode < 0 || chicken.mode > 3 || chicken.frame_count != 16 || chicken.animation_rate != 3 || chicken.points != 250) {
        return false;
    }

    if (chicken.type == -1) {
        return chicken.frame >= 0;
    }

    if (chicken.type == 74) {
        return chicken.frame >= 0 && chicken.frame < 16;
    }

    switch (chicken.mode) {
        case 0:
            return chicken.frame == 0;
        case 1:
            return chicken.frame >= 0 && chicken.frame < 16;
        case 2:
            return chicken.frame == 15;
        case 3:
            return chicken.frame >= 0 && chicken.frame < 10;
    }

    return false;
}

bool valid(const HiddenLeaf& leaf) {
    return (leaf.type == -1 || leaf.type == 35) && leaf.world_x == 2350 && leaf.world_y == 290 && valid_pose(leaf.screen_x, leaf.screen_y) && leaf.frame == 0 && leaf.points == 50;
}

void check_camera(float camera, int recoil) {
    if (!std::isfinite(camera) || camera < 0 || camera > 1280 || recoil < 0 || recoil > 8) {
        throw std::invalid_argument("Invalid hidden target camera or recoil");
    }
}

void check_update(const HiddenChicken& chicken, float delta, int pointer_x) {
    if (!valid(chicken) || !std::isfinite(delta) || delta < 0 || pointer_x < 0 || pointer_x > 639) {
        throw std::invalid_argument("Invalid hidden chicken state, elapsed time or pointer");
    }
}

bool pointer_inside(const HiddenChicken& chicken, int pointer_x, bool strict) {
    const long long previous_x = static_cast<int>(chicken.screen_x);
    const long long aim_x = pointer_x + 16;

    return strict ? previous_x - 320 < aim_x && aim_x < previous_x + 381 : previous_x - 320 <= aim_x && aim_x <= previous_x + 381;
}

std::optional<contracts::SpriteDraw> chicken_draw(const HiddenChicken& chicken, bool collision) {
    if (!valid(chicken)) {
        throw std::invalid_argument("Invalid hidden chicken draw state");
    }

    if (chicken.type == -1 || (collision && chicken.type != 34) || (!collision && chicken.type == 34 && chicken.mode == 0)) {
        return std::nullopt;
    }

    const contracts::ScreenPoint position{static_cast<int>(chicken.screen_x), static_cast<int>(chicken.screen_y)};

    if (chicken.type == 34 && chicken.mode == 0) {
        return contracts::SpriteDraw{"", {0, 0, 61, 117}, position, false};
    }

    const bool dead = chicken.type == 74;
    const bool retreating = !dead && chicken.mode == 3;
    const int width = dead ? 80 : retreating ? 78 : 61;
    const int height = dead ? 114 : retreating ? 121 : 117;

    return contracts::SpriteDraw{dead         ? "gfx/hide3"
                                 : retreating ? "gfx/hide2"
                                              : "gfx/hide1",
        {0, chicken.frame * height, collision ? 61 : width, collision ? 117 : height}, position, true};
}

bool needs_random(const HiddenChicken& chicken, float delta, int pointer_x, bool enabled) {
    if (chicken.type != 34) {
        return false;
    }

    if (chicken.mode == 0) {
        const long double elapsed = static_cast<long double>(delta) + chicken.animation_time;

        return elapsed > chicken.wait && (enabled || !pointer_inside(chicken, pointer_x, false));
    }

    if (chicken.mode == 3) {
        return static_cast<long double>(delta) * 18.0F + chicken.animation_time >= 10;
    }

    return false;
}

} // namespace

bool hidden_chicken_needs_random(const HiddenChicken& chicken, float delta, int pointer_x, bool enabled) {
    check_update(chicken, delta, pointer_x);

    return needs_random(chicken, delta, pointer_x, enabled);
}

void update_hidden_chicken(HiddenChicken& chicken, float delta, float camera, int recoil, int pointer_x, bool enabled, std::optional<std::uint32_t> random) {
    check_update(chicken, delta, pointer_x);
    check_camera(camera, recoil);

    if (needs_random(chicken, delta, pointer_x, enabled) != random.has_value() || (random && *random > 32767)) {
        throw std::invalid_argument("Hidden chicken update requires exactly the consumed original CRT draw");
    }

    if (chicken.type == -1) {
        return;
    }

    auto next = chicken;

    if (chicken.type == 74) {
        const long double animation = static_cast<long double>(chicken.animation_rate) * delta * 5.0F + chicken.animation_time;

        if (animation >= std::numeric_limits<int>::max()) {
            throw std::invalid_argument("Hidden chicken death exceeds the original signed frame range");
        }

        next.animation_time = stored(animation);
        next.frame = static_cast<int>(animation);

        if (next.frame >= next.frame_count) {
            next.type = -1;
        }
    } else if (chicken.mode == 0) {
        next.animation_time = stored(static_cast<long double>(delta) + chicken.animation_time);

        if (random) {
            next.wait = static_cast<float>(*random % 3 + (enabled ? 2 : 1));
            next.animation_time = 0;
            next.mode = 1;
            next.world_x = 1394;
        }
    } else if (chicken.mode == 1 || chicken.mode == 3) {
        const long double animation = static_cast<long double>(delta) * 18.0F + chicken.animation_time;
        next.animation_time = stored(animation);
        const int terminal = chicken.mode == 1 ? 16 : 10;
        // Captured tables contain sequential cells followed by -1 padding. Clamp
        // oversized updates at the first sentinel instead of reading past them.
        if (animation >= terminal) {
            next.frame = chicken.mode == 1 ? 15 : 0;
            next.mode = chicken.mode == 1 ? 2 : 0;
            next.animation_time = 0;

            if (random) {
                next.wait = static_cast<float>(*random % 16 + 4);
            }
        } else {
            next.frame = static_cast<int>(animation);
        }
    } else if (enabled) {
        const long double animation = static_cast<long double>(delta) + chicken.animation_time;
        next.animation_time = stored(animation);

        if (animation > chicken.wait) {
            next.animation_time = 0;
            next.mode = 3;
            next.frame = 0;
            next.world_x = 1395;
        }
    } else if (pointer_inside(chicken, pointer_x, true)) {
        next.animation_time = 0;
        next.mode = 3;
        next.frame = 0;
        next.world_x = 1395;
    }

    next.screen_x = stored(static_cast<long double>(next.world_x) - detail::camera_offset(camera, 1.6F));
    next.screen_y = stored(static_cast<long double>(next.world_y) + recoil);

    if (!valid(next)) {
        throw std::invalid_argument("Hidden chicken update exceeds the original finite field range");
    }

    chicken = next;
}

void update_hidden_leaf(HiddenLeaf& leaf, float camera, int recoil) {
    if (!valid(leaf)) {
        throw std::invalid_argument("Invalid hidden leaf state");
    }

    check_camera(camera, recoil);

    if (leaf.type == -1) {
        return;
    }

    leaf.screen_x = stored(static_cast<long double>(leaf.world_x) - detail::camera_offset(camera, 1.4F));
    leaf.screen_y = stored(static_cast<long double>(leaf.world_y) + static_cast<long double>(recoil) * 0.8F);
}

HiddenImpact hit_hidden_leaf(HiddenLeaf& leaf, bool& enabled, int leaf_hits, int spider_hits, int branch_hits) {
    if (!valid(leaf) || leaf.type != 35 || leaf_hits < 0 || spider_hits < 0 || branch_hits < 0) {
        throw std::invalid_argument("Hidden leaf impact requires a live target and valid prerequisite counters");
    }

    if (leaf_hits >= 10 && spider_hits == 6 && branch_hits == 0) {
        enabled = true;
    }

    leaf.type = -1;

    return {50, HiddenPopup{2360, 320, 50}, {HiddenSound::score_50}, 1, 0, 0};
}

HiddenImpact hit_hidden_chicken(HiddenChicken& chicken, bool enabled) {
    if (!valid(chicken) || chicken.type != 34) {
        throw std::invalid_argument("Hidden chicken impact requires a live target");
    }

    if (!enabled) {
        return {};
    }

    HiddenImpact result{250, HiddenPopup{stored(static_cast<long double>(chicken.world_x) + 10), 163, 250},
        {HiddenSound::chicken_shot_close, HiddenSound::chicken_shot_mid, HiddenSound::chicken_shot_far, HiddenSound::magic_balloons}, 4, 32, 24};
    chicken.type = 74;
    chicken.frame = 0;
    chicken.animation_time = 0;

    return result;
}

std::optional<contracts::SpriteDraw> hidden_chicken_draw(const HiddenChicken& chicken) {
    return chicken_draw(chicken, false);
}

std::optional<contracts::SpriteDraw> hidden_chicken_collision(const HiddenChicken& chicken) {
    return chicken_draw(chicken, true);
}

std::optional<contracts::SpriteDraw> hidden_leaf_draw(const HiddenLeaf& leaf) {
    if (!valid(leaf)) {
        throw std::invalid_argument("Invalid hidden leaf draw state");
    }

    if (leaf.type == -1) {
        return std::nullopt;
    }

    return contracts::SpriteDraw{"gfx/Leaf", {0, 0, 60, 60}, {static_cast<int>(leaf.screen_x), static_cast<int>(leaf.screen_y)}, true};
}

std::optional<contracts::SpriteDraw> hidden_leaf_collision(const HiddenLeaf& leaf) {
    return hidden_leaf_draw(leaf);
}

} // namespace moorhuhn2::world
