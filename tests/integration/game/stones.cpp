#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "render/framebuffer.hpp"
#include "render/round_scene.hpp"
#include "support/require.hpp"
#include "ui/screens.hpp"
#include "world/presentation.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
struct InitialStones {
    float seed;
    std::array<contracts::ScreenPoint, 8> positions;
};

constexpr std::array initializers{
    InitialStones{0.0F, {{{1828, 351}, {1278, 410}, {771, 394}, {2077, 362}, {1910, 352}, {1024, 416}, {1927, 375}, {1940, 419}}}},
    InitialStones{1.0F, {{{1674, 379}, {648, 397}, {1316, 343}, {619, 381}, {1354, 370}, {1299, 376}, {382, 400}, {100, 361}}}},
    InitialStones{42.0F, {{{1361, 407}, {1803, 352}, {594, 374}, {1895, 398}, {673, 350}, {22, 415}, {1085, 350}, {465, 348}}}},
    InitialStones{12345.0F, {{{1444, 363}, {1559, 343}, {1955, 355}, {740, 349}, {1155, 343}, {2103, 359}, {251, 412}, {63, 362}}}},
};

std::vector<contracts::SpriteDraw> stone_draws(const game::Round& round) {
    std::vector<contracts::SpriteDraw> draws;
    for (const auto& stone : round.state().stones) {
        draws.push_back(world::stone_draw(stone));
    }
    return draws;
}

using moorhuhn2::test::require;

void step(game::Round& round, float time) {
    game::test::advance(round, time, {});
    round.resolve_actions({});
}

render::Framebuffer compose(const game::Round& round, const render::ImageBank& images, const world::Presentation& landscape) {
    const auto draws = render::round_scene(round, landscape);
    render::Framebuffer frame;
    frame.draw(images, draws);
    return frame;
}

struct Target {
    std::size_t index;
    contracts::ScreenPoint point;
};

Target visible_target(const game::Round& round, const render::ImageBank& images, const render::Framebuffer& frame) {
    const auto draws = stone_draws(round);
    for (std::size_t index = 0; index < draws.size(); ++index) {
        const auto& draw = draws[index];
        for (int y = 38; y < 77; ++y) {
            for (int x = 45; x < 106; ++x) {
                const contracts::ScreenPoint point{draw.destination.x + x, draw.destination.y + y};
                if (point.x < 64 || point.x >= 560 || point.y >= 415 || point.y < 0 || images.rgb565_at("gfx/stone", x, y) == 0xf81f) {
                    continue;
                }
                if (frame.matches_sprite_pixel(images, draw, point)) {
                    return {index, point};
                }
            }
        }
    }
    throw std::runtime_error("Seeded scene needs a source-visible stone target");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected adopted assets directory");
        for (const auto& initial : initializers) {
            const game::Round seeded(initial.seed, 0);
            for (std::size_t index = 0; index < seeded.state().stones.size(); ++index) {
                const auto& actual = seeded.state().stones[index];
                const auto& expected = initial.positions[index];
                require(actual.world_x == expected.x && actual.world_y == expected.y && actual.type == 10, "Original seeded stone placement differs");
            }
            require(seeded.state().random_draws == 176, "Original scene initialization RNG count differs");
        }
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog catalog(assets);
        const world::Presentation landscape(catalog);
        game::Round round(1, 0);
        step(round, 0);
        const auto frame = compose(round, images, landscape);
        const auto target = visible_target(round, images, frame);
        const auto original = round.state().stones[target.index];
        const auto draw = world::stone_draw(original);
        game::RoundInput shot;
        shot.pointer = {target.point.x - 16, target.point.y - 16};
        shot.fire_down = true;
        const auto random_before = game::test::after_leaves(game::test::snapshot(round.state()), 1);
        game::test::advance(round, 0, shot);
        round.resolve_actions([&](const auto& candidate, auto point) {
            return frame.matches_sprite_pixel(images, candidate, point);
        });
        require(round.state().score == 2 && round.state().hits == 1 && round.state().misses == 0 && round.state().stones_hit == 1 && round.state().ammo_index == 9,
            "Visible stone must award 2, credit one hit and consume a cartridge");
        require(round.state().random_draws == random_before.draws + 2 && round.state().stones[target.index].type == 50,
            "Frog and leaves consume the pre-input stream, then stone impact consumes two");
        require(round.events().size() == 2 && round.events()[0].kind == game::RoundEventKind::shot && round.events()[1].kind == game::RoundEventKind::stone_hit
                    && round.events()[1].slot == 24 + static_cast<int>(target.index) && round.events()[1].points == 2,
            "Shot and stone events preserve scene slot and order");
        require(round.state().particles[0].active && round.state().particles[0].age == 0 && !round.state().particles[1].active,
            "Stone allocates first available score slot without advancing it during input");

        // Only this stone's candidate can match: a dying object must not be queried.
        bool queried_dying = false;
        const auto repeated_random = game::test::after_leaves(game::test::snapshot(round.state()), 1);
        game::test::advance(round, 0, shot);
        round.resolve_actions([&](const auto& candidate, auto) {
            const bool same = candidate.image == draw.image && candidate.destination == draw.destination;
            queried_dying = queried_dying || same;
            return same;
        });
        require(!queried_dying && round.state().score == 2 && round.state().hits == 1 && round.state().misses == 1 && round.state().random_draws == repeated_random.draws
                    && round.state().random_state == repeated_random.state,
            "Repeated dying-stone shot must miss without score or RNG side effects");
        step(round, .25F);
        const auto& popup = round.state().particles[0];
        require(popup.active && popup.age == .25F && popup.world_x == original.world_x + 63 && popup.world_y == original.world_y + 42.5F && popup.screen_x == original.screen_x + 63
                    && popup.screen_y == original.world_y + 50.5F && popup.points == 2,
            "Source popup origin, rise, camera and recoil differ");
        step(round, 2);
        require(round.state().particles[0].active, "Popup survives exactly two seconds");
        round.reset_sample(0);
        step(round, 0x1p-23F);
        require(!round.state().particles[0].active && round.state().particles[0].age == 2.0F,
            "Popup lifetime comparison uses extended age even when its binary32 store rounds back to 2");
        require(round.state().chickens[24 + target.index].type == -2, "Dying stone must retain its reserved scene slot");

        game::Round occluded(1, 0);
        const auto occluded_random = game::test::after_leaves(game::test::snapshot(occluded.state()), 1);
        game::test::advance(occluded, 0, shot);
        render::Framebuffer cover;
        cover.clear(static_cast<std::uint16_t>(frame.rgb565_at(target.point.x, target.point.y) ^ 0xffffU));
        occluded.resolve_actions([&](const auto& candidate, auto point) {
            return candidate.image == draw.image && candidate.destination == draw.destination && cover.matches_sprite_pixel(images, candidate, point);
        });
        require(occluded.state().hits == 0 && occluded.state().misses == 1 && occluded.state().score == 0 && occluded.state().random_draws == occluded_random.draws
                    && occluded.state().random_state == occluded_random.state,
            "Occluding scene word must reject a stone without consuming impact RNG");

        for (const auto local : {contracts::ScreenPoint{0, 50}, {146, 50}, {60, 0}, {60, 110}}) {
            game::Round edge(1, 0);
            shot.pointer = {draw.destination.x + local.x - 16, draw.destination.y + local.y - 16};
            game::test::advance(edge, 0, shot);
            edge.resolve_actions([&](const auto& candidate, auto) {
                return candidate.image == draw.image && candidate.destination == draw.destination;
            });
            require(edge.state().hits == 0, "Stone rectangle excludes exact edges");
        }
        render::Framebuffer black;
        require(images.rgb565_at("gfx/stone", 10, 10) == 0xf81f && black.matches_sprite_pixel(images, draw, {draw.destination.x + 10, draw.destination.y + 10}),
            "Keyed scratch zero equality must not become an alpha-mask rejection");
        require(catalog.frame_source("gfx/stone", 20).y == 2200, "Final death cell must be available in the adopted asset grid");
        const auto before_render = round.state();
        for (int index = 0; index < 10; ++index) {
            static_cast<void>(stone_draws(round));
        }
        require(round.state() == before_render, "Stone render submission must not advance model state");
        game::Round reset(1, 0);
        require(reset.state().score == 0 && reset.state().stones_hit == 0 && reset.state().hits == 0 && reset.state().ammo_index == 8
                    && std::ranges::all_of(reset.state().stones,
                        [](const auto& stone) {
                            return stone.type == 10 && stone.frame == 0;
                        })
                    && std::ranges::none_of(reset.state().particles,
                        [](const auto& value) {
                            return value.active;
                        }),
            "New round resets all eight stones, score particles and impact bookkeeping");
        std::cout << "Stone source pixels, occlusion, strict bounds, impact RNG/score, repeated shot, popup and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
