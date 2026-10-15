#include "game/scene_object.hpp"

#include "game/round.hpp"
#include "support/require.hpp"

#include <array>
#include <iostream>

using namespace moorhuhn2;
using game::detail::scene_object;
using game::detail::SceneKind;
using test::require;

int main() {
    try {
        game::RoundState state;

        // Adjacent array boundaries must not steal singleton slots.
        struct Expected {
            int slot;
            SceneKind kind;
            std::size_t index;
        };

        constexpr std::array boundaries{Expected{5, SceneKind::spider, 0}, Expected{6, SceneKind::scarecrow, 0}, Expected{13, SceneKind::scarecrow, 7},
            Expected{14, SceneKind::hidden_chicken, 0}, Expected{15, SceneKind::tree, 0}, Expected{18, SceneKind::tree, 3}, Expected{19, SceneKind::frog, 0},
            Expected{20, SceneKind::branch, 0}, Expected{23, SceneKind::branch, 3}, Expected{24, SceneKind::stone, 0}, Expected{31, SceneKind::stone, 7},
            Expected{32, SceneKind::rose, 0}, Expected{33, SceneKind::hidden_leaf, 0}};
        for (const auto& row : boundaries) {
            const auto object = scene_object(state, row.slot);
            require(object.kind == row.kind && object.index == row.index, "Static slot boundary belongs to its own record");
        }
        state.spider.type = -1;
        state.frog.type = -1;
        state.trees[0].type = -1;
        state.branches[3].type = -1;
        state.rose.type = -1;
        for (const int slot : {5, 15, 19, 23, 32, 34, 127, 255}) {
            state.chickens[slot].type = 2;
            state.chickens[slot].depth = 4;
            auto object = scene_object(state, slot);
            require(object.kind == SceneKind::chicken && object.index == static_cast<std::size_t>(slot) && object.type == 2 && object.depth == 4,
                "Vacated static slots and the expanded pool resolve to the new chicken");
            state.chickens[slot].type = game::balloon_scene_marker;
            state.balloons[slot].type = 21;
            object = scene_object(state, slot);
            require(object.kind == SceneKind::balloon && object.type == 21 && object.depth == 3, "A marker resolves to the owning balloon record");
        }
        state.scarecrow[0].type = 500;
        state.boat.sinking = true;
        require(scene_object(state, 6).kind == SceneKind::scarecrow && scene_object(state, 6).type == 500, "Inert scarecrow still owns its slot");
        require(scene_object(state, 3).kind == SceneKind::boat && scene_object(state, 3).type == 52, "Sinking boat keeps its reaction type");
        require(scene_object(state, state.chickens.size()).kind == SceneKind::none, "Out-of-pool lookups are empty");
        std::cout << "Scene slot ownership, boundaries and reuse passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
