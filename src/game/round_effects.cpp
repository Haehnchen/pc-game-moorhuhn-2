#include "game/round.hpp"
#include "game/round_math.hpp"
#include "game/scene_object.hpp"

#include <algorithm>
#include <limits>

namespace moorhuhn2::game {
using detail::add_score;
using detail::depth_index;
using detail::parallax;
using detail::recoil_scale;
using detail::stored;
using detail::truncated;

void Round::apply_airplane_events(std::span<const world::AirplaneEvent> effects) {
    constexpr std::array sound_events{RoundEventKind::airplane_engine_start, RoundEventKind::airplane_engine_pan, RoundEventKind::airplane_engine_stop,
        RoundEventKind::airplane_failure_start, RoundEventKind::airplane_failure_pan, RoundEventKind::airplane_failure_stop, RoundEventKind::airplane_hit};

    for (const auto& effect : effects) {
        if (effect.kind == world::AirplaneEventKind::penalty) {
            add_score(state_.score, effect.points);
            score_popup(effect.world_x, effect.world_y, 4, effect.points);
        } else if (effect.kind == world::AirplaneEventKind::smoke) {
            spawn_aircraft_smoke(false, effect.source_type, effect.world_x, effect.world_y);
        } else if (effect.kind == world::AirplaneEventKind::crash) {
            events_.push_back({RoundEventKind::airplane_crash, state_.airplane_slot, 4});
        } else {
            events_.push_back({sound_events[static_cast<std::size_t>(effect.kind)], state_.airplane_slot, 4, 0, effect.pan});
        }
    }
}

void Round::spawn_aircraft_smoke(bool blimp, int source_type, float x, float y) {
    const auto free = std::ranges::find_if(state_.particles, [](const auto& particle) {
        return !particle.active;
    });

    if (free == state_.particles.end()) {
        return;
    }

    const auto index = static_cast<std::size_t>(free - state_.particles.begin());
    state_.aircraft_smoke[index] = blimp ? world::make_blimp_smoke(x, y,
                                               [&] {
                                                   return random();
                                               })
                                         : world::make_airplane_smoke(source_type, x, y, [&] {
                                               return random();
                                           });

    *free = Particle{};
    free->active = true;
    free->type = blimp ? 4 : 5;
    free->depth = 4;
}

void Round::score_popup(float x, float y, int depth, int points) {
    const auto found = std::ranges::find_if(state_.particles, [](const Particle& popup) {
        return !popup.active;
    });

    if (found == state_.particles.end()) {
        return;
    }

    *found = Particle{};
    found->active = true;
    found->world_x = x;
    found->world_y = y;
    found->points = points;
    found->depth = depth;
}

void Round::centered_debris(float x, float y, int depth, int count) {
    for (auto& particle : state_.particles) {
        if (count <= 0) {
            break;
        }
        if (particle.active) {
            continue;
        }

        particle = Particle{};
        particle.active = true;
        particle.type = 3;
        particle.depth = depth;
        particle.world_x = x;
        particle.world_y = y;
        particle.velocity_x = stored((static_cast<int>(random() % 64) - 32) * static_cast<long double>(0.01F));
        particle.velocity_y = stored((static_cast<int>(random() % 20) - 5) * static_cast<long double>(0.02F));
        --count;
    }
}

void Round::wood_debris(int count) {
    for (auto& particle : state_.particles) {
        if (count <= 0) {
            break;
        }
        if (particle.active) {
            continue;
        }

        particle = Particle{};
        particle.active = true;
        particle.type = 3;
        particle.world_x = stored(static_cast<long double>(state_.camera) * 1.6F + input_.pointer.x + 16);
        particle.world_y = static_cast<float>(input_.pointer.y + 16);
        // Only successful allocations consume these three draws, in this order.
        particle.frame = 1 + static_cast<int>(random() % 3);
        particle.velocity_x = stored((static_cast<int>(random() % 64) - 32) * static_cast<long double>(0.01F));
        particle.velocity_y = stored((static_cast<int>(random() % 20) - 1) * static_cast<long double>(0.1F));
        --count;
    }
}

void Round::update_particles() {
    // The original scans all 100 particle records in ascending order. Persistent
    // leaves consume their conditional random draws before transient effects.
    for (auto& leaf : state_.persistent_leaves) {
        world::update_persistent_leaf(leaf, state_.delta, state_.camera, state_.recoil, [&] {
            return random();
        });
    }

    for (std::size_t index = 0; index < state_.particles.size(); ++index) {
        auto& particle = state_.particles[index];

        if (!particle.active) {
            continue;
        }

        if (particle.type == 4 || particle.type == 5) {
            auto& smoke = state_.aircraft_smoke[index];
            world::update_aircraft_smoke(smoke, state_.delta, state_.camera, state_.recoil);
            particle.active = smoke.type >= 0;
            continue;
        }

        const bool foreground = particle.depth <= 1;
        const auto depth = foreground ? 0 : depth_index(particle.depth);
        constexpr std::array rise_speed{50.0F, 25.0F, 12.5F};
        const float camera_scale = particle.depth == 0 ? 1.8F : parallax[depth];
        const int camera = truncated(static_cast<long double>(state_.camera) * camera_scale);
        const long double delta = state_.delta;

        if (particle.type == 3) {
            particle.velocity_x = stored(particle.velocity_x + (particle.velocity_x > 0 ? 1 : -1) * delta * 0.01F);
            particle.velocity_y = stored(particle.velocity_y + delta * 0.4F);
            particle.world_x = stored(particle.world_x + delta * particle.velocity_x * 100.0F);
            particle.world_y = stored(particle.world_y + delta * particle.velocity_y * 100.0F);
        } else {
            particle.world_y = stored(particle.world_y - delta * rise_speed[depth]);
        }

        particle.screen_x = stored(static_cast<long double>(particle.world_x) - camera);
        // Type 3 debris always uses 0.8 recoil, even when its branch is depth2.
        const float recoil_factor = particle.type == 3 ? 0.8F : foreground ? 1.2F : recoil_scale[depth];
        particle.screen_y = stored(static_cast<long double>(particle.world_y) + state_.recoil * static_cast<long double>(recoil_factor));

        if (particle.type == 3) {
            if (particle.world_y > 480.0F) {
                particle.active = false;
            }
        } else {
            // The EXE selects 53-bit x87 arithmetic. FST stores binary32 but
            // retains that rounded sum for FCOMP; do not compare the field.
            static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53);
            const volatile double age = static_cast<double>(particle.age) + state_.delta;
            particle.age = static_cast<float>(age);

            if (age > 2.0) {
                particle.active = false;
            }
        }
    }
}

void Round::grant_stone_bonus() {
    if (state_.stones_hit != 8 || state_.stone_bonus_granted) {
        return;
    }

    state_.stone_bonus_granted = true;
    state_.special_ammo = true;

    for (int index = 0; index < 16; ++index) {
        spawn_balloon();
    }

    events_.push_back({RoundEventKind::special_ammo});
}

void Round::grant_spider_bonus() {
    const auto bonus = world::apply_spider_bonus(state_.spider_hits, state_.spider_hero);

    if (bonus.kind == world::SpiderBonusKind::none) {
        return;
    }

    if (bonus.kind == world::SpiderBonusKind::hero) {
        events_.push_back({RoundEventKind::spider_balloons, spider_scene_slot, 2});
        events_.push_back({RoundEventKind::spider_hero, spider_scene_slot, 2, bonus.points});
    }

    for (int index = 0; index < bonus.balloon_count; ++index) {
        spawn_balloon();
    }

    if (bonus.kind == world::SpiderBonusKind::first) {
        events_.push_back({RoundEventKind::spider_balloons, spider_scene_slot, 2});
    }

    if (bonus.notice) {
        static_cast<void>(world::allocate_notice(state_.notices, bonus.notice->type, bonus.notice->y));
    }

    add_score(state_.score, bonus.points);
}

void Round::grant_scarecrow_bonus() {
    const auto effect = world::apply_scarecrow_completion(state_.scarecrow_hits, state_.scarecrow_complete, state_.scarecrow_ordered);

    for (int index = 0; index < effect.balloon_count; ++index) {
        spawn_balloon();
    }

    for (int index = 0; index < effect.chicken_count; ++index) {
        spawn(2);
    }

    if (effect.magic_sound) {
        events_.push_back({RoundEventKind::scarecrow_balloons});
    } else if (effect.chicken_count) {
        events_.push_back({RoundEventKind::scarecrow_chickens});
    }
}

void Round::grant_balloon_bonus() {
    if (state_.balloons_hit != 25 || state_.balloon_hero) {
        return;
    }

    events_.push_back({RoundEventKind::balloon_hero, -1, 0, 100});
    state_.balloon_hero = true;

    for (int index = 0; index < 6; ++index) {
        spawn(4);
    }

    static_cast<void>(world::allocate_notice(state_.notices, world::notice_type::balloon_hero, 180));
    add_score(state_.score, 100);
}

std::optional<int> Round::selected_type() const {
    if (state_.selected_slot < 0) {
        return std::nullopt;
    }
    return detail::scene_object(state_, static_cast<std::size_t>(state_.selected_slot)).type;
}

void Round::update_multi_hit() {
    const bool available = std::ranges::any_of(state_.notices, [](const auto& notice) {
        return !notice.active;
    });

    // Read the previous selected slot after its world update: its reaction can
    // already have ended or its slot can have been reused by a different object.
    const auto effect = specials::update_multi_hit(state_.multi_hit, state_.delta, selected_type(), available);

    if (effect.notice) {
        static_cast<void>(world::allocate_notice(state_.notices, effect.notice->kind, effect.notice->y));
    }

    add_score(state_.score, effect.points);

    if (effect.score_sound) {
        events_.push_back({RoundEventKind::multi_hit_bonus, -1, 0, effect.points});
    }
}

} // namespace moorhuhn2::game
