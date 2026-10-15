#include "../../game/round_clock.hpp"
#include "game/round.hpp"
#include "support/numeric.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace moorhuhn2;

void check(bool condition, std::string_view id, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(id) + ": " + std::string(message));
    }
}

using moorhuhn2::test::binary32;

struct RandomBoundary {
    std::uint64_t draws;
    std::uint32_t state;
};

struct LeafDrift {
    std::size_t slot;
    float world_x;
    float amplitude;
};

struct BalloonPose {
    std::size_t slot;
    int type;
    float world_x;
};

struct EventExpectation {
    game::RoundEvent event;
    bool payload;
};

struct SceneUpdate {
    RandomBoundary random;
    std::vector<LeafDrift> leaves;
    std::vector<EventExpectation> events;
    std::vector<BalloonPose> balloons;
};

struct SceneCase {
    std::string id;
    std::uint32_t random_seed;
    std::size_t scene_capacity;
    bool frog_live;
    std::vector<BalloonPose> balloons;
    SceneUpdate after;
};

struct ImpactExpectation {
    RandomBoundary random;
    std::vector<EventExpectation> events;
    int type;
    int frame;
    float velocity_x;
    float velocity_y;
};

struct LiveHitCase {
    std::string id;
    std::uint32_t random_seed;
    SceneUpdate before_hit;
    ImpactExpectation after_hit;
    SceneUpdate next_update;
    float next_world_x;
};

struct AnimationState {
    int type;
    int frame;
    std::uint32_t animation_bits;
};

struct AnimationStep {
    std::uint32_t sample_bits;
    std::uint32_t delta_bits;
    bool fire;
    AnimationState after_advance;
    AnimationState after_actions;
};

struct AnimationCase {
    std::string id;
    int initial_type;
    std::uint32_t initial_animation_bits;
    std::vector<AnimationStep> steps;
};

struct CountdownStep {
    std::uint32_t sample_bits;
    std::uint32_t remaining_bits;
    int minutes;
    int seconds;
    int countdown_second;
    bool exit_requested;
    std::vector<EventExpectation> events;
};

struct CountdownCase {
    std::string id;
    int initial_cache;
    std::vector<CountdownStep> steps;
};

const std::array<SceneCase, 8> scene_cases = {{SceneCase{"early-conditional-drift-before-frog", 2U, 128U, true, {BalloonPose{5U, 61, 100.0F}},
                                                   SceneUpdate{RandomBoundary{37ULL, 0x6c4b7231U},
                                                       {LeafDrift{2U, 702.0F, 6.0F}, LeafDrift{14U, 714.0F, 1.0F}, LeafDrift{25U, 726.0F, 0.0F}}, {},
                                                       {BalloonPose{5U, 61, 101.0F}}}},
    SceneCase{"late-balloon-after-frog", 2U, 128U, true, {BalloonPose{34U, 61, 100.0F}},
        SceneUpdate{
            RandomBoundary{36ULL, 0x489c4186U}, {LeafDrift{3U, 703.0F, 6.0F}, LeafDrift{15U, 715.0F, 1.0F}, LeafDrift{26U, 727.0F, 0.0F}}, {}, {BalloonPose{34U, 61, 100.0F}}}},
    SceneCase{"early-no-second-draw", 1U, 128U, true, {BalloonPose{5U, 61, 100.0F}}, SceneUpdate{RandomBoundary{34ULL, 0x2d128b23U}, {}, {}, {BalloonPose{5U, 61, 100.0F}}}},
    SceneCase{"early-conditional-zero-drift", 19U, 128U, true, {BalloonPose{5U, 61, 100.0F}},
        SceneUpdate{RandomBoundary{35ULL, 0xe5f2b874U}, {LeafDrift{12U, 713.0F, 0.0F}}, {}, {BalloonPose{5U, 61, 100.0F}}}},
    SceneCase{"early-conditional-frog-croak", 2132U, 128U, true, {BalloonPose{5U, 61, 100.0F}},
        SceneUpdate{RandomBoundary{36ULL, 0x3d191878U}, {LeafDrift{18U, 718.0F, 2.0F}, LeafDrift{27U, 728.0F, 0.0F}},
            {EventExpectation{game::RoundEvent{game::RoundEventKind::frog_croak, 19, 2, 0, binary32(0xbecccccdU)}, true}}, {BalloonPose{5U, 61, 99.0F}}}},
    SceneCase{"mixed-early-late-and-last-slot", 2U, 128U, true, {BalloonPose{5U, 61, 100.0F}, BalloonPose{34U, 61, 200.0F}, BalloonPose{127U, 61, 300.0F}},
        SceneUpdate{RandomBoundary{40ULL, 0xc9e9bc2aU}, {LeafDrift{11U, 711.0F, 1.0F}, LeafDrift{22U, 723.0F, 0.0F}}, {},
            {BalloonPose{5U, 61, 101.0F}, BalloonPose{34U, 61, 199.0F}, BalloonPose{127U, 61, 300.0F}}}},
    SceneCase{"expanded-scene-last-slot", 2U, 256U, true, {BalloonPose{5U, 61, 100.0F}, BalloonPose{34U, 61, 200.0F}, BalloonPose{127U, 61, 300.0F}, BalloonPose{255U, 61, 400.0F}},
        SceneUpdate{RandomBoundary{41ULL, 0xece29245U}, {LeafDrift{10U, 710.0F, 1.0F}, LeafDrift{21U, 722.0F, 0.0F}}, {},
            {BalloonPose{5U, 61, 101.0F}, BalloonPose{34U, 61, 199.0F}, BalloonPose{127U, 61, 300.0F}, BalloonPose{255U, 61, 400.0F}}}},
    SceneCase{"dying-balloon-without-live-frog", 2U, 128U, false, {BalloonPose{5U, 61, 100.0F}, BalloonPose{127U, 61, 200.0F}},
        SceneUpdate{RandomBoundary{37ULL, 0x6c4b7231U}, {LeafDrift{2U, 702.0F, 6.0F}, LeafDrift{14U, 714.0F, 1.0F}, LeafDrift{25U, 726.0F, 0.0F}}, {},
            {BalloonPose{5U, 61, 101.0F}, BalloonPose{127U, 61, 200.0F}}}}}};

const LiveHitCase expected_live_hit = LiveHitCase{"normal-live-hit-then-early-death-update", 2U,
    SceneUpdate{RandomBoundary{35ULL, 0x4c8f5dbfU}, {LeafDrift{4U, 704.0F, 6.0F}, LeafDrift{16U, 716.0F, 1.0F}, LeafDrift{27U, 728.0F, 0.0F}}, {}, {}},
    ImpactExpectation{RandomBoundary{37ULL, 0x6c4b7231U},
        {EventExpectation{game::RoundEvent{game::RoundEventKind::shot, -1, 0, 0, 0.0F}, true},
            EventExpectation{game::RoundEvent{game::RoundEventKind::balloon_hit, 5, 3, 2, 0.0F}, true}},
        61, 6, 8.0F, 3.0F},
    SceneUpdate{RandomBoundary{71ULL, 0x9f5b86d3U}, {LeafDrift{4U, 704.0F, 6.0F}, LeafDrift{16U, 716.0F, 1.0F}, LeafDrift{27U, 728.0F, 0.0F}}, {}, {}}, 100.0F};

const std::array<AnimationCase, 7> animation_cases = {
    {AnimationCase{"live-stored19-retained18", 2, 0x00000000U,
         {AnimationStep{0x3fa22222U, 0x3fa22222U, false, AnimationState{2, 18, 0x41980000U}, AnimationState{2, 18, 0x41980000U}},
             AnimationStep{0x3fa22222U, 0x00000000U, false, AnimationState{2, 0, 0x00000000U}, AnimationState{2, 0, 0x00000000U}}}},
        AnimationCase{
            "live-upper-neighbor-resets", 2, 0x00000000U, {AnimationStep{0x3fa22223U, 0x3fa22223U, false, AnimationState{2, 0, 0x00000000U}, AnimationState{2, 0, 0x00000000U}}}},
        AnimationCase{"live-stored19-hit-retains-accumulator", 2, 0x00000000U,
            {AnimationStep{0x3fa22222U, 0x3fa22222U, true, AnimationState{2, 18, 0x41980000U}, AnimationState{42, 0, 0x41980000U}},
                AnimationStep{0x3fa22222U, 0x00000000U, false, AnimationState{42, 0, 0x00000000U}, AnimationState{42, 0, 0x00000000U}}}},
        AnimationCase{"death-stored10-retained9", 42, 0x3f000000U,
            {AnimationStep{0x3f222222U, 0x3f222222U, false, AnimationState{42, 9, 0x41200000U}, AnimationState{42, 9, 0x41200000U}},
                AnimationStep{0x3f222222U, 0x00000000U, false, AnimationState{42, 0, 0x00000000U}, AnimationState{42, 0, 0x00000000U}}}},
        AnimationCase{"death-upper-neighbor-resets", 42, 0x3f000000U,
            {AnimationStep{0x3f222223U, 0x3f222223U, false, AnimationState{42, 0, 0x00000000U}, AnimationState{42, 0, 0x00000000U}}}},
        AnimationCase{
            "live-wrap-discards-remainder", 2, 0x41980000U, {AnimationStep{0x3e000000U, 0x3e000000U, false, AnimationState{2, 0, 0x00000000U}, AnimationState{2, 0, 0x00000000U}}}},
        AnimationCase{"death-wrap-discards-remainder", 42, 0x41200000U,
            {AnimationStep{0x3e000000U, 0x3e000000U, false, AnimationState{42, 0, 0x00000000U}, AnimationState{42, 0, 0x00000000U}}}}}};

const std::array<CountdownCase, 8> timer_cases = {
    {CountdownCase{"cold-first-eight-counts", 0,
         {CountdownStep{0x42a30000U, 0x41080000U, 0, 8, 8, false, {EventExpectation{game::RoundEvent{game::RoundEventKind::countdown, -1, 0, 0, 0.0F}, false}}}}},
        CountdownCase{"retained-eight-first-eight-is-silent", 8, {CountdownStep{0x42a30000U, 0x41080000U, 0, 8, 8, false, {}}}},
        CountdownCase{"retained-seven-first-eight-counts", 7,
            {CountdownStep{0x42a30000U, 0x41080000U, 0, 8, 8, false, {EventExpectation{game::RoundEvent{game::RoundEventKind::countdown, -1, 0, 0, 0.0F}, false}}}}},
        CountdownCase{"cold-first-zero-does-not-exit", 0, {CountdownStep{0x42b30000U, 0x3f000000U, 0, 0, 0, false, {}}}},
        CountdownCase{"retained-eight-first-zero-exits", 8,
            {CountdownStep{0x42b30000U, 0x3f000000U, 0, 0, 0, true, {EventExpectation{game::RoundEvent{game::RoundEventKind::game_over, -1, 0, 0, 0.0F}, false}}}}},
        CountdownCase{"retained-eight-above-countdown-keeps-cache", 8,
            {CountdownStep{0x428c0000U, 0x41a00000U, 0, 20, 8, false, {}}, CountdownStep{0x42a00000U, 0x41200000U, 0, 10, 8, false, {}},
                CountdownStep{0x42a30000U, 0x41080000U, 0, 8, 8, false, {}}}},
        CountdownCase{"retained-eight-skips-zero", 8,
            {CountdownStep{0x42b80000U, 0xc0000000U, 0, -2, -2, false, {EventExpectation{game::RoundEvent{game::RoundEventKind::countdown, -1, 0, 0, 0.0F}, false}}}}},
        CountdownCase{"ordinary-countdown-then-zero", 0,
            {CountdownStep{0x42a30000U, 0x41080000U, 0, 8, 8, false, {EventExpectation{game::RoundEvent{game::RoundEventKind::countdown, -1, 0, 0, 0.0F}, false}}},
                CountdownStep{0x42a50000U, 0x40f00000U, 0, 7, 7, false, {EventExpectation{game::RoundEvent{game::RoundEventKind::countdown, -1, 0, 0, 0.0F}, false}}},
                CountdownStep{0x42b30000U, 0x3f000000U, 0, 0, 0, true, {EventExpectation{game::RoundEvent{game::RoundEventKind::game_over, -1, 0, 0, 0.0F}, false}}}}}}};

void check_events(const game::Round& round, std::span<const EventExpectation> expected, std::string_view id) {
    check(round.events().size() == expected.size(), id, "Source event count");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto& event = round.events()[index];
        const auto& row = expected[index];
        check(event.kind == row.event.kind, id, "Source event order");
        if (row.payload) {
            check(event.slot == row.event.slot && event.depth == row.event.depth && event.points == row.event.points
                      && std::bit_cast<std::uint32_t>(event.pan) == std::bit_cast<std::uint32_t>(row.event.pan),
                id, "Source event slot, depth, points and retained-position pan");
        }
    }
}

game::RoundState& fixture(game::Round& round, bool frog_live = true) {
    auto& state = const_cast<game::RoundState&>(round.state());
    // Remove unrelated conditional consumers. Keep the full scene and permanent
    // particle passes; the reviewed vectors include every leaf's random gate.
    state.spawn_accumulator = -10000;
    state.camera = 0;
    state.hidden_chicken.type = -1;
    state.spider.type = -1;
    state.frog.type = frog_live ? 11 : -1;
    state.frog.screen_x = 4;
    for (std::size_t index = 0; index < state.persistent_leaves.size(); ++index) {
        state.persistent_leaves[index] = {.world_x = static_cast<float>(700 + index), .world_y = 240};
    }
    return state;
}

void step(game::Round& round, float sample, game::RoundInput input = {}) {
    game::test::advance(round, sample, input);
    round.resolve_actions({});
}

void check_random(const game::RoundState& state, const RandomBoundary& expected, std::string_view id) {
    check(state.random_draws == expected.draws && state.random_state == expected.state, id, "Full scene/particle CRT boundary");
}

void check_leaves(const game::RoundState& state, std::span<const LeafDrift> expected, std::string_view id) {
    for (std::size_t slot = 0; slot < state.persistent_leaves.size(); ++slot) {
        const auto row = std::ranges::find_if(expected, [slot](const auto& value) {
            return value.slot == slot;
        });
        const float x = row == expected.end() ? static_cast<float>(700 + slot) : row->world_x;
        const float amplitude = row == expected.end() ? 0 : row->amplitude;
        const auto& leaf = state.persistent_leaves[slot];
        check(leaf.world_x == x && leaf.amplitude == amplitude, id, "All 32 original leaf drift and conditional amplitude outputs");
    }
}

void install_scene(game::Round& round, const SceneCase& row) {
    auto& state = fixture(round, row.frog_live);
    state.random_state = row.random_seed;
    state.random_draws = 0;
    state.scene_capacity = row.scene_capacity;
    for (const auto& initial : row.balloons) {
        const auto slot = initial.slot;
        state.chickens[slot].type = -3;
        state.balloons[slot] = {.type = initial.type, .world_x = initial.world_x, .world_y = 100, .frame = 6, .points = 2};
    }
}

void check_scene(const game::Round& round, const SceneUpdate& expected, std::string_view id) {
    const auto& state = round.state();
    check_random(state, expected.random, id);
    check_leaves(state, expected.leaves, id);
    check_events(round, expected.events, id);
    for (const auto& row : expected.balloons) {
        const auto slot = row.slot;
        const auto& balloon = state.balloons[slot];
        check(balloon.type == row.type && balloon.world_x == row.world_x && balloon.screen_x == balloon.world_x && balloon.world_y == 100 && balloon.screen_y == 100
                  && state.chickens[slot].type == -3,
            id, "The reused scene slot drifts and poses exactly once in its ascending turn");
    }
}

void scene_order(std::span<const SceneCase> cases) {
    for (const auto& row : cases) {
        const auto id = row.id;
        game::Round round(1, 0);
        install_scene(round, row);
        game::test::advance(round, 0, {});
        check_scene(round, row.after, id);
        const auto before = round.state();
        for (int depth = 2; depth <= 4; ++depth) {
            static_cast<void>(round.target_draws(depth));
            static_cast<void>(round.target_draws(depth));
        }
        check(round.state() == before, id, "Rendering cannot move scene records or consume CRT words");
        round.resolve_actions({});
        check_scene(round, row.after, id);
    }
}

void normal_live_hit(const LiveHitCase& row) {
    const auto id = row.id;
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.random_state = row.random_seed;
    state.random_draws = 0;
    state.chickens[5].type = -3;
    state.balloons[5] = {.type = 21, .world_x = 100, .world_y = 100, .frame = 1, .points = 2};
    game::test::advance(round, 0, {.pointer = {95, 105}, .fire_down = true});
    check_random(state, row.before_hit.random, id);
    check_leaves(state, row.before_hit.leaves, id);
    check_events(round, row.before_hit.events, id);
    check(state.balloons[5].type == 21 && state.balloons[5].world_x == 100, id, "Live balloon update consumes no drift word before input");
    round.resolve_actions([](const auto& draw, auto) {
        return draw.image == "gfx/ballon";
    });
    const auto& hit = row.after_hit;
    check_random(state, hit.random, id);
    check_events(round, hit.events, id);
    check(state.selected_slot == 5 && state.hits == 1 && state.misses == 0 && state.score == 2 && state.ammo_index == 9 && state.balloons[5].type == hit.type
              && state.balloons[5].frame == hit.frame && state.balloons[5].velocity_x == hit.velocity_x && state.balloons[5].velocity_y == hit.velocity_y,
        id, "Normal firing retains the reused slot and consumes its two impact words after the particle pass");
    check_leaves(state, row.before_hit.leaves, id);
    game::test::advance(round, 0, {});
    const auto& next = row.next_update;
    check_random(state, next.random, id);
    check_leaves(state, next.leaves, id);
    check_events(round, next.events, id);
    check(state.balloons[5].world_x == row.next_world_x && state.balloons[5].screen_y == 106.4F, id,
        "Next update places the dying balloon before the frog and uses the prior shot recoil");
    round.resolve_actions({});
    check_random(state, next.random, id);
}

void animation_rounding(std::span<const AnimationCase> cases) {
    for (const auto& row : cases) {
        const auto id = row.id;
        game::Round round(1, 0);
        auto& state = fixture(round, false);
        auto& chicken = state.chickens[34];
        chicken = {.type = row.initial_type, .depth = 2, .world_x = 100, .world_y = 100, .animation_time = binary32(row.initial_animation_bits), .points = 5};
        for (const auto& update : row.steps) {
            const bool fire = update.fire;
            game::test::advance(round, binary32(update.sample_bits), {.pointer = {205, 110}, .fire_down = fire});
            const auto& expected = update.after_advance;
            check(std::bit_cast<std::uint32_t>(state.delta) == update.delta_bits && chicken.type == expected.type && chicken.frame == expected.frame
                      && std::bit_cast<std::uint32_t>(chicken.animation_time) == expected.animation_bits,
                id, "Frame truncates the retained x87 value after the independent binary32 store");
            const auto draw = game::chicken_draw(chicken);
            check(draw.source.y == chicken.frame * 124, id, "The retained frame selects the actual normal or dying sprite row");
            round.resolve_actions([](const auto& draw, auto) {
                return draw.image == "gfx/Chickright1";
            });
            const auto& resolved = update.after_actions;
            check(chicken.type == resolved.type && chicken.frame == resolved.frame && std::bit_cast<std::uint32_t>(chicken.animation_time) == resolved.animation_bits, id,
                "Impact retains animation storage; the following scene update applies the death count");
            if (fire) {
                check(state.selected_slot == 34 && state.score == 5 && state.hits == 1 && state.ammo_index == 9 && round.events().size() == 2
                          && round.events()[0].kind == game::RoundEventKind::shot && round.events()[1].kind == game::RoundEventKind::chicken_hit,
                    id, "The retained-value boundary remains hittable through normal input resolution");
            }
        }
    }
}

void countdown_cache(std::span<const CountdownCase> cases) {
    for (const auto& row : cases) {
        const auto id = row.id;
        const int initial_cache = row.initial_cache;
        // Establish retained 7/8 through a real previous round's timer update.
        game::Round previous(1, 0);
        fixture(previous, false);
        if (initial_cache != 0) {
            step(previous, 89.5F - static_cast<float>(initial_cache));
        }
        check(previous.state().countdown_second == initial_cache, id, "Previous normal round produces the retained cache");
        game::Round round(1, 0, previous.state().countdown_second);
        check(round.state().countdown_second == initial_cache && round.state().remaining == 90 && round.state().score == 0 && round.state().ammo_index == 8
                  && round.state().random_draws == 176 && round.state().updates == 0 && !round.state().exit_requested,
            id, "New round retains only the incoming cached second among these round-entry fields");
        fixture(round, false);
        for (const auto& expected : row.steps) {
            step(round, binary32(expected.sample_bits));
            const auto& state = round.state();
            check(std::bit_cast<std::uint32_t>(state.remaining) == expected.remaining_bits && state.minutes == expected.minutes && state.seconds == expected.seconds
                      && state.countdown_second == expected.countdown_second && state.exit_requested == expected.exit_requested,
                id, "Retained cache controls changed-second countdown and zero-bucket exit");
            check_events(round, expected.events, id);
        }
    }
    game::Round retained(1, 0, 8);
    fixture(retained, false);
    step(retained, 89.5F);
    game::Round restart(1, 0, retained.state().countdown_second);
    fixture(restart, false);
    step(restart, 89.5F);
    check(retained.state().exit_requested && !restart.state().exit_requested && restart.state().countdown_second == 0, "natural-expiry-cache-reset",
        "A source zero-bucket exit writes the cache that the following round inherits");
}

void pause_and_resume(std::span<const SceneCase> cases) {
    const auto row = std::ranges::find_if(cases, [](const auto& value) {
        return value.id == "mixed-early-late-and-last-slot";
    });
    check(row != cases.end(), "pause", "Pause scenario requires the mixed scene case");
    game::Round round(1, 0);
    install_scene(round, *row);
    step(round, 0);
    check_scene(round, row->after, "pause-first-update");
    auto frozen = round.state();
    frozen.previous_sample = 100;
    frozen.delta = 0;
    round.reset_sample(100);
    check(round.state() == frozen, "pause", "Resetting a paused sample cannot run an additional scene or particle pass");
    for (const float sample : {100.0F, 100.125F}) {
        const float delta = sample - round.state().previous_sample;
        game::test::advance(round, sample, {});
        const auto before_render = round.state();
        static_cast<void>(round.target_draws(3));
        check(round.state() == before_render, "pause", "Repeated composition cannot reorder scene consumers");
        round.resolve_actions({});
        check(round.state().delta == delta && round.state().remaining == frozen.remaining - (sample - 100), "pause",
            "Resumed scene updates use only the current sampled delta without catching up paused time");
    }
    check(round.state().updates == 3, "pause", "Sample reset does not run a scene update");
}
} // namespace

int main() {
    try {
        scene_order(scene_cases);
        normal_live_hit(expected_live_hit);
        animation_rounding(animation_cases);
        countdown_cache(timer_cases);
        pause_and_resume(scene_cases);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Source scene order/CRT, live hit/death, retained animation/countdown cache and pause/resume passed\n";
}
