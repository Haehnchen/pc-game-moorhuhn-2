#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "game/round.hpp"
#include "support/require.hpp"

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

using namespace moorhuhn2;

namespace {
using moorhuhn2::test::require;

float number(std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}

game::RoundState& fixture(game::Round& round) {
    auto& state = const_cast<game::RoundState&>(round.state());
    // Isolate the fixture's allocator/reload stream from unrelated entity RNG.
    state.frog.type = -1;
    state.hidden_chicken.type = -1;
    state.spider.type = -1;
    state.camera = 0;
    state.random_state = 1;
    state.random_draws = 0;
    return state;
}

void tick(game::Round& round, float delta = 0, game::RoundInput input = {}, bool local_reload_stream = false) {
    game::test::advance(round, round.state().previous_sample + delta, input);
    if (local_reload_stream) {
        // Literal reload vectors begin at the action boundary; normal leaves
        // have already consumed the preceding update stream.
        auto& state = const_cast<game::RoundState&>(round.state());
        state.random_state = 1;
        state.random_draws = 0;
    }
    round.resolve_actions([](const auto& draw, auto) {
        return draw.image == "gfx/rose";
    });
}

struct TriggerState {
    int score, house_flag;
    std::uint32_t remaining_bits;
    int ammo_index, selected_type;
};

struct TriggerExpectation {
    int score;
    bool mode;
    std::uint32_t remaining_bits;
    int rose_type;
    std::size_t scene_limit;
    int reload_start_index;
    std::uint32_t spawn_threshold_bits;
};

struct TriggerCase {
    std::string_view name;
    TriggerState before;
    TriggerExpectation expected;
};

constexpr std::array trigger_cases{
    TriggerCase{"lower", {88, 1, 0x42700000U, 15, 29}, {98, true, 0x42700000U, 69, 256, 0, 0x40a00000U}},
    TriggerCase{"upper", {88, 1, 0x42820000U, 15, 29}, {98, true, 0x42820000U, 69, 256, 0, 0x40a00000U}},
    TriggerCase{"lower-below", {88, 1, 0x426fffffU, 15, 29}, {98, false, 0x426fffffU, 69, 128, 8, 0x42480000U}},
    TriggerCase{"upper-above", {88, 1, 0x42820001U, 15, 29}, {98, false, 0x42820001U, 69, 128, 8, 0x42480000U}},
    TriggerCase{"score87", {87, 1, 0x42700000U, 15, 29}, {97, false, 0x42700000U, 69, 128, 8, 0x42480000U}},
    TriggerCase{"post-award88", {78, 1, 0x42700000U, 15, 29}, {88, false, 0x42700000U, 69, 128, 8, 0x42480000U}},
    TriggerCase{"score89", {89, 1, 0x42700000U, 15, 29}, {99, false, 0x42700000U, 69, 128, 8, 0x42480000U}},
    TriggerCase{"house0", {88, 0, 0x42700000U, 15, 29}, {98, false, 0x42700000U, 69, 128, 8, 0x42480000U}},
    TriggerCase{"early-preempts", {88, 1, 0x42aa0000U, 15, 29}, {98, false, 0x42b40000U, 69, 128, 8, 0x42480000U}},
    TriggerCase{"late-preempts", {88, 1, 0x40a00000U, 15, 29}, {138, false, 0x40a00000U, 69, 128, 8, 0x42480000U}},
    TriggerCase{"reaction-ineligible", {88, 1, 0x42700000U, 15, 69}, {88, false, 0x42700000U, 69, 128, 8, 0x42480000U}},
};

struct CadenceCase {
    std::string_view name;
    std::uint32_t accumulator_bits, delta_bits, threshold_bits, final_accumulator_bits;
    bool scene_free;
    std::uint64_t allocator_draws;
};

constexpr std::array cadence_cases{
    CadenceCase{"normal-equality", 0x00000000U, 0x3f800000U, 0x42480000U, 0x42480000U, true, 0},
    CadenceCase{"normal-above", 0x00000000U, 0x3f800001U, 0x42480000U, 0x00000000U, true, 3},
    CadenceCase{"mode-equality", 0x3ff00000U, 0x3d800000U, 0x40a00000U, 0x40a00000U, true, 0},
    CadenceCase{"mode-tenth-rounds-above", 0x00000000U, 0x3dcccccdU, 0x40a00000U, 0x00000000U, true, 3},
    CadenceCase{"retained-above-stored-equal", 0x40a00000U, 0x31800000U, 0x40a00000U, 0x00000000U, true, 3},
    CadenceCase{"large-step-one-request", 0x00000000U, 0x41200000U, 0x40a00000U, 0x00000000U, true, 3},
    CadenceCase{"full-pool-still-discards-overshoot", 0x00000000U, 0x3dcccccdU, 0x40a00000U, 0x00000000U, false, 0},
};

struct SpawnExpectation {
    int slot, type, depth, points;
    std::uint32_t world_x_bits, random_state;
    std::uint64_t random_draws;
};

struct AllocatorCase {
    int scene_limit;
    std::vector<std::size_t> free_slots;
    SpawnExpectation expected;
};

const std::array allocator_cases{
    AllocatorCase{128, {35, 34}, {34, 0, 4, 25, 0x4498c000U, 415139642U, 3}},
    AllocatorCase{256, {127, 35}, {35, 0, 4, 25, 0x4498c000U, 415139642U, 3}},
    AllocatorCase{256, {}, {-1, 0, 0, 0, 0x00000000U, 1U, 0}},
    AllocatorCase{128, {128}, {-1, 0, 0, 0, 0x00000000U, 1U, 0}},
    AllocatorCase{256, {255, 128}, {128, 0, 4, 25, 0x4498c000U, 415139642U, 3}},
};

struct AmmoExpectation {
    std::size_t index;
    int state, frame;
    std::array<std::uint32_t, 5> bits;
};

constexpr std::array ordinary_ammo{
    AmmoExpectation{8, 1, 21, {0x43c30000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf800000U}},
    AmmoExpectation{9, 1, 21, {0x43cf8000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbf19999aU}},
    AmmoExpectation{10, 1, 21, {0x43dc0000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbfb33333U}},
    AmmoExpectation{11, 1, 21, {0x43e88000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbf99999aU}},
    AmmoExpectation{12, 1, 21, {0x43f50000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbfb33333U}},
    AmmoExpectation{13, 1, 21, {0x4400c000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf19999aU}},
    AmmoExpectation{14, 1, 21, {0x44070000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf800000U}},
    AmmoExpectation{15, 1, 21, {0x440d4000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf4ccccdU}},
};

constexpr std::array house_ammo{
    AmmoExpectation{0, 1, 21, {0x433e0000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf800000U}},
    AmmoExpectation{1, 1, 21, {0x43570000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbf19999aU}},
    AmmoExpectation{2, 1, 21, {0x43700000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbfb33333U}},
    AmmoExpectation{3, 1, 21, {0x43848000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbf99999aU}},
    AmmoExpectation{4, 1, 21, {0x43910000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbfb33333U}},
    AmmoExpectation{5, 1, 21, {0x439d8000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf19999aU}},
    AmmoExpectation{6, 1, 21, {0x43aa0000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf800000U}},
    AmmoExpectation{7, 1, 21, {0x43b68000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf4ccccdU}},
    AmmoExpectation{8, 1, 21, {0x43c30000U, 0x43cf8000U, 0x00000000U, 0x3fe00000U, 0xbf800000U}},
    AmmoExpectation{9, 1, 21, {0x43cf8000U, 0x43cf8000U, 0x00000000U, 0x3fe00000U, 0xbf4ccccdU}},
    AmmoExpectation{10, 1, 21, {0x43dc0000U, 0x43cf8000U, 0x00000000U, 0x3fe00000U, 0xbfb33333U}},
    AmmoExpectation{11, 1, 21, {0x43e88000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbf99999aU}},
    AmmoExpectation{12, 1, 21, {0x43f50000U, 0x43cf8000U, 0x00000000U, 0x3f800000U, 0xbf800000U}},
    AmmoExpectation{13, 1, 21, {0x4400c000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf4ccccdU}},
    AmmoExpectation{14, 1, 21, {0x44070000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbf19999aU}},
    AmmoExpectation{15, 1, 21, {0x440d4000U, 0x43cf8000U, 0x00000000U, 0x3fe00000U, 0xbf4ccccdU}},
};

constexpr std::array branch_ammo{
    AmmoExpectation{6, 1, 21, {0x43aa0000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf800000U}},
    AmmoExpectation{7, 1, 21, {0x43b68000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbf19999aU}},
    AmmoExpectation{8, 1, 21, {0x43c30000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbfb33333U}},
    AmmoExpectation{9, 1, 21, {0x43cf8000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbf99999aU}},
    AmmoExpectation{10, 1, 21, {0x43dc0000U, 0x43cf8000U, 0x00000000U, 0x3fc00000U, 0xbfb33333U}},
    AmmoExpectation{11, 1, 21, {0x43e88000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf19999aU}},
    AmmoExpectation{12, 1, 21, {0x43f50000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf800000U}},
    AmmoExpectation{13, 1, 21, {0x4400c000U, 0x43cf8000U, 0x00000000U, 0x3fa00000U, 0xbf4ccccdU}},
    AmmoExpectation{14, 1, 21, {0x44070000U, 0x43cf8000U, 0x00000000U, 0x3fe00000U, 0xbf800000U}},
    AmmoExpectation{15, 1, 21, {0x440d4000U, 0x43cf8000U, 0x00000000U, 0x3fe00000U, 0xbf4ccccdU}},
};

constexpr std::array<AmmoExpectation, 0> unchanged_ammo{};

struct ReloadCase {
    std::string_view name;
    int reload_start_index, ammo_index;
    bool fire, right;
    int expected_ammo_index;
    std::uint32_t random_state;
    std::uint64_t random_draws;
    bool did_reload;
    std::span<const AmmoExpectation> records;
};

const std::array reload_cases{
    ReloadCase{"ordinary-eight", 8, 16, false, true, 8, 2179684817U, 16, true, ordinary_ammo},
    ReloadCase{"house-sixteen", 0, 16, false, true, 0, 1423890337U, 32, true, house_ammo},
    ReloadCase{"branch-ten", 6, 16, false, true, 6, 2503793637U, 20, true, branch_ammo},
    ReloadCase{"partial-ignored", 0, 12, false, true, 12, 1U, 0, false, unchanged_ammo},
    ReloadCase{"last-shot-no-auto-reload", 0, 15, true, false, 16, 1U, 0, false, unchanged_ammo},
    ReloadCase{"last-shot-and-right", 0, 15, true, true, 0, 1423890337U, 32, true, house_ammo},
};

struct BranchCase {
    int branch_hits;
    bool granted;
    int reload_start_index;
    bool expected_granted;
    int expected_start_index;
};

constexpr std::array branch_cases{
    BranchCase{4, false, 0, true, 6},
    BranchCase{4, false, 8, true, 6},
    BranchCase{4, true, 0, true, 0},
    BranchCase{3, false, 0, false, 0},
    BranchCase{5, false, 0, false, 0},
};

void trigger() {
    for (const auto& row : trigger_cases) {
        const auto& before = row.before;
        const auto& expected = row.expected;
        for (const bool special : {false, true}) {
            game::Round round(1, 0);
            auto& state = fixture(round);
            state.score = before.score;
            state.village.house_hit = before.house_flag == 1;
            state.remaining = number(before.remaining_bits);
            state.ammo_index = before.ammo_index;
            state.rose.type = before.selected_type;
            state.special_ammo = special;
            const auto random = game::test::after_leaves(game::test::snapshot(state));
            tick(round, 0, {.pointer = {491, 390}, .fire_down = true});
            require(state.score == expected.score && state.house88_mode == expected.mode && state.remaining == number(expected.remaining_bits)
                        && state.rose.type == expected.rose_type && state.scene_capacity == expected.scene_limit && state.reload_start_index == expected.reload_start_index
                        && state.spawn_threshold == number(expected.spawn_threshold_bits) && state.ammo_index == before.ammo_index + 1 && state.random_draws == random.draws
                        && state.random_state == random.state,
                "Original trigger/precedence/ammunition state: " + std::string(row.name));
            const int score = state.score;
            tick(round, 0, {.pointer = {491, 390}, .fire_down = true, .reload_down = true});
            require(state.score == score && state.ammo_index == state.reload_start_index, "Empty repeat cannot trigger again; refill uses the new start index");
        }
    }
}

void cadence() {
    for (const auto& row : cadence_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.spawn_accumulator = number(row.accumulator_bits);
        state.spawn_threshold = number(row.threshold_bits);
        if (!row.scene_free) {
            for (auto& chicken : state.chickens) {
                chicken.type = -2;
            }
        }
        tick(round, number(row.delta_bits));
        require(state.spawn_accumulator == number(row.final_accumulator_bits) && state.random_draws == game::test::after_leaves({1, 0}, row.allocator_draws).draws,
            "Retained cadence threshold, one request and overshoot: " + std::string(row.name));
    }
}

void allocator() {
    for (const auto& row : allocator_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.scene_capacity = row.scene_limit;
        state.spawn_accumulator = 51;
        for (auto& chicken : state.chickens) {
            chicken.type = -2;
        }
        for (const auto& slot : row.free_slots) {
            state.chickens[slot].type = -1;
        }
        tick(round);
        const auto& expected = row.expected;
        const auto random = game::test::after_leaves({expected.random_state, expected.random_draws});
        require(state.random_draws == random.draws && state.random_state == random.state, "Allocator RNG stops at the active scan bound before the normal leaf pass");
        const auto event = std::ranges::find(round.events(), game::RoundEventKind::spawn, &game::RoundEvent::kind);
        const int slot = expected.slot;
        require(slot < 0 ? event == round.events().end() : event != round.events().end() && event->slot == slot, "First-free allocation including inert upper slots");
        if (slot >= 0) {
            const auto& chicken = state.chickens[static_cast<std::size_t>(slot)];
            require(chicken.type == expected.type && chicken.depth == expected.depth && chicken.world_x == number(expected.world_x_bits) && chicken.points == expected.points,
                "Actual source spawn in the selected scene slot");
        }
    }
}

void reload() {
    for (const auto& row : reload_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.reload_start_index = row.reload_start_index;
        state.ammo_index = row.ammo_index;
        tick(round, 0, {.fire_down = row.fire, .reload_down = row.right}, true);
        require(state.ammo_index == row.expected_ammo_index && state.random_draws == row.random_draws && state.random_state == row.random_state,
            "Reload count and ordered CRT draws: " + std::string(row.name));
        const bool did_reload = std::ranges::find(round.events(), game::RoundEventKind::reload, &game::RoundEvent::kind) != round.events().end();
        require(did_reload == row.did_reload, "Only an empty magazine accepts reload");
        for (const auto& record : row.records) {
            const auto& ammo = state.ammo[record.index];
            require(ammo.state == record.state && ammo.frame == record.frame && ammo.x == number(record.bits[0]) && ammo.y == number(record.bits[1])
                        && ammo.animation_time == number(record.bits[2]) && ammo.velocity_x == number(record.bits[3]) && ammo.velocity_y == number(record.bits[4]),
                "Source cartridge initialization including all 16 loaded positions");
        }
    }
}

void branch_and_reset() {
    for (const auto& row : branch_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.branches_hit = row.branch_hits;
        state.branch_ammo_granted = row.granted;
        state.reload_start_index = row.reload_start_index;
        const auto random = game::test::after_leaves(game::test::snapshot(state));
        tick(round);
        require(state.branch_ammo_granted == row.expected_granted && state.reload_start_index == row.expected_start_index && state.random_draws == random.draws
                    && state.random_state == random.state && state.score == 0,
            "Fourth branch overwrites reload start once, without points or RNG");
    }
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.branches_hit = 4;
    state.village.house_hit = true;
    state.score = 88;
    state.remaining = 62;
    tick(round, 0, {.pointer = {491, 390}, .fire_down = true});
    require(state.branch_ammo_granted && state.reload_start_index == 0, "Same-update rose trigger follows and overrides the branch grant");
    tick(round);
    require(state.reload_start_index == 0, "Already granted branch bonus cannot overwrite the rose on later updates");
    const game::Round fresh(1, 0);
    const auto& reset = fresh.state();
    require(reset.scene_capacity == 128 && reset.spawn_threshold == 50 && reset.reload_start_index == 8 && !reset.house88_mode && !reset.branch_ammo_granted
                && !reset.village.house_hit && reset.ammo_index == 8
                && std::ranges::all_of(std::span{reset.chickens}.subspan(128),
                    [](const auto& c) {
                        return c.type == -2;
                    }),
        "New round restores mode defaults and original inert extra backing");
}
} // namespace

int main() {
    try {
        trigger();
        cadence();
        allocator();
        reload();
        branch_and_reset();
        std::cout << "House88 trigger, cadence, scene backing, reload, branch overwrite and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
