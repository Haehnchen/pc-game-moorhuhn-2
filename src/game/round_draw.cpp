#include "game/round.hpp"
#include "game/round_math.hpp"
#include "game/scene_object.hpp"

#include <stdexcept>
#include <string_view>

namespace moorhuhn2::game {
using detail::depth_index;
using detail::truncated;

namespace {
constexpr std::array<int, 3> widths{165, 83, 41};
constexpr std::array<int, 3> heights{124, 62, 31};
constexpr std::array<std::string_view, 3> flying_left{"gfx/Chickleft1", "gfx/Chickleft2", "gfx/Chickleft3"};
constexpr std::array<std::string_view, 3> flying_right{"gfx/Chickright1", "gfx/Chickright2", "gfx/Chickright3"};
constexpr std::array<std::string_view, 3> dying_left{"gfx/DChickleft1", "gfx/DChickleft2", "gfx/DChickleft3"};
constexpr std::array<std::string_view, 3> dying_right{"gfx/DChickright1", "gfx/DChickright2", "gfx/DChickright3"};
} // namespace

contracts::SpriteDraw chicken_draw(const Chicken& chicken) {
    if (chicken.depth < 2 || chicken.depth > 4 || !((chicken.type >= 0 && chicken.type <= 3) || (chicken.type >= 40 && chicken.type <= 43))) {
        throw std::invalid_argument("Cannot draw an inactive or unsupported chicken");
    }

    const auto index = depth_index(chicken.depth);
    const bool dead = chicken.type >= 40;
    const int live_type = dead ? chicken.type - 40 : chicken.type;
    const bool left = live_type == 0 || live_type == 3;
    const auto image = dead ? (left ? dying_left[index] : dying_right[index]) : (left ? flying_left[index] : flying_right[index]);

    return {image, {0, chicken.frame * heights[index], widths[index], heights[index]}, {truncated(chicken.screen_x), truncated(chicken.screen_y)}, true};
}

std::vector<contracts::DepthDraw> Round::target_draws(int depth) const {
    std::vector<contracts::DepthDraw> draws;
    draws.reserve(state_.scene_capacity);
    append_target_draws(depth, draws);

    return draws;
}

void Round::append_target_draws(int depth, std::vector<contracts::DepthDraw>& draws) const {
    for (std::size_t index = 0; index < state_.scene_capacity; ++index) {
        const auto object = detail::scene_object(state_, index);
        if (object.depth != depth) {
            continue;
        }
        std::optional<contracts::SpriteDraw> draw;
        switch (object.kind) {
            case detail::SceneKind::big_chicken:
                draw = world::big_chicken_draw(state_.big_chickens[object.index]);
                break;
            case detail::SceneKind::special_chicken:
                draw = world::special_chicken_draw(state_.special_chicken);
                break;
            case detail::SceneKind::airplane:
                draw = world::airplane_draw(state_.airplane);
                break;
            case detail::SceneKind::blimp:
                draw = world::blimp_draw(state_.blimp);
                break;
            case detail::SceneKind::spider:
                draw = world::spider_draw(state_.spider);
                break;
            case detail::SceneKind::hidden_chicken:
                draw = world::hidden_chicken_draw(state_.hidden_chicken);
                break;
            case detail::SceneKind::hidden_leaf:
                draw = world::hidden_leaf_draw(state_.hidden_leaf);
                break;
            case detail::SceneKind::frog:
                draw = world::frog_draw(state_.frog);
                break;
            case detail::SceneKind::stone:
                draw = world::stone_draw(state_.stones[object.index]);
                break;
            case detail::SceneKind::tree_fragment:
                draw = world::tree_fragment_draw(state_.tree_fragments[object.index]);
                break;
            case detail::SceneKind::balloon:
                draw = world::balloon_draw(state_.balloons[object.index]);
                break;
            case detail::SceneKind::chicken:
                if ((object.type >= 0 && object.type <= 3) || (object.type >= 40 && object.type <= 43)) {
                    draw = chicken_draw(state_.chickens[object.index]);
                }
                break;
            default:
                break; // Static objects are inserted by world::Presentation.
        }
        if (draw) {
            draws.push_back({depth, *draw, static_cast<int>(index)});
        }
    }
}

std::vector<contracts::SpriteDraw> Round::ammo_draws() const {
    std::vector<contracts::SpriteDraw> draws;

    for (const auto& ammo : state_.ammo) {
        if (ammo.state > 0) {
            draws.push_back({state_.special_ammo ? "gfx/flyaway3" : "gfx/flyaway1", {0, ammo.frame * 68, 90, 68}, {truncated(ammo.x), truncated(ammo.y)}, true});
        }
    }

    return draws;
}

} // namespace moorhuhn2::game
