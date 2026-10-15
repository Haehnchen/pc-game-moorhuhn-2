#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "render/framebuffer.hpp"
#include "render/round_scene.hpp"
#include "support.hpp"
#include "ui/screens.hpp"
#include "world/presentation.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::count_event;
using moorhuhn2::game::test::fixture;
using moorhuhn2::game::test::require;

void step(game::Round& round, float delta = 0) {
    fixture(round).spawn_accumulator = 0;
    game::test::advance(round, round.state().previous_sample + delta, {});
    round.resolve_actions({});
}

std::vector<contracts::SpriteDraw> scene(const game::Round& round, const world::Presentation& landscape) {
    return render::round_scene(round, landscape);
}

void shoot(game::Round& round, const render::ImageBank& images, const world::Presentation& landscape, contracts::ScreenPoint hotspot) {
    auto& state = fixture(round);
    state.recoil = 0;
    const contracts::ScreenPoint pointer{hotspot.x - 16, hotspot.y - 16};
    game::test::advance(round, state.previous_sample, {.pointer = pointer, .fire_down = true});
    auto draws = scene(round, landscape);
    render::Framebuffer frame;
    frame.draw(images, draws);
    round.resolve_actions([&](const auto& sprite, auto point) {
        return frame.matches_sprite_pixel(images, sprite, point);
    });
}

void settle(game::Round& round, int expected_type) {
    for (int i = 0; i < 32 && round.state().hunting_stand.type != expected_type; ++i) {
        step(round, .125F);
    }
    require(round.state().hunting_stand.type == expected_type, "Original stand reaction must reach its next live stage");
    for (int i = 0; i < 20; ++i) {
        step(round, .125F);
    }
}

// A02 original-word guards at camera0/recoil0: mask cells 0/1/2. These are
// hotspot positions; shoot() applies the original cursor's -16 input offset.
constexpr contracts::ScreenPoint initial_aim{302, 190};
constexpr contracts::ScreenPoint ladder_aim{313, 216};
constexpr contracts::ScreenPoint body_aim{297, 203};

void complete_route(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 0;
    const auto random_before = state.random_draws;
    const auto leaf_draws = game::test::leaf_draw_count(state, 1);
    shoot(round, images, landscape, initial_aim);
    require(state.selected_slot == 4 && state.hits == 1 && state.score == 25 && state.hunting_stand.type == 53,
        "Original visible mask0 must award 25 and start the first reaction in slot4");
    require(state.random_draws == random_before + 1 + leaf_draws, "Stand impact consumes no RNG after the frog and persistent leaves");
    require(count_event(round, game::RoundEventKind::hunting_stand_chicken) == 1 && round.events().size() == 2, "First stand hit must emit only shot and far chicken impact");
    require(state.particles[0].active && state.particles[0].points == 25 && state.particles[0].world_x == 292 && state.particles[0].world_y == 208 && state.particles[0].depth == 3
                && state.particles[0].age == 0,
        "Original stand popup must be allocated at 292,208 without advancing during input");
    shoot(round, images, landscape, initial_aim);
    require(state.selected_slot == -1 && state.misses == 1 && state.score == 25 && state.hunting_stand.type == 53, "Reaction53 cannot be selected by a repeated shot");
    settle(round, 14);
    for (int i = 0; i < 2; ++i) {
        shoot(round, images, landscape, ladder_aim);
        require(state.selected_slot == 4 && state.hunting_stand.type == 14 && state.hunting_stand.ladder_special_hits == 0 && state.score == 25,
            "Ordinary ladder impacts credit accuracy without damaging the ladder");
        require(count_event(round, game::RoundEventKind::hunting_stand_hit) == 1 && round.events().size() == 2, "Ordinary ladder impact must use only the wood sample");
    }
    state.special_ammo = true; // The actual eight-stone grant is covered by the native route.
    shoot(round, images, landscape, ladder_aim);
    require(state.hunting_stand.ladder_special_hits == 1 && state.hunting_stand.type == 14 && state.score == 25, "First special ladder hit cannot complete the stage");
    shoot(round, images, landscape, ladder_aim);
    require(state.hunting_stand.ladder_special_hits == 2 && state.hunting_stand.type == 54 && state.score == 50,
        "Exactly the second special ladder hit awards 25 and starts reaction54");
    require(round.events().size() == 3 && round.events()[1].kind == game::RoundEventKind::hunting_stand_hit && round.events()[2].kind == game::RoundEventKind::hunting_stand_ladder,
        "Ladder destruction must preserve shot, wood, ladder sound order");
    settle(round, 15);
    state.special_ammo = false;
    shoot(round, images, landscape, body_aim);
    require(state.selected_slot == 4 && state.hunting_stand.stand_special_hits == 0 && state.hunting_stand.type == 15 && state.score == 50,
        "Ordinary final-body hit cannot increment special damage");
    state.special_ammo = true;
    for (int hit = 1; hit <= 3; ++hit) {
        if (state.ammo_index == 16) {
            game::test::advance(round, state.previous_sample, {.reload_down = true});
            round.resolve_actions({});
            require(state.ammo_index == 8 && state.hunting_stand.stand_special_hits == hit - 1 && state.special_ammo,
                "Normal empty reload must preserve stand damage and ammunition mode");
        }
        shoot(round, images, landscape, body_aim);
        require(
            state.selected_slot == 4 && state.hunting_stand.stand_special_hits == hit && state.score == (hit == 3 ? 75 : 50) && state.hunting_stand.type == (hit == 3 ? 55 : 15),
            "Final body must require exactly three special hits");
    }
    require(round.events().size() == 3 && round.events()[1].kind == game::RoundEventKind::hunting_stand_hit && round.events()[2].kind == game::RoundEventKind::hunting_stand_broken,
        "Final destruction must preserve shot, wood, breaks-down sound order");
    for (int i = 0; i < 24; ++i) {
        step(round, .125F);
    }
    require(state.hunting_stand.type == 55 && state.hunting_stand.frame == 9 && state.chickens[4].type == -2,
        "Destroyed stand clamps frame9 and permanently reserves its original scene slot");
    require(state.hits == 9 && state.misses == 1 && state.multi_hit.write_index == 0 && state.score == 75,
        "All live hits credit accuracy; stand reactions cannot enter the multiple-hit tracker");
    shoot(round, images, landscape, body_aim);
    require(state.selected_slot == -1 && state.misses == 2 && state.score == 75 && !count_event(round, game::RoundEventKind::hunting_stand_hit),
        "Retained terminal art is not a repeat target");
    const game::Round fresh(1, 0);
    require(fresh.state().hunting_stand == world::initial_hunting_stand() && fresh.state().chickens[4].type == -2,
        "A new round restores stage, animation and both special-hit counters");
}

void boundaries(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round cover(1, 0);
    fixture(cover).camera = 1280;
    shoot(cover, images, landscape, {431, 190});
    require(cover.state().selected_slot != 4 && cover.state().hunting_stand.type == 13 && cover.state().score == 0,
        "The decorative right cover must never create an independent stand target");
    game::Round outside(1, 0);
    fixture(outside).camera = 0;
    shoot(outside, images, landscape, {352, 208});
    require(outside.state().selected_slot == -1 && outside.state().score == 0, "Original nonmatching mask position must miss");
    for (const auto edge : {contracts::ScreenPoint{252, 190}, contracts::ScreenPoint{372, 190}, contracts::ScreenPoint{302, 158}, contracts::ScreenPoint{302, 258}}) {
        game::Round round(1, 0);
        fixture(round).camera = 0;
        bool queried = false;
        game::test::advance(round, 0, {.pointer = {edge.x - 16, edge.y - 16}, .fire_down = true});
        round.resolve_actions([&](const auto& sprite, auto) {
            queried |= sprite.image == "gfx/hochsitzcol";
            return false;
        });
        require(!queried && round.state().misses == 1, "Stand rectangle boundaries must be strict before scratch comparison");
    }
    game::Round full(1, 0);
    auto& state = fixture(full);
    state.camera = 0;
    for (auto& particle : state.particles) {
        particle.active = true;
        particle.points = 777;
    }
    shoot(full, images, landscape, initial_aim);
    require(state.score == 25 && state.hunting_stand.type == 53
                && std::ranges::all_of(state.particles,
                    [](const auto& particle) {
                        return particle.active && particle.points == 777;
                    }),
        "Full popup pool must not block points, reaction or overwrite a particle");
    game::Round empty(1, 0);
    fixture(empty).camera = 0;
    fixture(empty).ammo_index = 16;
    shoot(empty, images, landscape, initial_aim);
    require(empty.state().hunting_stand.type == 13 && empty.state().hits == 0 && empty.state().misses == 0 && count_event(empty, game::RoundEventKind::empty) == 1,
        "Empty fire must leave the stand and accuracy unchanged");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected adopted assets");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        const world::Presentation landscape(frames);
        complete_route(images, landscape);
        boundaries(images, landscape);
        game::Round round(1, 0);
        fixture(round).camera = 0;
        step(round);
        const auto before_render = round.state();
        const auto draws = scene(round, landscape);
        require(before_render == round.state(), "Stand rendering must not change gameplay state");
        require(std::ranges::count(draws, std::string_view{"gfx/hochsitz3"}, &contracts::SpriteDraw::image) == 1
                    && std::ranges::count(draws, std::string_view{"gfx/hochsitz4"}, &contracts::SpriteDraw::image) == 1,
            "One scene record must draw both the initial stand and its remote cover");
        std::cout << "Original stand masks, full damage route, popup shortage, sound order, reload, terminal state and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
