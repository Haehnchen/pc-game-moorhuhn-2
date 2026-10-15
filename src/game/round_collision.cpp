#include "game/round.hpp"
#include "game/scene_object.hpp"

namespace moorhuhn2::game {
std::optional<contracts::SpriteDraw> Round::collision_sprite(const detail::SceneObject& object, int recoil) const {
    std::optional<contracts::SpriteDraw> candidate;
    switch (object.kind) {
        case detail::SceneKind::village: {
            candidate = world::village_draw(static_cast<world::VillageTarget>(object.index), state_.camera, recoil);
            break;
        }
        case detail::SceneKind::boat: {
            if (object.type == 12) {
                candidate = world::boat_collision(state_.camera, recoil);
            }
            break;
        }
        case detail::SceneKind::hunting_stand: {
            if (object.type >= 13 && object.type <= 15) {
                candidate = world::hunting_stand_collision(state_.hunting_stand);
            }
            break;
        }
        case detail::SceneKind::hidden_chicken: {
            if (object.type == 34) {
                candidate = world::hidden_chicken_collision(state_.hidden_chicken);
            }
            break;
        }
        case detail::SceneKind::hidden_leaf: {
            if (object.type == 35) {
                candidate = world::hidden_leaf_collision(state_.hidden_leaf);
            }
            break;
        }
        case detail::SceneKind::rose: {
            if (object.type == 29) {
                candidate = world::rose_collision(state_.rose);
            }
            break;
        }
        case detail::SceneKind::spider: {
            if (object.type == 20) {
                candidate = world::spider_collision(state_.spider);
            }
            break;
        }
        case detail::SceneKind::scarecrow: {
            const auto& piece = state_.scarecrow[object.index];

            if (piece.type == 16 || piece.type == 17) {
                candidate = world::scarecrow_draw(piece);

                if (piece.type == 16) {
                    candidate->image = {};
                }
            }
            break;
        }
        case detail::SceneKind::tree: {
            const auto& tree = state_.trees[object.index];

            if (tree.type == 26) {
                candidate = world::tree_draw(tree);
            }
            break;
        }
        case detail::SceneKind::frog: {
            if (object.type == 11) {
                candidate = world::frog_draw(state_.frog);
            }
            break;
        }
        case detail::SceneKind::branch: {
            const auto& branch = state_.branches[object.index];

            if (branch.type == 22 || branch.type == 23) {
                candidate = world::branch_draw(branch);
            }
            break;
        }
        case detail::SceneKind::stone: {
            const auto& stone = state_.stones[object.index];

            if (stone.type == 10) {
                candidate = world::stone_draw(stone);
            }
            break;
        }
        case detail::SceneKind::tree_fragment: {
            if (object.type == 27 || object.type == 28 || object.type == 30) {
                const auto& fragment = state_.tree_fragments[object.index];
                candidate = world::tree_fragment_draw(fragment);

                if (fragment.type != 27) {
                    // These source cases keep the cleared scratch surface;
                    // their strict rectangles can still match black scene words.
                    candidate->image = {};
                    candidate->source.x = 0;
                    candidate->source.y = 0;
                }
            }
            break;
        }
        case detail::SceneKind::balloon: {
            if (object.type == 21) {
                candidate = world::balloon_draw(state_.balloons[object.index]);
            }
            break;
        }
        case detail::SceneKind::big_chicken: {
            if (object.type == 4) {
                candidate = world::big_chicken_collision(state_.big_chickens[object.index]);
            }
            break;
        }
        case detail::SceneKind::special_chicken: {
            if (object.type == 5) {
                candidate = world::special_chicken_collision(state_.special_chicken);
            }
            break;
        }
        case detail::SceneKind::airplane: {
            candidate = world::airplane_collision(state_.airplane);
            break;
        }
        case detail::SceneKind::blimp: {
            candidate = world::blimp_collision(state_.blimp);
            break;
        }
        case detail::SceneKind::chicken: {
            if (object.type >= 0 && object.type <= 3) {
                candidate = chicken_draw(state_.chickens[object.index]);
            }
            break;
        }
        case detail::SceneKind::none:
            break;
    }
    return candidate;
}

} // namespace moorhuhn2::game
