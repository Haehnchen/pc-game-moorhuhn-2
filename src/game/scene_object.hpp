#pragma once

#include <cstddef>

namespace moorhuhn2::game {
struct RoundState;

namespace detail {
enum class SceneKind {
    none,
    village,
    boat,
    hunting_stand,
    hidden_chicken,
    hidden_leaf,
    rose,
    spider,
    scarecrow,
    tree,
    frog,
    branch,
    stone,
    tree_fragment,
    balloon,
    big_chicken,
    special_chicken,
    airplane,
    blimp,
    chicken
};

struct SceneObject {
    SceneKind kind{SceneKind::none};
    std::size_t index{};
    int type{-1};
    int depth{};
};

// Resolve ownership from live records, including reused early slots. Retained
// scarecrow pieces and sinking boats continue to own their original slots.
[[nodiscard]] SceneObject scene_object(const RoundState& state, std::size_t slot);
} // namespace detail
} // namespace moorhuhn2::game
