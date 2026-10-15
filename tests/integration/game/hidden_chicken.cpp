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
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::fixture;
using moorhuhn2::game::test::require;
using moorhuhn2::game::test::step;

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

void trigger_cases(const render::ImageBank& images, const world::Presentation& landscape) {
    // The original camera1280 composed guard preserves Leaf0 local(30,20)
    // through M2BG01 and the normal cursor. Type35 is distinct from live leaves.
    constexpr contracts::ScreenPoint aim{589, 310};

    struct Case {
        int leaves, spider, branches;
        bool enabled;
    };

    constexpr std::array cases{
        Case{0, 0, 0, false}, Case{9, 6, 0, false}, Case{10, 5, 0, false}, Case{10, 7, 0, false}, Case{10, 6, 1, false}, Case{10, 6, 0, true}, Case{11, 6, 0, true}};
    for (const auto& vector : cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = 1280;
        state.leaves_hit = vector.leaves;
        state.spider_hits = vector.spider;
        state.branches_hit = vector.branches;
        shoot(round, images, landscape, aim);
        require(state.selected_slot == 33 && state.hidden_leaf.type == -1 && state.chickens[33].type == -1 && state.score == 50 && state.hits == 1,
            "The actual partly occluded Leaf0 must award 50, credit one hit and release slot33");
        require(state.hidden_chicken_enabled == vector.enabled && state.leaves_hit == vector.leaves && state.spider_hits == vector.spider && state.branches_hit == vector.branches,
            "The special leaf must test exact existing counters without changing them");
        require(round.events().size() == 2 && round.events()[1].kind == game::RoundEventKind::hidden_leaf_hit, "The special leaf must emit gunshot then the 50-point sound only");
        require(state.particles[0].active && state.particles[0].type == 2 && state.particles[0].points == 50 && state.particles[0].depth == 3 && state.particles[0].world_x == 2360
                    && state.particles[0].world_y == 320,
            "The original special leaf popup is at 2360,320 depth3");
        shoot(round, images, landscape, aim);
        require(
            state.selected_slot == -1 && state.score == 50 && state.hits == 1 && state.misses == 1, "The omitted special leaf exposes a nonmatching repeat without another reward");
        require(state.multi_hit.write_index == 0, "Type35 removal does not enter multiple-hit tracking");
    }
    game::Round empty(1, 0);
    fixture(empty).camera = 1280;
    fixture(empty).ammo_index = 16;
    shoot(empty, images, landscape, aim);
    require(empty.state().hidden_leaf.type == 35 && !empty.state().hidden_chicken_enabled && empty.state().score == 0, "Empty input cannot consume the special leaf");
}

void hidden_capacity_and_order() {
    for (const int particle_free : {0, 1, 32, 33, 68}) {
        for (const int scene_free : {0, 1, 23, 24}) {
            game::Round round(1, 0);
            auto& state = fixture(round);
            state.hidden_chicken_enabled = true;
            state.hidden_chicken.mode = 2;
            state.hidden_chicken.frame = 15;
            for (auto& chicken : state.chickens) {
                chicken.type = -2;
            }
            for (int i = 0; i < scene_free; ++i) {
                state.chickens[static_cast<std::size_t>(34 + i)].type = -1;
            }
            for (std::size_t i = 0; i < state.particles.size(); ++i) {
                state.particles[i].active = i >= static_cast<std::size_t>(particle_free);
                state.particles[i].points = 777;
            }
            const auto random_before = state.random_draws;
            const auto leaf_draws = game::test::leaf_draw_count(state, 1);
            game::test::advance(round, 0, {.pointer = {385, 150}, .fire_down = true});
            round.resolve_actions([](const auto& sprite, auto) {
                return sprite.image == "gfx/hide1";
            });
            require(state.selected_slot == 14 && state.hidden_chicken.type == 74 && state.hidden_chicken.mode == 2 && state.hidden_chicken.frame == 0
                        && state.hidden_chicken.animation_time == 0 && state.score == 250 && state.hits == 1,
                "Enabled selection must award 250 and start 74 without resetting the source mode");
            const auto debris = std::ranges::count_if(state.particles, [](const auto& p) {
                return p.active && p.type == 3;
            });
            const auto balloons = std::ranges::count_if(state.balloons, [](const auto& b) {
                return b.type == 21;
            });
            require(debris == std::min(32, particle_free) && balloons == std::min(24, scene_free), "Source first-free particle and scene pools must saturate independently");
            require(state.random_draws - random_before == leaf_draws + static_cast<std::uint64_t>(1 + 2 * debris + 5 * balloons),
                "Frog and persistent leaves precede the successful debris and balloon allocation draws");
            const auto popups = std::ranges::count_if(state.particles, [](const auto& p) {
                return p.active && p.type == 2 && p.points == 250;
            });
            require(popups == (particle_free > 32 ? 1 : 0), "The score popup is allocated after all 32 debris attempts");
            constexpr std::array expected{game::RoundEventKind::shot, game::RoundEventKind::hidden_chicken_close, game::RoundEventKind::hidden_chicken_mid,
                game::RoundEventKind::hidden_chicken_far, game::RoundEventKind::hidden_chicken_balloons};
            require(round.events().size() == expected.size(), "No extra hidden-chicken impact event is permitted");
            for (std::size_t i = 0; i < expected.size(); ++i) {
                require(round.events()[i].kind == expected[i], "Hidden-chicken sounds must retain original close/mid/far/magic order");
            }
            for (const auto& particle : state.particles) {
                if (particle.active && particle.type == 3) {
                    require(particle.world_x == 1424 && particle.world_y == 163 && particle.depth == 2 && particle.frame == 0,
                        "Hidden debris must use the original record center, depth2 and particle2 cell 0");
                }
            }
            step(round);
            require(state.multi_hit.write_index == 0 && state.chickens[14].type == -2, "Reaction74 retains its slot and never registers multiple-hit");
            state.spawn_accumulator = 0;
            for (int i = 0; i < 12; ++i) {
                step(round, .1F);
            }
            require(state.hidden_chicken.type == -1 && state.chickens[14].type == -1, "Finished hide3 must free the hidden chicken's actual slot");
        }
    }
}

void hidden_composed_hits(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round disabled(1, 0);
    auto& state = fixture(disabled);
    state.hidden_chicken.mode = 3;
    state.hidden_chicken.world_x = 1395;
    shoot(disabled, images, landscape, {394, 151});
    require(state.selected_slot == 14 && state.hidden_chicken.type == 34 && state.score == 0 && state.hits == 1 && disabled.events().size() == 1,
        "The real hide2 frame0 source aim credits a disabled hit without damage, points or impact audio");

    game::Round enabled(1, 0);
    auto& live = fixture(enabled);
    live.hidden_chicken_enabled = true;
    live.hidden_chicken.mode = 2;
    live.hidden_chicken.frame = 15;
    shoot(enabled, images, landscape, {384, 157});
    require(live.selected_slot == 14 && live.hidden_chicken.type == 74 && live.score == 250 && live.hits == 1,
        "The real hide1 frame15 source aim must reach the enabled impact through actual foreground and cursor");
    require(live.particles[32].type == 2 && live.particles[32].points == 250 && live.particles[32].depth == 2 && live.particles[32].world_x == 1404
                && live.particles[32].world_y == 163,
        "The hidden popup follows 32 source debris particles and retains the original asymmetric x+10 placement");
    shoot(enabled, images, landscape, {384, 157});
    require(live.selected_slot != 14 && live.score == 250 && live.hits == 1 && live.misses == 1, "Reaction74 art rejects a repeated hit");

    for (const auto edge : {contracts::ScreenPoint{371, 150}, contracts::ScreenPoint{432, 150}, contracts::ScreenPoint{394, 105}, contracts::ScreenPoint{394, 222},
             contracts::ScreenPoint{440, 151}}) {
        game::Round bounded(1, 0);
        fixture(bounded).hidden_chicken.mode = 3;
        fixture(bounded).hidden_chicken.world_x = 1395;
        bool queried = false;
        game::test::advance(bounded, 0, {.pointer = {edge.x - 16, edge.y - 16}, .fire_down = true});
        bounded.resolve_actions([&](const auto& sprite, auto) {
            queried |= sprite.image == "gfx/hide2";
            return false;
        });
        require(!queried, "Hide2 keeps strict 61x117 collision bounds despite its 78x121 visible cell");
    }
}

void hidden_scratch_and_reuse() {
    game::Round round(1, 0);
    auto& state = fixture(round);
    game::test::advance(round, 0, {.pointer = {385, 150}, .fire_down = true});
    round.resolve_actions([](const auto& draw, auto) {
        return draw.image.empty() && draw.source.width == 61;
    });
    require(state.selected_slot == 14 && state.hidden_chicken.type == 34 && state.hidden_chicken.mode == 0 && state.score == 0 && state.hits == 1,
        "A cleared-black scratch match can credit an invisible disabled chicken without damage");
    state.hidden_chicken_enabled = true;
    state.recoil = 0;
    game::test::advance(round, 0, {.pointer = {385, 150}, .fire_down = true});
    round.resolve_actions([](const auto& draw, auto) {
        return draw.image.empty() && draw.source.width == 61;
    });
    require(state.hidden_chicken.type == 74 && state.score == 250 && state.hits == 2, "The enabled handler also preserves the original invisible-black selection case");
    for (int i = 0; i < 12; ++i) {
        step(round, .1F);
    }
    require(state.hidden_chicken.type == -1, "Death must complete before slot-reuse check");
    state.balloons[14] = world::make_balloon(640, {0, 1, 2, 3, 4});
    state.chickens[14].type = -3;
    const auto draws = round.target_draws(3);
    require(std::ranges::any_of(draws,
                [](const auto& draw) {
                    return draw.scene_slot == 14 && draw.sprite.image == "gfx/ballon";
                }),
        "A later balloon can use the freed hidden-chicken scene slot");
    const game::Round fresh(1, 0);
    require(fresh.state().hidden_chicken == world::HiddenChicken{} && fresh.state().hidden_leaf == world::HiddenLeaf{} && !fresh.state().hidden_chicken_enabled,
        "A new round restores both objects, wait, animation and the prerequisite latch");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected adopted runtime assets");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        const world::Presentation landscape(frames);
        trigger_cases(images, landscape);
        hidden_composed_hits(images, landscape);
        hidden_capacity_and_order();
        hidden_scratch_and_reuse();
        std::cout << "Hidden trigger prerequisites, composed pixels, pools/RNG/order, scratch selection, removal/reuse and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
