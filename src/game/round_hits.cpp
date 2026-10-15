#include "game/round.hpp"
#include "game/round_math.hpp"
#include "game/scene_object.hpp"

namespace moorhuhn2::game {
using detail::add_score;
using detail::stored;

void Round::apply_hidden_impact(const world::HiddenImpact& impact, int slot, int depth) {
    constexpr std::array sound_events{RoundEventKind::hidden_chicken_close, RoundEventKind::hidden_chicken_mid, RoundEventKind::hidden_chicken_far,
        RoundEventKind::hidden_chicken_balloons, RoundEventKind::hidden_leaf_hit};

    for (std::size_t index = 0; index < impact.sound_count; ++index) {
        events_.push_back({sound_events[static_cast<std::size_t>(impact.sounds[index])], slot, depth, impact.points});
    }

    // Type 34 uses its original 61x117 center even during the wider hide2 draw.
    centered_debris(stored(static_cast<long double>(state_.hidden_chicken.world_x) + 30), 163, 2, impact.particle_count);

    for (int index = 0; index < impact.balloon_count; ++index) {
        spawn_balloon();
    }

    add_score(state_.score, impact.points);

    if (impact.popup) {
        score_popup(impact.popup->world_x, impact.popup->world_y, depth, impact.popup->points);
    }

    ++state_.hits;
}

void Round::hit_tree(std::size_t index) {
    auto& tree = state_.trees[index];
    wood_debris(3);

    if ((tree.frame == 1 && !state_.trees_destroyed[0]) || (tree.frame == 3 && !state_.trees_destroyed[1])) {
        // The bound is redrawn after every request, including failed allocations.
        for (int count = 0; count < static_cast<int>(random() % 5) + 1; ++count) {
            spawn_tree_fragment(27);
        }
    }

    if (tree.frame == 0 || tree.frame == 2) {
        for (int count = 0; count < static_cast<int>(random() % 2); ++count) {
            spawn_tree_fragment(27);
        }
    }

    const int slot = first_tree_scene_slot + static_cast<int>(index);

    if (world::damage_tree(tree, state_.branches_hit)) {
        for (int count = 0; count < 4; ++count) {
            spawn_tree_fragment(28);
        }

        for (int count = 0; count < 16; ++count) {
            spawn_tree_fragment(27);
        }

        wood_debris(32);
        spawn_tree_fragment(30);

        if (tree.frame == 0 || tree.frame == 2) {
            state_.trees_destroyed[static_cast<std::size_t>(tree.frame / 2)] = true;
        }

        const auto random_x = random();
        const auto random_y = random();
        world::start_tree_fall(tree, random_x, random_y);
        events_.push_back({RoundEventKind::tree_destroyed, slot, 2});
    }

    events_.push_back({RoundEventKind::tree_hit, slot, 2});
    ++state_.hits;
}

void Round::hit_scarecrow(std::size_t index) {
    auto& piece = state_.scarecrow[index];
    auto* adjacent = index + 1 < state_.scarecrow.size() ? &state_.scarecrow[index + 1] : nullptr;
    const auto effect = world::prepare_scarecrow_hit(piece, adjacent);
    const auto popup = [&] {
        score_popup(effect.popup_x, effect.popup_y, 3, effect.popup_points);
    };
    const auto release = [&](world::ScarecrowPiece& target) {
        const auto random_x = random();
        const auto random_y = random();
        world::start_scarecrow_fall(target, random_x, random_y);
    };
    int points = piece.points;
    add_score(state_.score, piece.points);
    popup();
    release(piece);
    world::record_scarecrow_frame(state_.scarecrow_hits, state_.scarecrow_ordered, piece.frame);

    if (effect.release_hat) {
        // The original keeps the selected head for both popups and adds the
        // adjacent hat's points twice, despite crediting only one accurate shot.
        add_score(state_.score, adjacent->points);
        popup();
        release(*adjacent);
        add_score(state_.score, adjacent->points);
        world::record_scarecrow_frame(state_.scarecrow_hits, state_.scarecrow_ordered, adjacent->frame);
        points += 2 * adjacent->points;
    }

    ++state_.hits;
    events_.push_back({RoundEventKind::scarecrow_hit, first_scarecrow_scene_slot + static_cast<int>(index), 3, points});
}

void Round::apply_village_hit(const detail::SceneObject& object) {
    const auto impact = world::hit_village(state_.village, static_cast<world::VillageTarget>(static_cast<int>(object.index)), state_.frog_hits);
    constexpr std::array sound_events{RoundEventKind::church_bell, RoundEventKind::church_bell_second, RoundEventKind::house_hit};

    for (std::size_t index = 0; index < impact.sound_count; ++index) {
        events_.push_back({sound_events[static_cast<std::size_t>(impact.sounds[index])], static_cast<int>(object.index), 4, impact.points});
    }

    add_score(state_.score, impact.points);
    state_.remaining = stored(static_cast<long double>(state_.remaining) + impact.extra_seconds);

    if (impact.popup) {
        score_popup(impact.popup->world_x, impact.popup->world_y, 4, impact.popup->points);
    }

    if (impact.spawn_balloon) {
        spawn_balloon();
    }

    ++state_.hits;
}

void Round::apply_boat_hit() {
    ++state_.hits;
    events_.push_back({RoundEventKind::boat_hit, boat_scene_slot, 3, 0});

    if (world::hit_boat(state_.boat, state_.special_ammo)) {
        add_score(state_.score, 50);
        score_popup(678, 303, 3, 50);
        events_.push_back({RoundEventKind::boat_sunk, boat_scene_slot, 3, 50});
    }
}

void Round::apply_hunting_stand_hit(int selected) {
    const auto impact = world::hit_hunting_stand(state_.hunting_stand, state_.special_ammo);
    add_score(state_.score, impact.points);

    if (impact.popup) {
        score_popup(impact.popup->world_x, impact.popup->world_y, 3, impact.popup->points);
    }

    constexpr std::array sound_events{
        RoundEventKind::hunting_stand_chicken, RoundEventKind::hunting_stand_hit, RoundEventKind::hunting_stand_ladder, RoundEventKind::hunting_stand_broken};

    for (std::size_t index = 0; index < impact.sound_count; ++index) {
        events_.push_back({sound_events[static_cast<std::size_t>(impact.sounds[index])], selected, 3, impact.points});
    }

    ++state_.hits;
}

void Round::apply_rose_hit() {
    if (state_.score == 88 && state_.village.house_hit && state_.remaining >= 60.0F && state_.remaining <= 65.0F) {
        state_.spawn_threshold = 5.0F;
        state_.scene_capacity = contracts::scene_pool_capacity;
        state_.house88_mode = true;
        state_.reload_start_index = 0;
    }

    const auto impact = world::hit_rose(state_.rose, state_.remaining);
    add_score(state_.score, impact.points);
    state_.remaining = impact.remaining_after;
    score_popup(impact.popup.world_x, impact.popup.world_y, 0, impact.popup.points);
    ++state_.hits;
}

void Round::apply_spider_hit() {
    const auto impact = world::hit_spider(state_.spider, state_.spider_hits, state_.special_ammo);
    add_score(state_.score, impact.points);

    if (impact.popup) {
        score_popup(impact.popup->world_x, impact.popup->world_y, 2, impact.popup->points);
    }

    if (impact.swings_sound) {
        events_.push_back({RoundEventKind::spider_hit, spider_scene_slot, 2, impact.points});
    }

    ++state_.hits;
}

void Round::apply_frog_hit() {
    const auto impact = world::hit_frog(state_.frog, state_.trees_destroyed[0], state_.trees_destroyed[1], state_.remaining);

    for (int count = 0; count < impact.balloon_count; ++count) {
        spawn_balloon();
    }

    if (impact.balloon_count) {
        events_.push_back({RoundEventKind::frog_tree_bonus, frog_scene_slot, 2});
    }

    ++state_.frog_hits;
    add_score(state_.score, impact.points);
    score_popup(impact.popup.world_x, impact.popup.world_y, 2, impact.points);
    events_.push_back({RoundEventKind::frog_hit, frog_scene_slot, 2, impact.points});
    ++state_.hits;
}

void Round::apply_branch_hit(const detail::SceneObject& object, int selected) {
    wood_debris(10);
    const auto random_x = random();
    const auto random_y = random();
    world::hit_branch(state_.branches[object.index], random_x, random_y);
    ++state_.branches_hit;
    ++state_.hits;
    events_.push_back({RoundEventKind::branch_hit, selected, 2});
}

void Round::apply_stone_hit(const detail::SceneObject& object) {
    auto& stone = state_.stones[object.index];
    add_score(state_.score, 2);
    score_popup(stored(static_cast<long double>(stone.world_x) + 63), stored(static_cast<long double>(stone.world_y) + 55), 2, 2);
    // Sequence the original x/y RNG calls explicitly.
    const auto random_x = random();
    const auto random_y = random();
    world::hit_stone(stone, random_x, random_y);
    ++state_.stones_hit;
    ++state_.hits;
    events_.push_back({RoundEventKind::stone_hit, first_stone_scene_slot + static_cast<int>(object.index), 2, 2});
}

void Round::apply_tree_leaf_hit(const detail::SceneObject& object, int selected) {
    auto& leaf = state_.tree_fragments[object.index];
    const auto impact = world::hit_tree_leaf(leaf);
    ++state_.leaves_hit;
    ++state_.hits;
    add_score(state_.score, impact.points);
    score_popup(impact.popup_x, impact.popup_y, leaf.depth, impact.points);
    events_.push_back({RoundEventKind::leaf_hit, selected, leaf.depth, impact.points});
}

void Round::apply_balloon_hit(const detail::SceneObject& object, int selected) {
    auto& balloon = state_.balloons[object.index];
    const bool rare = balloon.frame == 4;

    if (rare) {
        events_.push_back({RoundEventKind::rare_balloon, selected, 3, balloon.points});
    }

    add_score(state_.score, balloon.points);
    score_popup(stored(static_cast<long double>(balloon.world_x) + (rare ? -19 : 1)), stored(static_cast<long double>(balloon.world_y) + 21), 3, balloon.points);

    if (rare) {
        centered_debris(stored(static_cast<long double>(balloon.world_x) + 11), stored(static_cast<long double>(balloon.world_y) + 21), 3, 32);
    }

    const auto random_x = random();
    const auto random_y = random();
    world::hit_balloon(balloon, random_x, random_y);
    ++state_.balloons_hit;
    ++state_.hits;
    events_.push_back({RoundEventKind::balloon_hit, selected, 3, balloon.points});
}

void Round::apply_big_chicken_hit(const detail::SceneObject& object, int selected) {
    const auto impact = world::hit_big_chicken(state_.big_chickens[object.index]);
    add_score(state_.score, impact.points);
    score_popup(impact.popup.world_x, impact.popup.world_y, 1, impact.popup.points);
    ++state_.hits;
    events_.push_back({RoundEventKind::special_chicken_hit, selected, 1, impact.points});
}

void Round::apply_special_chicken_hit(int selected) {
    const auto impact = world::hit_special_chicken(state_.special_chicken);
    add_score(state_.score, impact.points);
    score_popup(impact.popup.world_x, impact.popup.world_y, 2, impact.popup.points);
    ++state_.hits;
    events_.push_back({RoundEventKind::special_chicken_hit, selected, 2, impact.points});
}

void Round::apply_blimp_hit(int selected) {
    const auto impact = world::hit_blimp(state_.blimp, input_.pointer, state_.blimp_progress);
    add_score(state_.score, impact.points);
    score_popup(impact.popup.world_x, impact.popup.world_y, 4, impact.popup.points);
    events_.push_back({RoundEventKind::blimp_hit, selected, 4, impact.points});
    ++state_.hits;
}

void Round::apply_chicken_hit(const detail::SceneObject& object, int selected) {
    auto& chicken = state_.chickens[object.index];
    add_score(state_.score, chicken.points);
    ++state_.hits;
    events_.push_back({RoundEventKind::chicken_hit, selected, chicken.depth, chicken.points});
    chicken.type += 40;
    chicken.frame = 0;
    // MH2 deliberately retains animation_time at the hit transition.
}

} // namespace moorhuhn2::game
