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
    state.spawn_accumulator = -10000;
    return state;
}

void tick(game::Round& round, float delta = 0, game::RoundInput input = {}, game::Round::PixelMatch match = {}) {
    game::test::advance(round, round.state().previous_sample + delta, input);
    round.resolve_actions(match);
}

void request(game::RoundState& state, bool airplane) {
    // Independent CRT words: seed 37->159 table31/plane; seed 17->94 table 30/blimp.
    state.random_state = airplane ? 37 : 17;
    state.random_draws = 0;
    state.spawn_accumulator = 51;
}

void allocation() {
    for (bool airplane : {false, true}) {
        game::Round round(1, 0);
        auto& s = fixture(round);
        for (auto& target : s.chickens) {
            target.type = -2;
        }
        request(s, airplane);
        const auto full_random = game::test::after_leaves(game::test::snapshot(s));
        tick(round);
        require(s.random_draws == full_random.draws && s.random_state == full_random.state && !s.airplane_state.active && !s.blimp_progress.active,
            "Full scene pool leaves RNG and aircraft gates untouched");
        s.chickens[70].type = -1;
        s.chickens[90].type = -1;
        request(s, airplane);
        tick(round);
        require(round.events().front().kind == game::RoundEventKind::spawn && round.events().front().slot == 70, "Aircraft uses the first free scene slot");
        const auto spawn_random = game::test::after_leaves({airplane ? 0x755e8d58U : 0x4f8b1054U, 5});
        require(s.random_draws == spawn_random.draws && s.random_state == spawn_random.state, "Source spawn consumes roster plus four ordered draws");
        require(airplane ? s.airplane.type == 18 && s.airplane.world_x == -172 && s.airplane.world_y == 21 && s.airplane.wait == 14
                         : s.blimp.type == 24 && s.blimp.world_x == 617 && s.blimp.world_y == 3,
            "Literal CRT spawn outcome matches original initializer");
        request(s, airplane);
        tick(round);
        const auto gate_random = game::test::after_leaves({airplane ? 37U : 17U, 0}, 1);
        require(s.random_draws == gate_random.draws && s.random_state == gate_random.state && s.chickens[90].type == -1, "Active target gate consumes only the roster draw");
        s.spawn_accumulator = -10000;
        if (airplane) {
            s.airplane.world_x = 2400;
        } else {
            s.blimp.world_x = 2400;
        }
        tick(round, .1F);
        require(s.chickens[70].type == -1 && (airplane ? !s.airplane_state.active : !s.blimp_progress.active), "Exit releases gate and shared scene slot");
        request(s, airplane);
        tick(round);
        require((airplane ? s.airplane_slot : s.blimp_slot) == 70, "A later random spawn reuses the released slot");
        tick(round, 0, {.cancel = true});
        require(s.exit_requested && s.airplane_state.engine_loop == airplane && !s.airplane_state.failure_loop,
            "The final hunting update retains airplane channels for the subsequent exit animation");
        require(std::ranges::none_of(round.events(),
                    [](const auto& event) {
                        return event.kind == game::RoundEventKind::airplane_engine_stop || event.kind == game::RoundEventKind::airplane_failure_stop;
                    }),
            "Requesting results must not release retained channels before the exit animation");
    }
}

std::vector<contracts::SpriteDraw> scene(const game::Round& round, const world::Presentation& presentation) {
    return render::round_scene(round, presentation);
}

void impacts(const render::ImageBank& images, const world::Presentation& presentation) {
    for (bool airplane : {false, true}) {
        for (bool special_ammo : {false, true}) {
            game::Round round(1, 0);
            auto& s = fixture(round);
            s.special_ammo = special_ammo;
            s.score = 100;
            request(s, airplane);
            tick(round);
            s.spawn_accumulator = -10000;
            if (airplane) {
                s.airplane.world_x = 1000;
                s.airplane.world_y = 0;
            } else {
                s.blimp.world_x = 1000;
                s.blimp.world_y = 0;
            }
            tick(round);
            const auto draw = airplane ? *world::airplane_collision(s.airplane) : *world::blimp_collision(s.blimp);
            render::Framebuffer frame;
            frame.draw(images, scene(round, presentation));
            std::optional<contracts::ScreenPoint> pointer;
            // Round clamps the cursor origin before drawing and firing; keep it nonnegative.
            for (int y = std::max(18, draw.destination.y + 2); y < std::min(478, draw.destination.y + draw.source.height - 2) && !pointer; ++y) {
                for (int x = std::max(18, draw.destination.x + 2); x < std::min(638, draw.destination.x + draw.source.width - 2); ++x) {
                    bool visible = true;
                    for (int dy = -2; dy <= 2; ++dy) {
                        for (int dx = -2; dx <= 2; ++dx) {
                            visible = visible && frame.rgb565_at(x + dx, y + dy) != 0 && frame.matches_sprite_pixel(images, draw, {x + dx, y + dy});
                        }
                    }
                    if (visible) {
                        pointer = contracts::ScreenPoint{x - 16, y - 16};
                        break;
                    }
                }
            }
            require(pointer.has_value(), "Original aircraft art offers a visible composed RGB565 aim");
            const int count = airplane ? 4 : 1;
            for (int hit = 1; hit <= count; ++hit) {
                // Release input between shots so recoil settles, then compose
                // the actual pre-input scene just as the production app does.
                if (hit > 1) {
                    tick(round);
                }
                auto aim = *pointer;
                aim.y += static_cast<int>(s.recoil * 0.6F);
                game::test::advance(round, s.previous_sample, {.pointer = aim, .fire_down = true});
                frame.draw(images, scene(round, presentation));
                const auto current = airplane ? *world::airplane_collision(s.airplane) : *world::blimp_collision(s.blimp);
                if (!frame.matches_sprite_pixel(images, current, {aim.x + 16, aim.y + 16})) {
                    throw std::runtime_error("Composed aim lost: family=" + std::to_string(airplane) + " hit=" + std::to_string(hit) + " aim=" + std::to_string(aim.x) + ","
                                             + std::to_string(aim.y) + " pose=" + std::to_string(current.destination.x) + "," + std::to_string(current.destination.y)
                                             + " frame=" + std::to_string(s.airplane.frame) + " recoil=" + std::to_string(s.recoil));
                }
                round.resolve_actions([&](const auto& candidate, auto point) {
                    return frame.matches_sprite_pixel(images, candidate, point);
                });
                if (s.selected_slot != 34 || s.hits != hit || s.misses != 0 || s.ammo_index != 8 + hit) {
                    throw std::runtime_error("Aircraft impact family=" + std::to_string(airplane) + " hit=" + std::to_string(hit) + " selected=" + std::to_string(s.selected_slot)
                                             + " accuracy=" + std::to_string(s.hits) + "/" + std::to_string(s.misses));
                }
                require(s.score == (airplane ? (hit < 3 ? 100 : 75) : 65), "Plane penalizes once on third hit; blimp penalizes once on first hit");
                require(s.multi_hit.write_index == 0, "Neither flight family participates in the multiple-hit tracker");
                if (airplane && hit == 3) {
                    const auto events = round.events();
                    require(events.size() == 5 && events[0].kind == game::RoundEventKind::airplane_engine_pan && events[1].kind == game::RoundEventKind::shot
                                && events[2].kind == game::RoundEventKind::airplane_hit && events[3].kind == game::RoundEventKind::airplane_engine_stop
                                && events[4].kind == game::RoundEventKind::airplane_failure_start,
                        "Third-hit source sound order is pan, gun, impact, motor stop, failure loop");
                }
            }
            require(std::ranges::count_if(s.particles,
                        [](const auto& p) {
                            return p.active && p.type == 2;
                        })
                        == 1,
                "Penalty creates exactly one popup");
            const auto before_render = round.state();
            static_cast<void>(scene(round, presentation));
            require(before_render == round.state(), "Flight rendering cannot change state");
            if (airplane) {
                tick(round, 0, {.cancel = true});
                require(s.exit_requested && !s.airplane_state.engine_loop && s.airplane_state.failure_loop,
                    "Exit retains the damaged plane's failure channel until the app finishes the transition");
                require(std::ranges::none_of(round.events(),
                            [](const auto& event) {
                                return event.kind == game::RoundEventKind::airplane_failure_stop;
                            }),
                    "Failure audio must not stop on the final hunting update");
            } else {
                tick(round, 0, {.pointer = *pointer, .fire_down = true}, [](const auto& candidate, auto) {
                    return candidate.image == "gfx/blimp";
                });
                require(s.score == 65 && s.hits == 1 && s.misses == 1, "Falling blimp rejects a repeat hit");
            }
        }
    }
}

void smoke_and_notice() {
    for (bool airplane : {false, true}) {
        game::Round round(1, 0);
        auto& s = fixture(round);
        request(s, airplane);
        tick(round);
        s.spawn_accumulator = -10000;
        if (airplane) {
            s.airplane_state.hits = 1;
            s.airplane.smoke_time = 1;
        } else {
            s.blimp.type = 64;
            s.blimp.animation_time = 1;
        }
        for (auto& particle : s.particles) {
            particle.active = true;
        }
        s.random_draws = 0;
        const auto full_smoke_random = game::test::after_leaves(game::test::snapshot(s));
        tick(round, .01F);
        require(s.random_draws == full_smoke_random.draws && s.random_state == full_smoke_random.state, "Full shared particle pool suppresses smoke RNG");
        s.particles[7].active = false;
        const auto smoke_random = game::test::after_leaves(game::test::snapshot(s), 2);
        tick(round, .21F);
        require(s.random_draws == smoke_random.draws && s.random_state == smoke_random.state && s.aircraft_smoke[7].type == (airplane ? 5 : 4) && s.particles[7].active,
            "First shared free particle receives smoke and exactly two source draws");
        require(s.aircraft_smoke[7].screen_x != -1024, "New smoke advances in the same update's particle pass");
    }
    game::Round round(1, 0);
    auto& s = fixture(round);
    s.blimp_progress.hits = 2;
    tick(round);
    require(s.blimp_progress.alternate && s.notices[0].type == 102 && s.score == 0, "Two blimps grant only the original notice and alternate artwork latch");
    tick(round);
    require(std::ranges::count_if(s.notices,
                [](const auto& n) {
                    return n.active;
                })
                == 1,
        "Blimp killer notice does not repeat");
    request(s, false);
    tick(round);
    require(s.blimp.frame == 2, "Subsequent rightward blimp uses the original alternate frame");
    const game::Round fresh(1, 0);
    require(fresh.state().airplane.type == -1 && !fresh.state().airplane_state.active && fresh.state().blimp.type == -1 && !fresh.state().blimp_progress.alternate
                && fresh.state().blimp_progress.hits == 0
                && std::ranges::all_of(fresh.state().aircraft_smoke,
                    [](const auto& p) {
                        return p.type == -1;
                    }),
        "New round clears flight gates, counters, variants, channels and smoke");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected runtime asset path");
        allocation();
        smoke_and_notice();
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        impacts(images, world::Presentation(frames));
        std::cout << "Flight allocation/gates/RNG, composed impacts, loop order, shared smoke, notice and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
