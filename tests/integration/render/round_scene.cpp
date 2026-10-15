#include "render/round_scene.hpp"

#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <iostream>

using namespace moorhuhn2;
using test::require;

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected assets directory");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::FrameCatalog frames(assets);
        const world::Presentation landscape(frames);
        game::Round round(1, 0);
        round.advance({0, 0, 0}, {.pointer = {207, 101}});
        auto& state = const_cast<game::RoundState&>(round.state());
        state.branches[0].type = -1;
        state.rose.type = -1;
        state.particles[0] = {.active = true, .type = 3, .depth = 2, .screen_x = 123, .screen_y = 234, .frame = 2};
        state.particles[1] = {.active = true, .type = 5, .depth = 4};
        state.aircraft_smoke[1] = {.type = 5, .screen_x = 333, .screen_y = 44};
        state.particles[2] = {.active = true, .type = 2, .depth = 1, .screen_x = 75, .screen_y = 85, .points = 25};
        state.notices[0] = {.active = false, .visible = true, .y = 160, .type = world::notice_type::balloon_hero};
        const auto before = state;
        const auto expected = render::round_scene(round, landscape);
        require(state == before, "Composition does not advance clocks, RNG, notices or poses");
        const auto position = [&](std::string_view image, contracts::ScreenPoint point) {
            const auto found = std::ranges::find_if(expected, [&](const auto& draw) {
                return draw.image == image && draw.destination == point;
            });
            require(found != expected.end(), "Complete frame contains effect at its retained pose");
            return found - expected.begin();
        };
        const auto smoke = position("gfx/particle2", {333, 44});
        const auto debris = position("gfx/particle2", {123, 234});
        const auto popup = position("gfx/font", {75, 85});
        const auto notice = position("gfx/font", {160, 160});
        require(debris < popup && popup < notice, "Score popups precede visible notices, including just-expired entries");
        require(smoke < debris, "Distant smoke precedes near debris");
        require(std::ranges::count(expected, std::string_view("gfx/particle"), &contracts::SpriteDraw::image) == 32, "All persistent leaves participate in pixel occlusion");
        const auto foreground = std::ranges::find(expected, std::string_view("gfx/M2BG05"), &contracts::SpriteDraw::image);
        require(foreground != expected.end() && notice < foreground - expected.begin(), "Foreground follows effects");
        require(expected.back().image == "gfx/mousecursor" && expected.back().destination == contracts::ScreenPoint{207, 101},
            "Cursor is the final draw at the current input position");

        const auto same_draw = [](const contracts::SpriteDraw& left, const contracts::SpriteDraw& right) {
            return left.image == right.image && left.source.x == right.source.x && left.source.y == right.source.y && left.source.width == right.source.width
                   && left.source.height == right.source.height && left.destination == right.destination && left.color_key == right.color_key && left.mirror_x == right.mirror_x;
        };
        render::RoundScene composer;
        std::vector<contracts::SpriteDraw> appended{{"sentinel", {}, {}}};
        composer.append(appended, round, landscape, round.pointer());
        require(appended.front().image == "sentinel" && std::ranges::equal(std::span(appended).subspan(1), expected, same_draw),
            "Reusable composition appends without replacing preceding draws");
        appended.clear();
        composer.append(appended, round, landscape, round.pointer());
        require(std::ranges::equal(appended, expected, same_draw) && state == before, "Reused scratch storage produces an identical frame without accumulating draws");
        std::cout << "Complete round composition, painter order and scratch reuse passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
