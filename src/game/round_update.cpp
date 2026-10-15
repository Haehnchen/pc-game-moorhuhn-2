#include "game/round.hpp"
#include "game/round_math.hpp"
#include "game/scene_object.hpp"

namespace moorhuhn2::game {
using detail::stored;

void Round::update_scene_object(std::size_t index) {
    const int slot = static_cast<int>(index);
    const auto object = detail::scene_object(state_, index);
    switch (object.kind) {
        case detail::SceneKind::boat: {
            world::update_boat(state_.boat, state_.delta);
            break;
        }
        case detail::SceneKind::hunting_stand: {
            world::update_hunting_stand(state_.hunting_stand, state_.delta, state_.camera, state_.recoil);
            break;
        }
        case detail::SceneKind::hidden_chicken: {
            const auto draw =
                world::hidden_chicken_needs_random(state_.hidden_chicken, state_.delta, input_.pointer.x, state_.hidden_chicken_enabled) ? std::optional{random()} : std::nullopt;
            world::update_hidden_chicken(state_.hidden_chicken, state_.delta, state_.camera, state_.recoil, input_.pointer.x, state_.hidden_chicken_enabled, draw);

            if (state_.hidden_chicken.type == -1) {
                state_.chickens[index].type = -1;
            }
            break;
        }
        case detail::SceneKind::hidden_leaf: {
            world::update_hidden_leaf(state_.hidden_leaf, state_.camera, state_.recoil);
            break;
        }
        case detail::SceneKind::rose: {
            world::update_rose(state_.rose, state_.delta, state_.camera, state_.recoil);

            if (state_.rose.type == -1) {
                state_.chickens[index].type = -1;
            }
            break;
        }
        case detail::SceneKind::spider: {
            world::update_spider(state_.spider, state_.delta, state_.camera, state_.recoil);

            if (state_.spider.type == -1) {
                state_.chickens[index].type = -1;
            }
            break;
        }
        case detail::SceneKind::scarecrow: {
            // Fallen scarecrow pieces become inert type 500 and retain their slots.
            world::update_scarecrow_piece(state_.scarecrow[object.index], state_.delta, state_.camera, state_.recoil);
            break;
        }
        case detail::SceneKind::tree: {
            auto& tree = state_.trees[object.index];
            const auto effect = world::update_tree(tree, state_.delta, state_.camera, state_.recoil);

            if (effect.ground_sound) {
                state_.chickens[index].type = -1;
                events_.push_back({RoundEventKind::tree_ground, slot, 2});
            }
            break;
        }
        case detail::SceneKind::frog: {
            const auto draw = state_.frog.type == 11 ? std::optional{random()} : std::nullopt;
            const auto effect = world::update_frog(state_.frog, state_.delta, state_.camera, state_.recoil, draw);

            if (effect.croak_pan) {
                events_.push_back({RoundEventKind::frog_croak, slot, 2, 0, *effect.croak_pan});
            }

            if (state_.frog.type == -1) {
                state_.chickens[index].type = -1;
            }
            break;
        }
        case detail::SceneKind::branch: {
            auto& branch = state_.branches[object.index];
            const auto effect = world::update_branch(branch, state_.delta, state_.camera, state_.recoil);

            if (effect.ground_sound) {
                state_.chickens[index].type = -1;
                events_.push_back({RoundEventKind::branch_ground, slot, 2});
            }
            break;
        }
        case detail::SceneKind::stone: {
            world::update_stone(state_.stones[object.index], state_.delta, state_.camera, state_.recoil);
            break;
        }
        case detail::SceneKind::tree_fragment: {
            auto& fragment = state_.tree_fragments[object.index];
            const auto effect = world::update_tree_fragment(fragment, state_.delta, state_.camera, state_.recoil);

            if (fragment.type == -1) {
                state_.chickens[index].type = -1;
            }

            if (effect.ground_sound) {
                events_.push_back({RoundEventKind::tree_ground, slot, fragment.depth});
            }

            if (effect.highfly_sound) {
                events_.push_back({RoundEventKind::tree_highfly, slot, fragment.depth});
            }
            break;
        }
        case detail::SceneKind::big_chicken: {
            auto& big = state_.big_chickens[object.index];
            world::update_big_chicken(big, state_.delta, state_.camera, state_.recoil);

            if (big.type == -1) {
                state_.chickens[index].type = -1;
            }
            break;
        }
        case detail::SceneKind::special_chicken: {
            world::update_special_chicken(state_.special_chicken, state_.delta, state_.camera, state_.recoil);

            if (state_.special_chicken.type == -1) {
                state_.chickens[index].type = -1;
            }
            break;
        }
        case detail::SceneKind::airplane: {
            apply_airplane_events(world::update_airplane(state_.airplane, state_.airplane_state, state_.delta, state_.camera, state_.recoil, [&] {
                return random();
            }));

            if (state_.airplane.type == -1) {
                state_.chickens[index].type = -1;
            }
            break;
        }
        case detail::SceneKind::blimp: {
            if (world::update_blimp(state_.blimp, state_.delta, state_.camera, state_.recoil, state_.blimp_progress)) {
                const auto& blimp = state_.blimp;
                spawn_aircraft_smoke(
                    true, blimp.type, stored(static_cast<long double>(blimp.world_x) + blimp.hit_offset_x), stored(static_cast<long double>(blimp.world_y) + blimp.hit_offset_y));
            }

            if (state_.blimp.type == -1) {
                state_.chickens[index].type = -1;
            }
            break;
        }
        case detail::SceneKind::chicken:
            update_chicken(state_.chickens[index]);
            break;
        case detail::SceneKind::balloon:
            update_balloon(index);
            break;
        case detail::SceneKind::none:
        case detail::SceneKind::village:
            break;
    }
}

} // namespace moorhuhn2::game
