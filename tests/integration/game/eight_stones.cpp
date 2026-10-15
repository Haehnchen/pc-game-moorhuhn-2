#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "game/round.hpp"
#include "support.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::fixture;

using moorhuhn2::test::require;

// Synthetic prerequisites exercise pool pressure and rare branches. The regular
// eight-shot route below and the native scenario use only normal round inputs.

void step(game::Round& round, float time, game::RoundInput input = {}) {
    game::test::advance(round, time, input);
    round.resolve_actions({});
}

bool has_event(const game::Round& round, game::RoundEventKind kind) {
    return std::ranges::any_of(round.events(), [kind](const auto& event) {
        return event.kind == kind;
    });
}

void check_random(const game::Round& round, const game::test::SourceRandom& expected, std::string_view message) {
    require(round.state().random_draws == expected.draws && round.state().random_state == expected.state, message);
}

void eight_real_stones() {
    game::Round round(1, 0);
    for (std::size_t index = 0; index < 8; ++index) {
        const auto& stone = round.state().stones[index];
        const float desired = std::clamp((stone.world_x + 73 - 320) / 1.6F, 0.0F, 1280.0F);
        game::RoundInput move;
        move.held_left = desired < round.state().camera;
        move.held_right = !move.held_left;
        step(round, round.state().previous_sample + std::abs(desired - round.state().camera) / 200.0F, move);
        step(round, round.state().previous_sample);
        const auto draw = world::stone_draw(stone);
        game::RoundInput fire;
        fire.fire_down = true;
        fire.pointer = {draw.destination.x + 73 - 16, draw.destination.y + 55 - 16};
        game::test::advance(round, round.state().previous_sample, fire);
        round.resolve_actions([&](const auto& candidate, auto) {
            return candidate.image == "gfx/stone" && candidate.destination == draw.destination;
        });
        require(round.state().stones_hit == static_cast<int>(index + 1) && round.state().score == static_cast<int>((index + 1) * 2),
            "Eight distinct source stone slots must score once");
        require(!round.state().special_ammo, "Grant must wait for the update after the eighth stone shot");
    }
    // The source grant's80 draws exclude the earlier scene and particle passes.
    // The live frog precedes all32 permanent leaves, including their zero-delta
    // conditional amplitude draws (MH2.PERSISTENT_LEAF.RNG.1).
    auto random = game::test::after_leaves(game::test::snapshot(round.state()), 1);
    random.discard(16 * 5);
    step(round, round.state().previous_sample);
    require(round.state().special_ammo && round.state().stone_bonus_granted && round.state().ammo_index == 16 && has_event(round, game::RoundEventKind::special_ammo),
        "Eight stones grant persistent mode once without filling the empty magazine");
    check_random(round, random, "Frog and ordered permanent-leaf updates precede sixteen five-draw balloon allocations");
    require(std::ranges::count_if(round.state().balloons,
                [](const auto& balloon) {
                    return balloon.type == 21 && balloon.screen_x == -1024 && balloon.screen_y == -1024;
                })
                == 16,
        "New balloons are created after scene update and keep their offscreen birth pose");
    random = game::test::after_leaves(random, 1);
    step(round, round.state().previous_sample);
    require(!has_event(round, game::RoundEventKind::special_ammo), "Grant cannot repeat on the next update");
    check_random(round, random, "Live balloons consume no RNG while frog and permanent-leaf updates continue");
    game::RoundInput reload;
    reload.reload_down = true;
    random = game::test::after_leaves(random, 1);
    random.discard(16);
    step(round, round.state().previous_sample, reload);
    require(round.state().special_ammo && round.state().ammo_index == 8, "Reload retains special mode and restores the normal magazine");
    check_random(round, random, "Frog and permanent-leaf updates precede the sixteen cartridge reload draws");
    for (const auto& sprite : round.ammo_draws()) {
        require(sprite.image == "gfx/flyaway3", "Original kind1 switches every active or falling cartridge to flyaway3");
    }
    reload.fire_down = true;
    for (int index = 0; index < 16; ++index) {
        step(round, round.state().previous_sample, reload);
    }
    require(round.state().special_ammo && round.state().ammo_index == 8, "Two more magazine cycles do not exhaust the original permanent ammunition mode");
}

void grant_preconditions_and_pool() {
    for (const int count : {0, 7, 9}) {
        game::Round round(1, 0);
        fixture(round).stones_hit = count;
        const auto random = game::test::after_leaves(game::test::snapshot(round.state()), 1);
        step(round, 0);
        require(!round.state().special_ammo, "Only exactly eight stone hits satisfy the original grant condition");
        check_random(round, random, "A rejected grant consumes no RNG after the frog and permanent-leaf passes");
    }
    game::Round already_granted(1, 0);
    fixture(already_granted).stones_hit = 8;
    fixture(already_granted).stone_bonus_granted = true;
    const auto latched_random = game::test::after_leaves(game::test::snapshot(already_granted.state()), 1);
    step(already_granted, 0);
    require(
        !already_granted.state().special_ammo && !has_event(already_granted, game::RoundEventKind::special_ammo), "A set grant latch independently blocks the eight-stone trigger");
    check_random(already_granted, latched_random, "A latched grant consumes no RNG after the frog and permanent-leaf passes");
    for (const int free_slots : {0, 2}) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.stones_hit = 8;
        for (auto& target : state.chickens) {
            target.type = -2;
        }
        if (free_slots != 0) {
            state.chickens[44].type = -1;
            state.chickens[66].type = -1;
        }
        auto random = game::test::after_leaves(game::test::snapshot(state), 1);
        random.discard(static_cast<unsigned>(free_slots * 5));
        step(round, 0);
        require(state.special_ammo && has_event(round, game::RoundEventKind::special_ammo), "Failed first-free allocations still grant mode/magic");
        check_random(round, random, "Only successful first-free balloon allocations consume five draws after the frog and permanent leaves");
        require(std::ranges::count_if(state.balloons,
                    [](const auto& value) {
                        return value.type == 21;
                    })
                    == free_slots,
            "Grant must respect the shared scene pool");
        if (free_slots != 0) {
            require(state.balloons[44].type == 21 && state.balloons[66].type == 21 && state.chickens[44].type == -3, "Balloons occupy original first-free scene indices");
        }
    }
}

void boat_chain() {
    game::Round round(1, 0);
    fixture(round).camera = 320;
    for (int index = 0; index < 5; ++index) {
        if (index == 2) {
            fixture(round).special_ammo = true;
        }
        const auto draw = world::boat_collision(round.state().camera, round.state().recoil);
        game::RoundInput input;
        input.fire_down = true;
        input.pointer = {draw.destination.x + 70 - 16, draw.destination.y + 42 - 16};
        game::test::advance(round, 0, input);
        round.resolve_actions([](const auto& candidate, auto) {
            return candidate.image == "gfx/boatcol";
        });
        require(round.state().hits == index + 1 && round.state().boat.special_hits == std::max(0, index - 1), "Only special boat hits advance the three-hit damage count");
        require(round.state().score == (index == 4 ? 50 : 0), "Only the third special hit awards fifty");
    }
    require(round.state().boat.sinking && has_event(round, game::RoundEventKind::boat_sunk), "Third special hit starts sinking");
    const auto events = round.events();
    require(
        events.size() == 3 && events[0].kind == game::RoundEventKind::shot && events[1].kind == game::RoundEventKind::boat_hit && events[2].kind == game::RoundEventKind::boat_sunk,
        "Source gunshot, wood impact, sinking sound order differs");
    const auto& popup = round.state().particles[0];
    require(popup.active && popup.depth == 3 && popup.world_x == 678 && popup.world_y == 303 && popup.points == 50,
        "Boat fifty-point popup must use its original centered depth3 origin");
    step(round, 1.25F);
    require(round.state().boat.frame == 10 && round.state().chickens[3].type == -2 && round.state().particles[0].world_y == 271.75F,
        "Sunk boat holds final cell and scene slot; depth3 score rises 25 px/s");
    const auto draw = world::boat_collision(round.state().camera, round.state().recoil);
    game::RoundInput repeat;
    repeat.fire_down = true;
    repeat.pointer = {draw.destination.x + 70 - 16, draw.destination.y + 42 - 16};
    game::test::advance(round, round.state().previous_sample, repeat);
    round.resolve_actions([](const auto& candidate, auto) {
        return candidate.image == "gfx/boatcol";
    });
    require(round.state().score == 50 && round.state().hits == 5 && round.state().misses == 1 && round.state().boat.special_hits == 3
                && !has_event(round, game::RoundEventKind::boat_sunk) && !has_event(round, game::RoundEventKind::boat_hit),
        "A sunk boat cannot award another hit, point or impact sound");
}

void balloon_hits_and_particle_pool() {
    for (int variant = 0; variant < 5; ++variant) {
        for (const int occupied : {0, 66, 68}) {
            game::Round round(1, 0);
            auto& state = fixture(round);
            state.balloons[34] = world::make_balloon(640, {100, static_cast<unsigned>(variant % 4), 8, 20, variant == 4 ? 0U : 1U});
            state.chickens[34].type = -3;
            for (int index = 0; index < occupied; ++index) {
                state.particles[static_cast<std::size_t>(index)].active = true;
            }
            auto random = game::test::after_leaves(game::test::snapshot(state), 1);
            step(round, 0);
            const auto draw = world::balloon_draw(state.balloons[34]);
            game::RoundInput input;
            input.fire_down = true;
            input.pointer = {draw.destination.x + 11 - 16, draw.destination.y + 21 - 16};
            game::test::advance(round, 0, input);
            round.resolve_actions([](const auto& candidate, auto) {
                return candidate.image == "gfx/ballon";
            });
            const int points = variant == 4 ? 100 : variant + 1;
            const auto debris = variant == 4 ? std::min(32, std::max(0, 67 - occupied)) : 0;
            random = game::test::after_leaves(random, 1);
            random.discard(2 + static_cast<unsigned>(debris * 2));
            require(state.score == points && state.hits == 1 && state.balloons_hit == 1 && state.balloons[34].type == 61 && state.balloons[34].frame == variant + 5,
                "Every live balloon variant can score with ordinary ammo and changes to its corresponding popped frame");
            check_random(round, random, "Both frog/permanent-leaf passes precede impact draws; rare popup reserves a slot before up to 32 two-draw debris allocations");
            require(std::ranges::count_if(state.particles,
                        [](const auto& particle) {
                            return particle.active && particle.type == 3;
                        })
                        == debris,
                "Rare debris must share score-particle capacity");
            require(has_event(round, game::RoundEventKind::rare_balloon) == (variant == 4), "Only frame4 plays the source fifty-point sample before balloon pop");
            game::test::advance(round, 0, input);
            round.resolve_actions([](const auto& candidate, auto) {
                return candidate.image == "gfx/ballon";
            });
            require(state.score == points && state.hits == 1 && state.misses == 1 && state.chickens[34].type == -3, "A popped balloon is ineligible and retains its pool slot");
        }
    }
    game::Round fresh(1, 0);
    require(!fresh.state().special_ammo && !fresh.state().stone_bonus_granted && !fresh.state().boat.sinking && fresh.state().boat.special_hits == 0
                && fresh.state().balloons_hit == 0
                && std::ranges::none_of(fresh.state().balloons,
                    [](const auto& value) {
                        return value.type >= 0;
                    }),
        "New round clears every chain latch, counter and balloon");
}

struct DebrisCase {
    float camera, delta;
    int recoil;
    std::array<float, 6> before;
    std::array<std::uint32_t, 6> after;
    bool active;
};

constexpr std::array debris_cases{
    DebrisCase{640.0F, 0.25F, 0, {1211.0F, 361.0F, -1024.0F, -1024.0F, -0.3199999928474426F, -0.09999999403953552F},
        {0x44965e00U, 0x43b48000U, 0x4399f800U, 0x43b48000U, 0xbea51eb8U, 0x32000000U}, true},
    DebrisCase{640.0F, 0.25F, 8, {1211.0F, 361.0F, -1024.0F, -1024.0F, 0.3100000023841858F, 0.2800000011920929F},
        {0x44985a00U, 0x43b94000U, 0x43a1e800U, 0x43bc7333U, 0x3ea00000U, 0x3ec28f5cU}, true},
    DebrisCase{100.625F, 0.25F, 0, {1211.0F, 361.0F, -1024.0F, -1024.0F, 0.0F, 0.0F}, {0x44975e00U, 0x43b5c000U, 0x4485de00U, 0x43b5c000U, 0xbb23d70aU, 0x3dcccccdU}, true},
    DebrisCase{640.0F, 0.0F, 0, {1211.0F, 361.0F, -1024.0F, -1024.0F, -0.3199999928474426F, -0.09999999403953552F},
        {0x44976000U, 0x43b48000U, 0x439e0000U, 0x43b48000U, 0xbea3d70aU, 0xbdccccccU}, true},
    DebrisCase{640.0F, 0.0F, 0, {1211.0F, 480.0F, -1024.0F, -1024.0F, -0.3199999928474426F, -0.09999999403953552F},
        {0x44976000U, 0x43f00000U, 0x439e0000U, 0x43f00000U, 0xbea3d70aU, 0xbdccccccU}, true},
    DebrisCase{640.0F, 0.0F, 0, {1211.0F, 480.0000305175781F, -1024.0F, -1024.0F, -0.3199999928474426F, -0.09999999403953552F},
        {0x44976000U, 0x43f00001U, 0x439e0000U, 0x43f00001U, 0xbea3d70aU, 0xbdccccccU}, false},
    DebrisCase{640.0F, 0.25F, 8, {1211.0F, 479.0F, -1024.0F, -1024.0F, -0.3199999928474426F, 0.2800000011920929F},
        {0x44965e00U, 0x43f44000U, 0x4399f800U, 0x43f77333U, 0xbea51eb8U, 0x3ec28f5cU}, false},
    DebrisCase{640.0F, 1.52587890625e-05F, 0, {1211.0F, 480.0F, -1024.0F, -1024.0F, -0.3199999928474426F, 9.999999747378752e-05F},
        {0x44975ffcU, 0x43f00000U, 0x439dfff0U, 0x43f00000U, 0xbea3d70fU, 0x38de83e4U}, true},
};

struct PopupCase {
    float camera, delta;
    int recoil;
    std::array<float, 3> before;
    std::array<float, 4> after;
    bool active;
};

constexpr std::array popup_cases{
    PopupCase{640.0F, 0.25F, 8, {1201.0F, 361.0F, 0.0F}, {354.75F, 306.0F, 361.1499938964844F, 0.25F}, true},
    PopupCase{100.625F, 0.25F, 0, {1181.0F, 361.0F, 0.0F}, {354.75F, 1041.0F, 354.75F, 0.25F}, true},
    PopupCase{640.0F, 2.0F, 0, {1201.0F, 361.0F, 0.0F}, {311.0F, 306.0F, 311.0F, 2.0F}, true},
    PopupCase{640.0F, 1.1920928955078125e-07F, 0, {1201.0F, 361.0F, 2.0F}, {361.0F, 306.0F, 361.0F, 2.0F}, false},
};

void original_particle_vectors() {
    for (const auto& row : debris_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = row.camera;
        state.recoil = row.recoil;
        auto& particle = state.particles[0];
        particle = {.active = true,
            .type = 3,
            .depth = 3,
            .world_x = row.before[0],
            .world_y = row.before[1],
            .screen_x = row.before[2],
            .screen_y = row.before[3],
            .velocity_x = row.before[4],
            .velocity_y = row.before[5]};
        step(round, row.delta);
        const std::array actual{std::bit_cast<std::uint32_t>(particle.world_x), std::bit_cast<std::uint32_t>(particle.world_y), std::bit_cast<std::uint32_t>(particle.screen_x),
            std::bit_cast<std::uint32_t>(particle.screen_y), std::bit_cast<std::uint32_t>(particle.velocity_x), std::bit_cast<std::uint32_t>(particle.velocity_y)};
        require(particle.active == row.active && actual == row.after, "Original rare debris lifetime and binary32 stores differ");
    }
    for (const auto& row : popup_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = row.camera;
        state.recoil = row.recoil;
        auto& particle = state.particles[0];
        particle.active = true;
        particle.depth = 3;
        particle.world_x = row.before[0];
        particle.world_y = row.before[1];
        particle.age = row.before[2];
        step(round, row.delta);
        require(
            particle.active == row.active && std::array{particle.world_y, particle.screen_x, particle.screen_y, particle.age} == row.after, "Original depth3 popup vector differs");
    }
}
} // namespace

int main() {
    try {
        original_particle_vectors();
        eight_real_stones();
        grant_preconditions_and_pool();
        boat_chain();
        balloon_hits_and_particle_pool();
        std::cout << "Eight-stone grant, shared allocation, persistent ammo/reload, boat chain, all balloon variants and particle pressure passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
