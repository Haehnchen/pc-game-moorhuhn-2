#include "game/round.hpp"

#include "persistent_leaves_support.hpp"
#include "round_clock.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace moorhuhn2;

using moorhuhn2::test::require;

void step(game::Round& round, float time, game::RoundInput input = {}) {
    game::test::advance(round, time, input);
    round.resolve_actions({});
}

void seeded_setup() {
    game::Round round(1.9F, 0);
    require(round.state().random_draws == 176, "MH2 initial ammo/leaves/stones draw count");
    require(round.state().ammo[0].velocity_x == 1.25F && round.state().ammo[1].velocity_x == 1.5F, "MSVCRT first ammo random values for truncated seed 1");
    require(round.state().ammo[0].velocity_y == -1.0F, "first ammo vertical velocity");
    step(round, 1);
    require(round.events().empty(), "spawn comparison is strictly greater than 50");
    game::test::advance(round, 1.125F, {});
    const auto& chicken = round.state().chickens[34];
    require(chicken.type == -7 && round.state().airplane_slot == 34 && round.state().airplane_state.active,
        "After the prior frog and leaf updates, spawn table 31 allocates the airplane in the first free scene slot");
    require(round.state().random_draws == 248 && round.state().random_state == 0x5968ad99U, "Source CRT boundary after two complete scene/particle updates");
    const auto before = round.state();
    std::vector<contracts::DepthDraw> appended{{0, {"prefix", {}, {}}}};
    round.append_target_draws(4, appended);
    require(appended.size() == 2 && appended.front().sprite.image == "prefix" && appended.back().scene_slot == 34 && appended.back().sprite.image == "gfx/rplane",
        "Appending target draws must retain the caller's prefix and scene-pool order");
    for (int index = 0; index < 20; ++index) {
        static_cast<void>(round.target_draws(4));
        static_cast<void>(round.ammo_draws());
    }
    require(round.state() == before, "render submission cannot change simulation");
    round.resolve_actions({});

    // The first spawn precedes particle updates, so accepted first-spawn poses
    // retain their literal values. Extend each old scene RNG boundary with the
    // independently predicted ordered leaf pass.
    struct SeedVector {
        int seed;
        int type;
        int depth;
        float x;
        float y;
        int points;
        std::uint32_t random_state;
        std::uint64_t draws;
    };

    constexpr SeedVector vectors[]{{0, 1, 4, 39, 215, 25, 0xc99857f4U, 180}, {1, 1, 4, 924, 215, 25, 0x5fa48405U, 180}, {6, 1, 3, 482, 310, 10, 0x4de1605aU, 180},
        {10, 2, 3, 772, 76, 10, 0xf5125408U, 182}, {12345, 1, 2, 485, 410, 5, 0x7a9157bdU, 180}};
    for (const auto& expected : vectors) {
        game::Round seeded(static_cast<float>(expected.seed), 0);
        step(seeded, 1.125F);
        const auto& target = seeded.state().chickens[34];
        require(target.type == expected.type && target.depth == expected.depth && target.world_x == expected.x && target.world_y == expected.y && target.points == expected.points,
            "five original seeded chicken/depth/position/score vectors");
        const auto random = game::test::after_leaves({expected.random_state, expected.draws});
        require(seeded.state().random_state == random.state && seeded.state().random_draws == random.draws, "five source seeded spawn/scene/particle RNG boundaries");
    }
}

void camera_and_clock() {
    game::Round round(0, 0);
    game::RoundInput input;
    input.pointer.x = 576;
    input.held_right = true;
    step(round, .25F, input);
    require(round.state().camera == 740, "pointer edge and held arrow each contribute 200 px/sec");
    input.pointer.x = 575;
    input.held_right = false;
    step(round, .5F, input);
    require(round.state().camera == 740, "pointer x 575 is inside neutral camera region");
    input.pointer.x = 0;
    input.held_left = true;
    step(round, 5, input);
    require(round.state().camera == 0, "ordered contributions clamp at camera 0");
    input.pointer.x = 320;
    input.held_right = true;
    step(round, 5.25F, input);
    require(round.state().camera == 50, "at 0 left is skipped before right guard");
    const float remaining = round.state().remaining;
    round.reset_sample(100);
    step(round, 100.125F);
    require(round.state().remaining == remaining - .125F, "boss/focus reset prevents catch-up timer delta");
    game::Round expiry(0, 0);
    step(expiry, 87.5F);
    step(expiry, 89.5F);
    require(expiry.state().exit_requested && expiry.state().remaining == .5F, "zero-second bucket expires before remaining 0");
    game::Round skipped(0, 0);
    step(skipped, 92);
    require(!skipped.state().exit_requested && skipped.state().seconds == -2, "large delta skipping 0 does not invent direct remaining <= 0 expiry");
    bool rejected = false;
    try {
        step(round, 99);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "backward samples rejected");
}

void ammunition() {
    game::Round round(1, 0);
    auto random = game::test::snapshot(round.state());
    game::RoundInput input;
    input.fire_down = true;
    input.reload_down = true;
    step(round, 0, input);
    require(round.state().ammo_index == 9 && round.state().misses == 1, "partial reload ignored after fire");
    input.reload_down = false;
    for (int index = 0; index < 6; ++index) {
        step(round, 0, input);
    }
    require(round.state().ammo_index == 15 && round.state().misses == 7, "seven fired shots");
    input.reload_down = true;
    step(round, 0, input);
    require(round.state().ammo_index == 8 && round.state().misses == 8, "last shot and reload in one consumed frame");
    require(
        round.events().size() == 2 && round.events()[0].kind == game::RoundEventKind::shot && round.events()[1].kind == game::RoundEventKind::reload, "fire event precedes reload");
    for (int update = 0; update < 8; ++update) {
        random = game::test::after_leaves(random, 1);
    }
    random.discard(16);
    require(round.state().random_draws == random.draws && round.state().random_state == random.state, "Eight live frog/leaf updates precede sixteen reload draws");
    input.reload_down = false;
    for (int index = 0; index < 8; ++index) {
        step(round, 0, input);
    }
    step(round, 0, input);
    require(round.state().misses == 16 && round.events().size() == 1 && round.events()[0].kind == game::RoundEventKind::empty, "empty fire does not count as shot or miss");
}

void hit_and_death() {
    game::Round round(1, 0);
    step(round, 1.125F);
    step(round, 1.125F);
    auto draw = game::chicken_draw(round.state().chickens[34]);
    game::RoundInput input;
    input.pointer = {draw.destination.x - 16, draw.destination.y};
    input.fire_down = true;
    game::test::advance(round, 1.125F, input);
    int callbacks = 0;
    round.resolve_actions([&](const auto&, auto) {
        ++callbacks;
        return true;
    });
    require(callbacks == 0 && round.state().score == 0, "strict left edge excluded before framebuffer comparison");
    input.pointer = {draw.destination.x + 5, draw.destination.y + 2};
    game::test::advance(round, 1.125F, input);
    round.resolve_actions([](const auto&, auto) {
        return false;
    });
    require(round.state().score == 0, "occluded RGB565 pixel fails candidate selection");
    game::test::advance(round, 1.125F, input);
    round.resolve_actions([](const auto&, auto) {
        return true;
    });
    require(round.state().score == 25 && round.state().hits == 1 && round.state().chickens[34].type == 41, "far ordinary hit scores 25 and changes 1 to 41");
    require(round.events().size() == 2 && round.events()[0].kind == game::RoundEventKind::shot && round.events()[1].kind == game::RoundEventKind::chicken_hit,
        "shot event before chicken hit");
    draw = game::chicken_draw(round.state().chickens[34]);
    require(draw.image == "gfx/DChickright3" && draw.source.height == 31, "correct far-right death sheet");
    game::test::advance(round, 1.125F, input);
    round.resolve_actions([](const auto&, auto) {
        return true;
    });
    require(round.state().score == 25 && round.state().hits == 1, "dying chicken ineligible for repeated hit");
    step(round, 4.125F);
    require(round.state().chickens[34].type == -1, "far circular death removes when world y exceeds 300");
    game::Round cancel(1, 0);
    input.cancel = true;
    step(cancel, 0, input);
    require(cancel.state().exit_requested && cancel.state().ammo_index == 9, "Escape still finishes hunting fire in same update");
}

} // namespace

int main() {
    try {
        seeded_setup();
        camera_and_clock();
        ammunition();
        hit_and_death();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "ordinary round source vectors, inputs, hit/death, timer passed\n";
}
