#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "render/frame_catalog.hpp"
#include "render/framebuffer.hpp"
#include "render/round_scene.hpp"
#include "support/require.hpp"
#include "world/presentation.hpp"

#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::test::require;

game::RoundState& fixture(game::Round& round) {
    auto& state = const_cast<game::RoundState&>(round.state());
    state.frog.type = -1;
    state.hidden_chicken.type = -1;
    return state;
}

void tick(game::Round& round, float delta = 0, game::RoundInput input = {}, game::Round::PixelMatch match = {}) {
    game::test::advance(round, round.state().previous_sample + delta, input);
    round.resolve_actions(match);
}

void request(game::RoundState& state, int type) {
    // Accepted CRT: seed 22 first word 110 selects table 14/type4, then word 14808
    // gives world_x1176; seed 7 first word 61 selects table 29/type5 without more RNG.
    state.random_state = type == 4 ? 22 : 7;
    state.random_draws = 0;
    state.spawn_accumulator = 51;
}

void allocator() {
    for (int type : {4, 5}) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.trees_destroyed = {true, true};
        for (auto& target : state.chickens) {
            target.type = -2;
        }
        request(state, type);
        const auto full_random = game::test::after_leaves(game::test::snapshot(state));
        tick(round);
        require(state.random_draws == full_random.draws && state.random_state == full_random.state && !state.special_chicken_spawned && round.events().empty(),
            "Full pool consumes no spawn RNG or one-shot latch");
        state.chickens[70].type = -1;
        state.chickens[90].type = -1;
        request(state, type);
        tick(round);
        require(round.events().front().kind == game::RoundEventKind::spawn && round.events().front().slot == 70, "Special types use the first available scene slot");
        if (type == 4) {
            require(state.random_draws == game::test::after_leaves({0xb9d87f40U, 2}).draws && state.random_state == game::test::after_leaves({0xb9d87f40U, 2}).state
                        && state.big_chickens[70].world_x == 1176 && state.chickens[70].type == -5,
                "Type4 consumes its source position draw and reserves the slot");
            require(round.events().size() == 2 && round.events()[1].kind == game::RoundEventKind::big_chicken_appear && round.events()[1].pan < 0,
                "Type4 emits its source initial one-shot pan");
        } else {
            require(state.random_draws == game::test::after_leaves({0x003d7aaeU, 1}).draws && state.random_state == game::test::after_leaves({0x003d7aaeU, 1}).state
                        && state.special_chicken_spawned && state.special_chicken_slot == 70 && state.chickens[70].type == -6 && round.events().size() == 1,
                "Type5 latches only a successful allocation, without initializer RNG or sound");
            request(state, type);
            tick(round);
            require(round.events().empty() && state.chickens[90].type == -1 && state.random_draws == game::test::after_leaves({7, 0}, 1).draws
                        && state.random_state == game::test::after_leaves({7, 0}, 1).state,
                "One-shot type5 cannot allocate a second live record");
        }
        state.spawn_accumulator = -10000;
        tick(round, type == 4 ? 3.125F : 6.5F);
        require(state.chickens[70].type == -1, "Unhit terminal animation releases its original scene slot");
    }
    for (const auto flags : {std::array{false, false}, std::array{true, false}, std::array{false, true}}) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.trees_destroyed = flags;
        request(state, 5);
        tick(round);
        require(!state.special_chicken_spawned && state.special_chicken.type == -1 && state.chickens[34].type == -1
                    && state.random_draws == game::test::after_leaves({7, 0}, 1).draws && state.random_state == game::test::after_leaves({7, 0}, 1).state,
            "Both actual crown flags are required");
    }
}

std::vector<contracts::SpriteDraw> scene(const game::Round& round, const world::Presentation& presentation) {
    return render::round_scene(round, presentation);
}

void impacts(const render::ImageBank& images, const world::Presentation& presentation) {
    for (int type : {4, 5}) {
        for (bool special_ammo : {false, true}) {
            game::Round round(1, 0);
            auto& state = fixture(round);
            state.trees_destroyed = {true, true};
            state.special_ammo = special_ammo;
            request(state, type);
            tick(round);
            state.spawn_accumulator = -10000;
            tick(round, .6F);
            const auto draw = type == 4 ? *world::big_chicken_collision(state.big_chickens[34]) : *world::special_chicken_collision(state.special_chicken);
            render::Framebuffer frame;
            frame.draw(images, scene(round, presentation));
            std::optional<contracts::ScreenPoint> pointer;
            for (int y = std::max(20, draw.destination.y + 1); y < std::min(460, draw.destination.y + draw.source.height - 1) && !pointer; ++y) {
                for (int x = std::max(20, draw.destination.x + 1); x < std::min(620, draw.destination.x + draw.source.width - 1); ++x) {
                    if (frame.rgb565_at(x, y) != 0 && frame.matches_sprite_pixel(images, draw, {x, y})) {
                        pointer = contracts::ScreenPoint{x - 16, y - 16};
                        break;
                    }
                }
            }
            require(pointer.has_value(), "Actual original art must offer a visible non-key RGB565 aim in the composed scene");
            const auto random = game::test::after_leaves(game::test::snapshot(state));
            const int expected_points = type == 4 ? 25 : 20;
            tick(round, 0, {.pointer = *pointer, .fire_down = true}, [&](const auto& candidate, auto point) {
                return frame.matches_sprite_pixel(images, candidate, point);
            });
            require(state.selected_slot == 34 && state.score == expected_points && state.hits == 1 && state.misses == 0 && state.ammo_index == 9,
                "Composed-pixel input selects the new target, awards source points and consumes one cartridge");
            require(state.random_draws == random.draws && state.random_state == random.state && round.events().size() == 2 && round.events()[0].kind == game::RoundEventKind::shot
                        && round.events()[1].kind == game::RoundEventKind::special_chicken_hit,
                "Impact plays shotgun then generic chickenhit2 once, without RNG");
            require(state.particles[0].active && state.particles[0].points == expected_points && state.particles[0].depth == (type == 4 ? 1 : 2),
                "New impact allocates the original shared popup depth");
            const auto before_render = round.state();
            static_cast<void>(scene(round, presentation));
            require(round.state() == before_render, "Special target rendering cannot advance state");
            tick(round, 0, {.pointer = *pointer, .fire_down = true}, [](const auto& candidate, auto) {
                return candidate.image == "gfx/big_appear" || candidate.image == "gfx/specialhuhn";
            });
            require(state.score == expected_points && state.hits == 1 && state.misses == 1 && state.ammo_index == 10, "Reaction44/45 cannot be shot again");
            require(state.multi_hit.write_index == (type == 4 ? 1U : 0U), "Only reaction44 participates in the next-update multi-hit tracker");
            tick(round, .75F);
            require(state.chickens[34].type == -1, "Hit reaction removes and releases its scene slot");
            request(state, 4);
            tick(round);
            require(state.big_chickens[34].type == 4, "A released special slot can be reused by an ordinary random request");
        }
    }
}

void bounds_and_popup() {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 250;
    state.big_chickens[34] = world::initial_big_chicken(300, state.camera);
    state.chickens[34].type = -5;
    tick(round);
    // Strict rectangle edges reject even a matcher that reports equality.
    tick(round, 0, {.pointer = {-116, 84}, .fire_down = true}, [](const auto&, auto) {
        return true;
    });
    require(state.score == 0 && state.misses == 1, "Large chicken uses strict interior bounds");
    tick(round); // Clear recoil before the source pose fixture.
    state.ammo_index = 16;
    tick(round, 0, {.pointer = {20, 200}, .fire_down = true}, [](const auto&, auto) {
        return true;
    });
    require(state.big_chickens[34].type == 4 && state.hits == 0, "Empty magazine cannot hit the large target");
    state.ammo_index = 8;
    tick(round, 0, {.pointer = {20, 200}, .fire_down = true}, [](const auto& d, auto) {
        return d.image == "gfx/big_appear";
    });
    tick(round, .25F);
    const auto& popup = state.particles[0];
    require(popup.world_y == 279.5F && popup.screen_x == 35 && std::bit_cast<std::uint32_t>(popup.screen_y) == 0x43908ccdU,
        "Source depth1 popup uses rise 50, camera 1.6 and recoil 1.2");
    const game::Round fresh(1, 0);
    require(!fresh.state().special_chicken_spawned && fresh.state().special_chicken_slot == -1 && fresh.state().special_chicken.type == -1
                && std::ranges::all_of(fresh.state().big_chickens,
                    [](const auto& c) {
                        return c.type == -1;
                    }),
        "New round resets both dynamic families and the one-shot prerequisite latch");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected runtime asset path");
        allocator();
        bounds_and_popup();
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        impacts(images, world::Presentation(frames));
        std::cout << "Special spawn gates/RNG/pools, composed hits, reactions, popup, multi-hit and reset passed\n";
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
