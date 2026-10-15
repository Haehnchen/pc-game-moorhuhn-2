#include "contracts/angle.hpp"
#include "contracts/random.hpp"
#include "game/round.hpp"
#include "game/round_math.hpp"

#include <cmath>

namespace moorhuhn2::game {
using detail::depth_index;
using detail::parallax;
using detail::recoil_scale;
using detail::stored;
using detail::truncated;

namespace {
constexpr std::array<int, 32> spawn_types{0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 4, 0, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 5, 24, 18};
constexpr std::array<int, 4> spawn_depths{2, 2, 3, 4};
constexpr std::array<int, 3> points{5, 10, 25};
} // namespace

std::uint32_t Round::random() {
    ++state_.random_draws;

    return contracts::next_random(state_.random_state);
}

std::optional<std::size_t> Round::free_scene_slot() const {
    for (std::size_t index = 0; index < state_.scene_capacity; ++index) {
        if (state_.chickens[index].type == -1) {
            return index;
        }
    }

    return std::nullopt;
}

void Round::spawn(std::optional<int> requested_type) {
    const auto free = free_scene_slot();

    if (!free) {
        return;
    }

    // A full pool returns before drawing the automatic type or initializer RNG.
    const int type = requested_type ? *requested_type : spawn_types[random() % 32];
    const auto slot = *free;
    auto& target = state_.chickens[slot];

    if (type == 4) {
        auto& chicken = state_.big_chickens[slot];
        chicken = world::initial_big_chicken(random(), state_.camera);
        target.type = big_chicken_scene_marker;
        events_.push_back({RoundEventKind::spawn, static_cast<int>(slot), 1, chicken.points});
        events_.push_back({RoundEventKind::big_chicken_appear, static_cast<int>(slot), 1, 0, world::big_chicken_spawn_pan(chicken)});
        return;
    }

    if (type == 5) {
        if (const auto chicken = world::make_special_chicken(state_.trees_destroyed[0], state_.trees_destroyed[1], state_.special_chicken_spawned)) {
            state_.special_chicken = *chicken;
            state_.special_chicken_slot = static_cast<int>(slot);
            target.type = special_chicken_scene_marker;
            events_.push_back({RoundEventKind::spawn, static_cast<int>(slot), 2, chicken->points});
        }

        return;
    }

    if (type == 18) {
        std::vector<world::AirplaneEvent> effects;

        if (const auto airplane = world::make_airplane(
                state_.airplane_state,
                [&] {
                    return random();
                },
                effects)) {
            state_.airplane = *airplane;
            state_.airplane_slot = static_cast<int>(slot);
            target.type = airplane_scene_marker;
            events_.push_back({RoundEventKind::spawn, static_cast<int>(slot), 4, airplane->points});
            apply_airplane_events(effects);
        }

        return;
    }

    if (type == 24) {
        if (const auto blimp = world::initial_blimp(state_.camera, state_.blimp_progress, [&] {
                return random();
            })) {
            state_.blimp = *blimp;
            state_.blimp_slot = static_cast<int>(slot);
            target.type = blimp_scene_marker;
            events_.push_back({RoundEventKind::spawn, static_cast<int>(slot), 4, blimp->points});
        }

        return;
    }

    target = Chicken{};
    target.type = type;
    target.depth = spawn_depths[random() % 4];
    const auto depth = depth_index(target.depth);
    target.points = points[depth];

    if (type == 0 || type == 1) {
        constexpr std::array<int, 3> x_ranges{2272, 1987, 1704};
        constexpr std::array<float, 3> y_origins{410.0F, 310.0F, 215.0F};
        target.world_x = static_cast<float>(random() % static_cast<std::uint32_t>(x_ranges[depth]));
        target.world_y = y_origins[depth];
        target.phase = type == 1 ? 0.1F : 0.0F;
    } else {
        const bool from_left = random() % 2 == 0;
        const int camera = truncated(static_cast<long double>(state_.camera) * parallax[depth]);
        target.type = from_left ? 2 : 3;
        target.world_x = static_cast<float>(from_left ? camera - static_cast<int>(random() % 64) - 150 : 670 + camera);
        const int height_range = depth == 0 ? 256 : 128;
        target.world_y = static_cast<float>(height_range - static_cast<int>(random() % static_cast<std::uint32_t>(height_range)));
    }

    events_.push_back({RoundEventKind::spawn, static_cast<int>(slot), target.depth, target.points});
}

void Round::update_chicken(Chicken& chicken) {
    constexpr std::array<float, 3> circle_speed{0.01F, 0.006F, 0.003F};
    constexpr std::array<float, 3> circle_x{150.0F, 100.0F, 60.0F};
    constexpr std::array<float, 3> circle_y{150.0F, 100.0F, 90.0F};
    constexpr std::array<float, 3> horizontal_speed{80.0F, 40.0F, 20.0F};
    constexpr std::array<float, 3> world_end{3200.0F, 2800.0F, 2400.0F};
    constexpr std::array<float, 3> dead_horizontal_speed{5.0F, 2.5F, 1.25F};
    constexpr std::array<float, 3> dead_fall_speed{90.0F, 50.0F, 30.0F};

    if (chicken.type < 0) {
        return;
    }

    const auto depth = depth_index(chicken.depth);
    const bool dying = chicken.type >= 40;
    const int type = dying ? chicken.type - 40 : chicken.type;
    const bool circular = type == 0 || type == 1;
    const int camera = truncated(static_cast<long double>(state_.camera) * parallax[depth]);
    const long double delta = state_.delta;
    const long double angle = static_cast<long double>(chicken.phase) * contracts::degrees_per_radian;
    const long double sine = circular ? std::sin(angle) : 0;
    const long double cosine = circular ? std::cos(angle) : 0;
    const long double recoil = static_cast<long double>(state_.recoil) * recoil_scale[depth];

    if (dying) {
        const int direction = type == 0 || type == 3 ? -1 : 1;
        chicken.world_x = stored(chicken.world_x + direction * delta * dead_horizontal_speed[depth]);
        chicken.world_y = stored(chicken.world_y + delta * dead_fall_speed[depth]);

        if (circular) {
            chicken.screen_x = stored(sine * circle_x[depth] + chicken.world_x - camera);
            chicken.screen_y = stored(recoil + stored(cosine) * static_cast<long double>(circle_y[depth]) + chicken.world_y);
        } else {
            chicken.screen_x = stored(static_cast<long double>(chicken.world_x) - camera);
            chicken.screen_y = stored(recoil + chicken.world_y);
        }

        const float limit = circular ? (depth == 0 ? 580.0F : (depth == 1 ? 480.0F : 300.0F)) : (type == 2 && depth > 0 ? 300.0F : 480.0F);

        if (chicken.world_y > limit) {
            chicken.type = -1;
        }
    } else if (circular) {
        chicken.phase = stored(chicken.phase + (type == 0 ? 1 : -1) * delta * circle_speed[depth]);
        // Live flight stores FSIN to binary32; death motion retains its precision.
        chicken.screen_x = stored(static_cast<long double>(stored(sine)) * circle_x[depth] + chicken.world_x - camera);
        chicken.screen_y = stored(cosine * circle_y[depth] + chicken.world_y + recoil);

        if ((type == 0 && chicken.phase >= 0.1F) || (type == 1 && chicken.phase <= 0.0F)) {
            chicken.type = -1;
        }
    } else {
        chicken.world_x = stored(chicken.world_x + (type == 2 ? 1 : -1) * delta * horizontal_speed[depth]);
        chicken.screen_x = stored(static_cast<long double>(chicken.world_x) - camera);
        chicken.screen_y = stored(chicken.world_y + recoil);

        if ((type == 2 && chicken.world_x > world_end[depth]) || (type == 3 && chicken.world_x < -120.0F)) {
            chicken.type = -1;
        }
    }

    const long double animation = chicken.animation_time + delta * 3.0F * 5.0F;
    chicken.animation_time = stored(animation);
    chicken.frame = truncated(animation);

    if (chicken.frame >= (dying ? 10 : 19)) {
        chicken.frame = 0;
        chicken.animation_time = 0;
    }
}

void Round::update_ammo() {
    for (auto& ammo : state_.ammo) {
        if (ammo.state != 2) {
            continue;
        }

        ammo.x = stored(ammo.x + static_cast<long double>(ammo.velocity_x) * state_.delta * 150.0F);
        ammo.y = stored(ammo.y + static_cast<long double>(ammo.velocity_y) * state_.delta * 150.0F);
        ammo.velocity_x = stored(ammo.velocity_x + static_cast<long double>(state_.delta) * 1.5F);
        ammo.velocity_y = stored(ammo.velocity_y + static_cast<long double>(state_.delta) * 1.5F);
        const long double animation = ammo.animation_time + static_cast<long double>(state_.delta) * 50.0F;
        ammo.animation_time = stored(animation);

        if (ammo.animation_time >= 1.5F) {
            ammo.animation_time = stored(static_cast<long double>(ammo.animation_time) - 1.5L);

            if (++ammo.frame >= 21) {
                ammo.frame = 0;
            }
        }

        if (ammo.x > 640.0F || ammo.y > 480.0F) {
            ammo.state = 0;
        }
    }
}

void Round::update_balloon(std::size_t index) {
    auto& balloon = state_.balloons[index];

    if (balloon.type < 0) {
        return;
    }

    int drift = 0;

    if (balloon.type == 61 && random() % 5 == 0) {
        drift = static_cast<int>(random() % 3) - 1;
    }

    world::update_balloon(balloon, state_.delta, state_.camera, state_.recoil, drift);

    if (balloon.type < 0) {
        state_.chickens[index].type = -1;
    }
}

void Round::spawn_balloon() {
    const auto free = free_scene_slot();

    if (!free) {
        return;
    }

    const auto slot = *free;
    std::array<std::uint32_t, 5> draws{};

    for (auto& value : draws) {
        value = random();
    }

    state_.balloons[slot] = world::make_balloon(state_.camera, draws);
    state_.chickens[slot].type = balloon_scene_marker;
}

void Round::spawn_tree_fragment(int type) {
    const auto free = free_scene_slot();

    if (!free) {
        return;
    }

    const auto slot = *free;

    if (type == 27) {
        std::array<std::uint32_t, 6> draws{};

        for (auto& value : draws) {
            value = random();
        }

        state_.tree_fragments[slot] = world::make_tree_leaf(state_.camera, input_.pointer.x, draws);
    } else if (type == 28) {
        std::array<std::uint32_t, 5> draws{};

        for (auto& value : draws) {
            value = random();
        }

        state_.tree_fragments[slot] = world::make_tree_branch(state_.camera, input_.pointer.x, draws);
    } else {
        state_.tree_fragments[slot] = world::make_tree_highfly(state_.camera, input_.pointer.x);
    }

    state_.chickens[slot].type = tree_fragment_scene_marker;
}

} // namespace moorhuhn2::game
