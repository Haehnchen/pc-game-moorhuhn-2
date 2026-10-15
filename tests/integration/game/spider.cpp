#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "render/framebuffer.hpp"
#include "render/round_scene.hpp"
#include "support.hpp"
#include "support/require.hpp"
#include "ui/screens.hpp"
#include "world/presentation.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::event;
using moorhuhn2::game::test::fixture;
using moorhuhn2::game::test::step;

using moorhuhn2::test::require;

void reveal(game::Round& round) {
    auto& spider = fixture(round).spider;
    spider.mode = 0;
    spider.countdown = 0;
    spider.frame = 0;
    spider.animation_time = 0;
    step(round);
}

std::vector<contracts::SpriteDraw> scene(const game::Round& round, const world::Presentation& landscape) {
    return render::round_scene(round, landscape);
}

void shoot(game::Round& round, const render::ImageBank& images, const world::Presentation& landscape, bool isolated = false) {
    auto& state = fixture(round);
    // Unit fixtures isolate the shared input/collision contract from magazine
    // capacity; the native route exercises actual empty/reload between hits.
    state.ammo_index = 8;
    game::RoundInput input;
    input.fire_down = true;
    input.pointer = {206, 156}; // Original body 22,176 at world 1224,-4/camera640.
    game::test::advance(round, state.previous_sample, input);
    render::Framebuffer frame;
    if (isolated) {
        const auto draw = world::spider_draw(state.spider);
        frame.draw(images, std::span(&draw, 1));
    } else {
        auto draws = scene(round, landscape);
        frame.draw(images, draws);
    }
    round.resolve_actions([&](const auto& candidate, auto point) {
        return (!isolated || candidate.image == "gfx/spinne1") && frame.matches_sprite_pixel(images, candidate, point);
    });
}

void ordinary(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    require(state.spider.type == 20 && state.spider.frame == 8 && state.spider.countdown == 5 && state.spider_hits == 0 && !state.spider_hero,
        "Original spider must begin hidden/revealing with cleared counter/latch");
    const auto random = state.random_draws;
    const auto leaf_draws = game::test::leaf_draw_count(state, 1);
    shoot(round, images, landscape);
    require(state.hits == 0 && state.misses == 1 && state.spider.type == 20 && state.score == 0, "Initial invisible spinne2 cell cannot substitute for spinne1 collision words");
    require(state.random_draws == random + 1 + leaf_draws, "Spider setup/update/hit selection consumes no RNG after frog and leaves");
    reveal(round);
    const auto draws = scene(round, landscape);
    const auto spider = std::ranges::find_if(draws, [](const auto& draw) {
        return draw.image == "gfx/spinne1";
    });
    const auto tree = std::ranges::find_if(draws, [](const auto& draw) {
        return draw.image == "gfx/tree1_01";
    });
    require(spider < tree, "Spider slot5 must precede original tree slots 15..18 at depth2");
    shoot(round, images, landscape);
    require(state.score == 50 && state.spider_hits == 1 && state.hits == 1 && state.spider.type == 60 && state.spider.frame == 0,
        "Actual composed/cursor-transparent spider body must give 50 and retract");
    require(state.particles[0].points == 50 && state.particles[0].depth == 2 && state.particles[0].world_x == 1229 && state.particles[0].world_y == 96,
        "Original spider 50 popup location differs");
    require(event(round, game::RoundEventKind::spider_hit), "Every ordinary spider hit must play its swing sample");
    shoot(round, images, landscape);
    require(state.score == 50 && state.spider_hits == 1 && state.hits == 1 && state.misses == 2 && !event(round, game::RoundEventKind::spider_hit),
        "Reaction type60 must reject another spider impact immediately");
    for (int i = 0; i < 25; ++i) {
        step(round, .125F);
    }
    require(state.spider.type == 20 && state.spider.frame == 0 && state.chickens[5].type == -2, "Normal reaction returns after 25 single-cell advances and keeps slot5");
    shoot(round, images, landscape, true);
    require(state.score == 50 && state.spider_hits == 2 && state.hits == 2 && event(round, game::RoundEventKind::spider_hit),
        "Second ordinary hit credits accuracy/sound without points");
}

void sequence(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    reveal(round);
    for (int hit = 1; hit <= 26; ++hit) {
        const int before = state.score;
        shoot(round, images, landscape, true);
        const int points = hit == 1 ? 50 : hit == 3 ? 100 : hit == 25 ? 200 : 0;
        require(state.score == before + points, "Original spider award differs at hit " + std::to_string(hit) + ": score=" + std::to_string(state.score)
                                                    + " before=" + std::to_string(before) + " counter=" + std::to_string(state.spider_hits)
                                                    + " type=" + std::to_string(state.spider.type) + " frame=" + std::to_string(state.spider.frame));
        const auto rng = state.random_draws;
        const auto leaf_draws = game::test::leaf_draw_count(state, 1);
        step(round);
        if (hit == 3) {
            require(state.spider_hits == 4 && !state.spider_hero && !state.special_ammo && state.random_draws == rng + 81 + leaf_draws,
                "Third hit's next update consumes frog/leaves, allocates 16 balloons, then bumps counter 3 to 4");
            require(event(round, game::RoundEventKind::spider_balloons), "First spider grant must play magic once");
        } else if (hit == 25) {
            require(state.spider_hits == 26 && state.spider_hero && state.score == 450 && state.random_draws == rng + 101 + leaf_draws && !state.special_ammo,
                "25th ordinary hit gives 200, then hero gives 100/20 balloons with unchanged ammo mode");
            require(state.notices[0].active && state.notices[0].age == 0 && state.notices[0].y == 180, "Hero allocates notice101 independently of score particles");
            require(event(round, game::RoundEventKind::spider_hero), "Hero score sound must occur once");
        } else {
            require(!event(round, game::RoundEventKind::spider_balloons) && !event(round, game::RoundEventKind::spider_hero), "Spider grants cannot repeat on other counters");
        }
        for (int i = 0; i < 25; ++i) {
            step(round, .125F);
        }
    }
    require(state.score == 450 && state.hits == 26 && state.spider_hits == 27, "Complete ordinary chain totals 450 and 26 accuracy hits");
    const auto before_render = round.state();
    static_cast<void>(scene(round, landscape));
    static_cast<void>(scene(round, landscape));
    require(round.state() == before_render, "Drawing must not advance spider or blinking notice state");
}

void collision_boundaries(const render::ImageBank& images) {
    for (const auto point : {contracts::ScreenPoint{200, 172}, {250, 172}, {222, 196}, {222, 172}}) {
        game::Round round(1, 0);
        reveal(round);
        game::test::advance(round, 0, {.pointer = {point.x - 16, point.y - 16}, .fire_down = true});
        render::Framebuffer occluded;
        occluded.clear(0x1234);
        bool queried = false;
        round.resolve_actions([&](const auto& draw, auto hit) {
            if (draw.image != "gfx/spinne1") {
                return false;
            }
            queried = true;
            return occluded.matches_sprite_pixel(images, draw, hit);
        });
        require(round.state().hits == 0 && round.state().misses == 1 && round.state().spider.type == 20, "Spider edges and a different composed word must reject the target");
        require(queried == (point.x == 222 && point.y == 172), "Strict spider rectangle edges reject before pixel sampling");
    }

    game::Round keyed(1, 0);
    reveal(keyed);
    game::test::advance(keyed, 0, {.pointer = {185, 4}, .fire_down = true}); // Screen 201,20, local 1,24.
    render::Framebuffer black;
    require(images.rgb565_at("gfx/spinne1", 1, 24) == 0xf81f, "Original spider scratch-zero test needs a keyed source word");
    keyed.resolve_actions([&](const auto& draw, auto point) {
        return draw.image == "gfx/spinne1" && black.matches_sprite_pixel(images, draw, point);
    });
    require(keyed.state().hits == 1 && keyed.state().score == 50, "Original keyed candidate pixels can hit an equal black composed word; opacity is not a hit mask");
}

void pools_and_special(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round special(1, 0);
    auto& state = fixture(special);
    reveal(special);
    state.special_ammo = true;
    state.spider_hits = 4;
    state.spider.points = 100;
    const auto rng = state.random_draws;
    const auto leaf_draws = game::test::leaf_draw_count(state, 1);
    shoot(special, images, landscape);
    require(state.score == 200 && state.spider_hits == 4 && state.spider.points == 200 && state.spider.countdown == 1 && state.random_draws == rng + 1 + leaf_draws,
        "Special ammo doubles the stored award without spider counter/RNG changes");
    require(!event(special, game::RoundEventKind::spider_hit), "Special spider impact must not play ordinary swing sound");
    for (int i = 0; i < 7; ++i) {
        step(special, .125F);
    }
    require(state.spider.type == 60 && state.spider.frame == 7, "Special reaction stays through its eighth stored cell");
    state.stones_hit = 8;
    step(special, .125F);
    require(state.spider.type == -1 && state.balloons[5].type == 21 && state.chickens[5].type == -3, "Special removal releases slot5 before later same-update balloon allocation");
    require(std::ranges::none_of(special.target_draws(2),
                [](const auto& draw) {
                    return draw.sprite.image.starts_with("gfx/spinne");
                }),
        "Removed spider must not remain in presentation");

    game::Round full(1, 0);
    auto& filled = fixture(full);
    filled.spider_hits = 26;
    filled.frog.type = -1;
    for (auto& chicken : filled.chickens) {
        chicken.type = -2;
    }
    for (auto& notice : filled.notices) {
        notice = {true, false, 0, 250};
    }
    const auto before = game::test::after_leaves(game::test::snapshot(filled));
    step(full, .125F);
    require(filled.score == 100 && filled.spider_hero && filled.random_draws == before.draws && filled.random_state == before.state && filled.notices[0].y == 245,
        "Exhausted balloon/notice pools cannot cancel hero points/latch/sounds or consume spawn RNG");
    const auto events = full.events();
    require(events.size() == 2 && events[0].kind == game::RoundEventKind::spider_balloons && events[1].kind == game::RoundEventKind::spider_hero,
        "Hero magic must precede score sound, even with no free pools");
    step(full);
    require(filled.score == 100 && !event(full, game::RoundEventKind::spider_hero), "Hero latch prevents a repeat grant");

    game::Round simultaneous(1, 0);
    auto& both = fixture(simultaneous);
    both.stones_hit = 8;
    both.spider_hits = 3;
    const auto simultaneous_leaves = game::test::leaf_draw_count(both, 1);
    step(simultaneous);
    require(both.special_ammo && both.spider_hits == 4 && both.random_draws == 337 + simultaneous_leaves,
        "Eight-stone grant precedes spider 16 allocation; both source grants can share the update");
    require(simultaneous.events().size() == 2 && simultaneous.events()[0].kind == game::RoundEventKind::special_ammo
                && simultaneous.events()[1].kind == game::RoundEventKind::spider_balloons,
        "Stone magic event must precede spider magic event");

    game::Round fresh(1, 0);
    require(fresh.state().spider == world::Spider{} && fresh.state().spider_hits == 0 && !fresh.state().spider_hero
                && std::ranges::none_of(fresh.state().notices,
                    [](const auto& notice) {
                        return notice.active || notice.visible;
                    }),
        "New round resets spider, counters, notice pool and hero latch");
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
        sequence(images, landscape);
        collision_boundaries(images);
        pools_and_special(images, landscape);
        std::cout << "Spider actual-pixel selection, return/removal, source awards/bonuses, pool ordering, audio and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
