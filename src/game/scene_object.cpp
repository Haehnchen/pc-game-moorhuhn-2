#include "game/scene_object.hpp"

#include "game/round.hpp"
#include "game/round_math.hpp"

namespace moorhuhn2::game::detail {
SceneObject scene_object(const RoundState& state, std::size_t index) {
    if (index >= state.chickens.size()) {
        return {};
    }
    const auto slot = static_cast<int>(index);
    if (slot < 3) {
        return {SceneKind::village, index, 31 + slot, 4};
    }
    if (slot == boat_scene_slot) {
        return {SceneKind::boat, 0, state.boat.sinking ? 52 : 12, 3};
    }
    if (slot == hunting_stand_scene_slot) {
        return {SceneKind::hunting_stand, 0, state.hunting_stand.type, 3};
    }
    if (slot == hidden_chicken_scene_slot && state.hidden_chicken.type >= 0) {
        return {SceneKind::hidden_chicken, 0, state.hidden_chicken.type, 2};
    }
    if (slot == hidden_leaf_scene_slot && state.hidden_leaf.type >= 0) {
        return {SceneKind::hidden_leaf, 0, state.hidden_leaf.type, 3};
    }
    if (slot == rose_scene_slot && state.rose.type >= 0) {
        return {SceneKind::rose, 0, state.rose.type, 0};
    }
    if (slot == spider_scene_slot && state.spider.type >= 0) {
        return {SceneKind::spider, 0, state.spider.type, 2};
    }
    if (slot_in_range(slot, first_scarecrow_scene_slot, state.scarecrow.size())) {
        return {SceneKind::scarecrow, index - first_scarecrow_scene_slot, state.scarecrow[index - first_scarecrow_scene_slot].type, 3};
    }
    if (slot_in_range(slot, first_tree_scene_slot, state.trees.size()) && state.trees[index - first_tree_scene_slot].type >= 0) {
        return {SceneKind::tree, index - first_tree_scene_slot, state.trees[index - first_tree_scene_slot].type, 2};
    }
    if (slot == frog_scene_slot && state.frog.type >= 0) {
        return {SceneKind::frog, 0, state.frog.type, 2};
    }
    if (slot_in_range(slot, first_branch_scene_slot, state.branches.size()) && state.branches[index - first_branch_scene_slot].type >= 0) {
        return {SceneKind::branch, index - first_branch_scene_slot, state.branches[index - first_branch_scene_slot].type, 2};
    }
    if (slot_in_range(slot, first_stone_scene_slot, state.stones.size()) && state.stones[index - first_stone_scene_slot].type >= 0) {
        return {SceneKind::stone, index - first_stone_scene_slot, state.stones[index - first_stone_scene_slot].type, 2};
    }
    if (state.tree_fragments[index].type >= 0) {
        return {SceneKind::tree_fragment, index, state.tree_fragments[index].type, state.tree_fragments[index].depth};
    }
    if (state.balloons[index].type >= 0) {
        return {SceneKind::balloon, index, state.balloons[index].type, 3};
    }
    if (state.big_chickens[index].type >= 0) {
        return {SceneKind::big_chicken, index, state.big_chickens[index].type, 1};
    }
    if (slot == state.special_chicken_slot && state.special_chicken.type >= 0) {
        return {SceneKind::special_chicken, 0, state.special_chicken.type, 2};
    }
    if (slot == state.airplane_slot && state.airplane.type >= 0) {
        return {SceneKind::airplane, 0, state.airplane.type, 4};
    }
    if (slot == state.blimp_slot && state.blimp.type >= 0) {
        return {SceneKind::blimp, 0, state.blimp.type, 4};
    }
    return {SceneKind::chicken, index, state.chickens[index].type, state.chickens[index].depth};
}
} // namespace moorhuhn2::game::detail
