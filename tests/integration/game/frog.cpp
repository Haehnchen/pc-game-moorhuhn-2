#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "render/framebuffer.hpp"
#include "render/round_scene.hpp"
#include "support.hpp"
#include "support/require.hpp"
#include "world/presentation.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::event;
using moorhuhn2::game::test::fixture;

using moorhuhn2::test::require;

// Unit-only prerequisites cover exact clock boundaries and exhausted pools.

void step(game::Round& round, float sample, game::RoundInput input = {}) {
    game::test::advance(round, sample, input);
    round.resolve_actions({});
}

std::vector<contracts::SpriteDraw> commands(const game::Round& round, const world::Presentation& landscape) {
    return render::round_scene(round, landscape);
}

void hit_frog(game::Round& round, const render::ImageBank& images, const world::Presentation& landscape) {
    game::RoundInput input;
    input.fire_down = true;
    input.pointer = {506, 344}; // Independent original local 82,30 at camera0.
    game::test::advance(round, round.state().previous_sample, input);
    render::Framebuffer frame;
    frame.draw(images, commands(round, landscape));
    round.resolve_actions([&](const auto& draw, auto point) {
        return frame.matches_sprite_pixel(images, draw, point);
    });
}

void ordinary(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 0;
    require(std::ranges::count_if(std::span{state.chickens}.first(state.scene_capacity),
                [](const auto& value) {
                    return value.type == -2;
                })
                == 34,
        "Original initializers occupy 34 records within the active 128-scene scan, including seven scarecrow pieces and one rose");
    require(state.frog.type == 11 && state.frog.screen_x == -1024 && state.frog_hits == 0 && state.random_draws == 176, "Original frog initializer/reset differs");
    const auto first_random = game::test::after_leaves(game::test::snapshot(state), 1);
    hit_frog(round, images, landscape);
    require(state.score == 5 && state.hits == 1 && state.misses == 0 && state.ammo_index == 9 && state.frog_hits == 1, "Actual frog pixel must award 5 once");
    require(
        state.frog.type == 51 && state.frog.frame == 12 && state.frog.animation_time == 12 && state.random_draws == first_random.draws && state.random_state == first_random.state,
        "Frog and leaves consume the pre-input stream; impact selects death12 without random calls");
    require(state.particles[0].points == 5 && state.particles[0].depth == 2 && state.particles[0].world_x == 490 && state.particles[0].world_y == 370,
        "Frog popup must preserve its original depth2 center");
    require(round.events().size() == 2 && round.events()[0].kind == game::RoundEventKind::shot && round.events()[1].kind == game::RoundEventKind::frog_hit
                && round.events()[1].slot == 19,
        "Frog hit sound must follow shotgun once at original slot19");
    game::RoundInput repeat;
    repeat.fire_down = true;
    repeat.pointer = {506, 344};
    const auto repeat_random = game::test::after_leaves(game::test::snapshot(state));
    game::test::advance(round, 0, repeat);
    bool queried = false;
    round.resolve_actions([&](const auto& draw, auto) {
        queried = queried || draw.image == "gfx/frog";
        return draw.image == "gfx/frog";
    });
    require(!queried && state.hits == 1 && state.misses == 1 && state.score == 5 && state.random_draws == repeat_random.draws && state.random_state == repeat_random.state,
        "Dying frog cannot score, croak or consume RNG beyond the live leaf pass");
    step(round, .125F);
    require(state.frog.type == 51 && state.frog.frame == 14 && state.frog.world_x == 452.5F && state.frog.world_y == 317.5F,
        "Frog jumps right/up at 100 px/s while death advances 20 cells/s");
    step(round, .3F);
    require(state.frog.type == -1 && state.chickens[19].type == -1, "Death completion releases original frog slot19");
    require(std::ranges::none_of(round.target_draws(2),
                [](const auto& draw) {
                    return draw.sprite.image == "gfx/frog";
                }),
        "Removed frog must no longer draw");

    game::Round special(1, 0);
    fixture(special).camera = 0;
    fixture(special).special_ammo = true;
    hit_frog(special, images, landscape);
    require(special.state().score == 5 && special.state().frog_hits == 1, "Special ammunition does not change the ordinary frog response");
    game::Round full(1, 0);
    fixture(full).camera = 0;
    for (auto& particle : fixture(full).particles) {
        particle.active = true;
    }
    hit_frog(full, images, landscape);
    require(full.state().score == 5 && full.state().frog.type == 51 && full.state().frog_hits == 1, "Full popup pool cannot suppress a frog impact");
}

void selection_and_order(const render::ImageBank& images, const world::Presentation& landscape) {
    for (const auto point : {contracts::ScreenPoint{440, 360}, {560, 360}, {522, 330}, {522, 410}, {522, 360}}) {
        game::Round round(1, 0);
        fixture(round).camera = 0;
        game::RoundInput input;
        input.fire_down = true;
        input.pointer = {point.x - 16, point.y - 16};
        game::test::advance(round, 0, input);
        render::Framebuffer occluded;
        occluded.clear(0x1234);
        bool queried = false;
        round.resolve_actions([&](const auto& draw, auto hit) {
            if (draw.image != "gfx/frog") {
                return false;
            }
            queried = true;
            return occluded.matches_sprite_pixel(images, draw, hit);
        });
        require(round.state().hits == 0 && round.state().misses == 1, "Strict frog edges and a different composed word must reject the target");
        require(queried == (point.x == 522 && point.y == 360), "Exact rectangle edges must reject before pixel sampling");
    }
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 0;
    step(round, 0);
    auto draw = world::frog_draw(state.frog);
    render::Framebuffer black;
    require(images.rgb565_at("gfx/frog", 1, 1) == 0xf81f && black.matches_sprite_pixel(images, draw, {441, 331}), "A keyed candidate pixel retains original scratch-zero equality");
    const auto rendered = commands(round, landscape);
    const auto find = [&](std::string_view name) {
        return std::ranges::find_if(rendered, [name](const auto& item) {
            return item.image == name;
        });
    };
    require(find("gfx/tree1_01") < find("gfx/frog") && find("gfx/frog") < find("gfx/branch1") && find("gfx/branch2") < find("gfx/stone"),
        "Frog must draw in slot19 between tree records and branches/stones");

    state.stones[0].world_x = 440;
    state.stones[0].world_y = 330;
    game::RoundInput input;
    input.fire_down = true;
    input.pointer = {506, 344};
    game::test::advance(round, 0, input);
    round.resolve_actions([](const auto& item, auto) {
        return item.image == "gfx/frog" || item.image == "gfx/stone";
    });
    require(state.frog_hits == 1 && state.stones_hit == 0, "Earlier frog slot19 wins equal-word overlap with stone24");
    state.stones_hit = 8;
    step(round, .3F);
    require(state.frog.type == -1 && state.balloons[19].type == 21 && state.chickens[19].type == -3, "Later special allocation can reuse a frog removed in this update");

    game::Round reused(1, 0);
    auto& reuse = fixture(reused);
    reuse.camera = 0;
    reuse.frog.type = -1;
    reuse.chickens[19] = {.type = 2, .depth = 2, .world_x = 440, .world_y = 330, .points = 5};
    reuse.stones[0].world_x = 440;
    reuse.stones[0].world_y = 330;
    game::test::advance(reused, 0, input);
    const auto ordered = commands(reused, landscape);
    const auto chicken = std::ranges::find_if(ordered, [](const auto& item) {
        return item.image == "gfx/Chickright1";
    });
    const auto branch = std::ranges::find_if(ordered, [](const auto& item) {
        return item.image == "gfx/branch1";
    });
    require(chicken < branch, "A replacement target must retain the frog slot's painter order");
    reused.resolve_actions([](const auto& item, auto) {
        return item.image == "gfx/Chickright1" || item.image == "gfx/stone";
    });
    require(reuse.score == 5 && reuse.stones_hit == 0 && reuse.chickens[19].type == 42, "Reused slot19 must precede fixed stone candidates");
}

void bonus_and_house(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 0;
    state.stones_hit = 4;
    state.remaining = 59;
    hit_frog(round, images, landscape);
    require(state.remaining == 59 && state.frog_hits == 1 && !event(round, game::RoundEventKind::frog_time_bonus), "A shot cannot trigger the earlier pre-input bonus phase");
    step(round, 0);
    require(state.remaining == 64 && state.seconds == 59 && state.frog_hits == 2 && event(round, game::RoundEventKind::frog_time_bonus),
        "Next pre-input phase grants 5 once without recomputing already sampled HUD time");
    step(round, 0);
    require(state.remaining == 64 && state.seconds == 4 && state.minutes == 1 && !event(round, game::RoundEventKind::frog_time_bonus), "Frog counter 2 prevents repeat grant");
    for (const int counter : {1, 2}) {
        game::Round village(1, 0);
        auto& value = fixture(village);
        value.frog.type = -1;
        value.frog_hits = counter;
        value.camera = 320;
        value.village = {.first_church_hit = true, .house_hit = true};
        game::RoundInput input;
        input.fire_down = true;
        input.pointer = {156, 215}; // House source 6,10 at camera 320.
        game::test::advance(village, 0, input);
        render::Framebuffer frame;
        frame.draw(images, commands(village, landscape));
        village.resolve_actions([&](const auto& candidate, auto point) {
            return frame.matches_sprite_pixel(images, candidate, point);
        });
        require(value.hits == 1 && value.score == 0 && value.village.house_points == 25 && !event(village, game::RoundEventKind::church_bell),
            "Both frog counter states suppress the existing house sequence");
    }
    game::Round croak(1, 0);
    auto& value = fixture(croak);
    value.random_state = 0x356ff641U; // Independent CRT inverse gives next value 3.
    step(croak, 0);
    require(event(croak, game::RoundEventKind::frog_croak) && croak.events().front().pan == -1, "Ambient croak must consume one draw and retain prior offscreen position for pan");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected runtime assets");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        const world::Presentation landscape(frames);
        ordinary(images, landscape);
        selection_and_order(images, landscape);
        bonus_and_house(images, landscape);
        std::cout << "Frog source pixels, hit/death/RNG, scene order/reuse, timer and house dependencies passed\n";
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
