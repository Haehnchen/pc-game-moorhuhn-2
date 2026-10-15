#include "world/boat.hpp"

#include "../../game/round_clock.hpp"
#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "render/framebuffer.hpp"
#include "support.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::require;

struct Position {
    float camera;
    int recoil;
    contracts::ScreenPoint destination;
    int frame;
};

constexpr std::array positions{
    Position{0.0F, 0, {626, 264}, 0},
    Position{128.0F, 0, {447, 264}, 0},
    Position{160.0F, 0, {403, 264}, 0},
    Position{256.0F, 0, {268, 264}, 0},
    Position{320.0F, 0, {179, 264}, 0},
    Position{640.0F, 0, {-269, 264}, 0},
    Position{1280.0F, 0, {-1165, 264}, 0},
    Position{320.0F, 8, {179, 270}, 0},
};

struct CollisionProbe {
    contracts::ScreenPoint pointer, local;
    std::optional<std::uint16_t> scene_word;
    std::uint16_t candidate_word;
    bool source_pixel, matches;
};

const std::array collision_probes{
    CollisionProbe{{233, 290}, {70, 42}, 45927, 45927, true, true},
    CollisionProbe{{164, 249}, {1, 1}, 62919, 0, true, false},
    CollisionProbe{{233, 290}, {70, 42}, 48879, 45927, false, false},
    CollisionProbe{{233, 290}, {70, 42}, 45927, 45927, false, true},
    CollisionProbe{{164, 249}, {1, 1}, 0, 0, false, true},
    CollisionProbe{{163, 290}, {0, 42}, std::nullopt, 0, false, false},
    CollisionProbe{{307, 290}, {144, 42}, std::nullopt, 0, false, false},
    CollisionProbe{{233, 248}, {70, 0}, std::nullopt, 0, false, false},
    CollisionProbe{{233, 327}, {70, 79}, std::nullopt, 0, false, false},
};

game::Round visible_round() {
    game::Round round(1, 0);
    game::RoundInput input;
    input.held_left = true;
    game::test::advance(round, 1.6F, input);
    round.resolve_actions({});
    return round;
}

void shoot(game::Round& round, const render::ImageBank& images, contracts::ScreenPoint local, bool occluded = false, bool reload = false) {
    const auto draw = world::boat_draw(round.state().camera, round.state().recoil);
    game::RoundInput input;
    input.pointer = {draw.destination.x + local.x - 16, draw.destination.y + local.y - 16};
    input.fire_down = true;
    input.reload_down = reload;
    game::test::advance(round, round.state().previous_sample, input);
    render::Framebuffer frame;
    if (!occluded) {
        frame.draw(images, std::span(&draw, 1));
    }
    round.resolve_actions([&](const auto& candidate, auto point) {
        return candidate.image == "gfx/boatcol" && frame.matches_sprite_pixel(images, candidate, point);
    });
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected adopted assets directory");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const std::array source{render::NamedImageView{"gfx/boat", assets.image("gfx/boat")}, render::NamedImageView{"gfx/boatcol", assets.image("gfx/boatcol")}};
        const render::ImageBank images(source);
        for (const auto& position : positions) {
            const auto actual = world::boat_draw(position.camera, position.recoil);
            require(actual.destination == position.destination && actual.source.y == position.frame, "Original-derived boat position vector differs");
        }
        for (const auto& probe : collision_probes) {
            auto candidate_round = visible_round();
            game::RoundInput input;
            input.pointer = probe.pointer;
            input.fire_down = true;
            game::test::advance(candidate_round, 1.6F, input);
            render::Framebuffer scene;
            if (probe.scene_word.has_value()) {
                scene.clear(*probe.scene_word);
                require(images.rgb565_at("gfx/boatcol", probe.local.x, probe.local.y) == probe.candidate_word, "Original collision sample differs");
                if (probe.source_pixel) {
                    require(images.rgb565_at("gfx/boat", probe.local.x, probe.local.y) == *probe.scene_word, "Original visible source sample differs");
                }
            }
            candidate_round.resolve_actions([&](const auto& candidate, auto point) {
                return candidate.image == "gfx/boatcol" && scene.matches_sprite_pixel(images, candidate, point);
            });
            require(candidate_round.state().hits == (probe.matches ? 1 : 0), "Original-derived collision/occlusion vector differs");
        }
        const auto draw = world::boat_draw(320, 0);
        require(draw.destination.x == 179 && draw.destination.y == 264, "Original boat camera position changed");
        require(world::boat_draw(320, 8).destination.y == 270, "Boat recoil must use stored binary32 y");
        require(images.rgb565_at("gfx/boat", 70, 42) == 0xb367 && images.rgb565_at("gfx/boatcol", 70, 42) == 0xb367, "Original hull sample differs");
        require(images.rgb565_at("gfx/boatcol", 20, 10) == 0 && images.rgb565_at("gfx/boat", 20, 10) != 0, "Original background exclusion differs");

        auto round = visible_round();
        shoot(round, images, {20, 10});
        require(round.state().hits == 0 && round.state().misses == 1 && round.state().score == 0, "Embedded scenery must not select the boat");
        for (int index = 0; index < 3; ++index) {
            shoot(round, images, {70, 42});
            require(round.state().hits == index + 1 && round.state().misses == 1 && round.state().score == 0, "Normal boat hits credit accuracy without score");
            const auto events = round.events();
            require(events.size() == 2 && events[0].kind == game::RoundEventKind::shot && events[1].kind == game::RoundEventKind::boat_hit && events[1].slot == 3
                        && events[1].depth == 3 && events[1].points == 0,
                "One shot must emit exactly one ordered boat impact");
            require(world::boat_draw(round.state().camera, round.state().recoil).source.y == 0, "Three ordinary hits must not sink the boat");
        }
        require(round.state().ammo_index == 12, "Boat impacts must consume cartridges");
        shoot(round, images, {70, 42}, true);
        require(round.state().hits == 3 && round.state().misses == 2, "An occluded hull must miss");
        for (int index = 0; index < 3; ++index) {
            shoot(round, images, {70, 42});
        }
        const auto hits = round.state().hits;
        shoot(round, images, {70, 42});
        require(round.state().hits == hits && round.events().size() == 1 && round.events()[0].kind == game::RoundEventKind::empty, "Empty fire cannot impact the boat");
        shoot(round, images, {70, 42}, false, true);
        require(round.state().ammo_index == 8 && round.state().hits == hits, "Empty fire then reload must not credit an impact");
        shoot(round, images, {70, 42});
        require(round.state().hits == hits + 1 && round.state().score == 0, "Boat remains eligible after repeated hits and reload");

        for (const auto local : {contracts::ScreenPoint{0, 42}, {144, 42}, {70, 0}, {70, 79}}) {
            auto edge_round = visible_round();
            shoot(edge_round, images, local);
            require(edge_round.state().hits == 0 && edge_round.state().misses == 1, "Strict boat rectangle must exclude its boundary");
        }
        auto fresh = visible_round();
        require(fresh.state().hits == 0 && fresh.state().misses == 0 && fresh.state().ammo_index == 8, "New round must reset impact bookkeeping");
        shoot(fresh, images, {70, 42});
        require(fresh.state().hits == 1 && fresh.state().score == 0, "New round must restore ordinary boat eligibility");

        // The original compares whole words, even when keyed scratch pixels are zero.
        render::Framebuffer black;
        const auto candidate = world::boat_collision(320, 0);
        require(black.matches_sprite_pixel(images, candidate, {draw.destination.x + 20, draw.destination.y + 10}), "Do not replace scratch equality with an opacity mask");
        std::cout << "Original boat collision, ordinary impacts, recoil, repeat, empty/reload and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
