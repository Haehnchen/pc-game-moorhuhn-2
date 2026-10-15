#include "game/round.hpp"
#include "game/round_math.hpp"
#include "game/scene_object.hpp"

#include <stdexcept>

namespace moorhuhn2::game {
using detail::stored;

void Round::load_ammo(int index, int state) {
    auto& ammo = state_.ammo[static_cast<std::size_t>(index)];
    ammo = AmmoSprite{};
    ammo.x = static_cast<float>(190 + index * 25);
    ammo.state = state;
    ammo.velocity_x = stored(1.0L + static_cast<long double>(random() % 4) * 0.25L);
    ammo.velocity_y = stored((-6.0L - static_cast<long double>((random() % 5) * 2)) * 0.1F);
}

int Round::select_target(const PixelMatch& pixel_match, int recoil) const {
    // The 32-pixel cursor uses its center; rectangle edges are excluded.
    const contracts::ScreenPoint hit{input_.pointer.x + 16, input_.pointer.y + 16};
    // Preserve pool order when a removed spider/frog slot is reused.
    for (std::size_t index = 0; index < state_.scene_capacity; ++index) {
        const auto candidate = collision_sprite(detail::scene_object(state_, index), recoil);
        if (candidate && hit.x > candidate->destination.x && hit.y > candidate->destination.y && hit.x < candidate->destination.x + candidate->source.width
            && hit.y < candidate->destination.y + candidate->source.height && pixel_match && pixel_match(*candidate, hit)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void Round::hit_target(int selected) {
    if (selected < 0) {
        ++state_.misses;
        return;
    }
    const auto object = detail::scene_object(state_, static_cast<std::size_t>(selected));
    switch (object.kind) {
        case detail::SceneKind::none:
            break;
        case detail::SceneKind::village:
            apply_village_hit(object);
            break;
        case detail::SceneKind::boat:
            apply_boat_hit();
            break;
        case detail::SceneKind::hunting_stand:
            apply_hunting_stand_hit(selected);
            break;
        case detail::SceneKind::hidden_chicken:
            apply_hidden_impact(world::hit_hidden_chicken(state_.hidden_chicken, state_.hidden_chicken_enabled), selected, 2);
            break;
        case detail::SceneKind::hidden_leaf:
            apply_hidden_impact(world::hit_hidden_leaf(state_.hidden_leaf, state_.hidden_chicken_enabled, state_.leaves_hit, state_.spider_hits, state_.branches_hit), selected, 3);
            state_.chickens[hidden_leaf_scene_slot].type = -1;
            break;
        case detail::SceneKind::rose:
            apply_rose_hit();
            break;
        case detail::SceneKind::spider:
            apply_spider_hit();
            break;
        case detail::SceneKind::scarecrow:
            if (object.index == 0) {
                ++state_.hits;
            } else {
                hit_scarecrow(object.index);
            }
            break;
        case detail::SceneKind::tree:
            hit_tree(object.index);
            break;
        case detail::SceneKind::frog:
            apply_frog_hit();
            break;
        case detail::SceneKind::branch:
            apply_branch_hit(object, selected);
            break;
        case detail::SceneKind::stone:
            apply_stone_hit(object);
            break;
        case detail::SceneKind::tree_fragment:
            if (object.type == 27) {
                apply_tree_leaf_hit(object, selected);
            } else {
                ++state_.hits;
            }
            break;
        case detail::SceneKind::balloon:
            apply_balloon_hit(object, selected);
            break;
        case detail::SceneKind::big_chicken:
            apply_big_chicken_hit(object, selected);
            break;
        case detail::SceneKind::special_chicken:
            apply_special_chicken_hit(selected);
            break;
        case detail::SceneKind::airplane:
            apply_airplane_events(world::hit_airplane(state_.airplane, state_.airplane_state));
            ++state_.hits;
            break;
        case detail::SceneKind::blimp:
            apply_blimp_hit(selected);
            break;
        case detail::SceneKind::chicken:
            apply_chicken_hit(object, selected);
            break;
    }
}

void Round::resolve_actions(const PixelMatch& pixel_match) {
    if (!awaiting_actions_) {
        throw std::logic_error("Round action resolution requires an updated/composed frame");
    }
    const int recoil = state_.recoil;
    state_.recoil = 0;
    state_.selected_slot = -1;
    if (input_.fire_down) {
        if (state_.ammo_index < ammo_capacity) {
            events_.push_back({RoundEventKind::shot});
            state_.ammo[static_cast<std::size_t>(state_.ammo_index)].state = 2;
            ++state_.ammo_index;
            state_.recoil = 8;
            state_.selected_slot = select_target(pixel_match, recoil);
            hit_target(state_.selected_slot);
        } else {
            events_.push_back({RoundEventKind::empty});
        }
    }
    if (input_.reload_down && state_.ammo_index == ammo_capacity) {
        events_.push_back({RoundEventKind::reload});
        state_.ammo_index = state_.reload_start_index;
        for (int index = state_.reload_start_index; index < ammo_capacity; ++index) {
            load_ammo(index, 1);
        }
    }
    awaiting_actions_ = false;
}

} // namespace moorhuhn2::game
