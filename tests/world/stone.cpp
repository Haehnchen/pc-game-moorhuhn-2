#include "world/stone.hpp"

#include "support/numeric.hpp"
#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace moorhuhn2;

using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct VelocityCase {
    std::string id;
    std::array<std::uint32_t, 2> random_values;
    std::uint32_t velocity_x_bits;
    std::uint32_t velocity_y_bits;
};

struct SeededImpactExpectation {
    std::array<std::uint32_t, 2> random_values;
    float velocity_x;
    float velocity_y;
};

struct SeededImpactCase {
    std::string id;
    SeededImpactExpectation next_stone_hit;
};

struct UpdateCase {
    std::string id;
    world::Stone before;
    float delta_seconds;
    float camera;
    int recoil;
    world::Stone after;
};

struct UpdateStep {
    float delta_seconds;
    world::Stone after;
};

struct SequenceCase {
    std::string id;
    world::Stone before;
    float camera;
    int recoil;
    std::vector<UpdateStep> steps;
};

const std::array<VelocityCase, 5> velocities_cases = {{VelocityCase{"minimum_horizontal_minimum_fall", {{0U, 0U}}, 0xbf800000U, 0xbf000000U},
    VelocityCase{"zero_horizontal_maximum_fall", {{10U, 19U}}, 0x00000000U, 0xc019999aU}, VelocityCase{"maximum_horizontal", {{19U, 10U}}, 0x3f666667U, 0xbfc00000U},
    VelocityCase{"crt_maximum", {{32767U, 32767U}}, 0xbe99999aU, 0xbf99999aU}, VelocityCase{"modulo_boundary", {{20U, 20U}}, 0xbf800000U, 0xbf000000U}}};

const std::array<SeededImpactCase, 4> initializers_cases = {{SeededImpactCase{"round_seed_0", SeededImpactExpectation{{{6388U, 21255U}}, -0.20000000298023224F, -2.0F}},
    SeededImpactCase{"round_seed_1", SeededImpactExpectation{{{27348U, 23199U}}, -0.20000000298023224F, -2.4000000953674316F}},
    SeededImpactCase{"round_seed_42", SeededImpactExpectation{{{1991U, 4593U}}, 0.10000000149011612F, -1.8000000715255737F}},
    SeededImpactCase{"round_seed_12345", SeededImpactExpectation{{{27089U, 31724U}}, -0.10000000149011612F, -0.9000000357627869F}}}};

const std::array<UpdateCase, 15> updates_cases = {{UpdateCase{"live_static_large_delta", world::Stone{10, 1200.0F, 380.0F, 176.0F, 380.0F, 0.0F, 0.0F, 0.0F, 0}, 90.0F, 640.0F, 0,
                                                       world::Stone{10, 1200.0F, 380.0F, 176.0F, 380.0F, 0.0F, 0.0F, 0.0F, 0}},
    UpdateCase{"positive_velocity_quarter_second", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3e800000U),
        640.0F, 0,
        world::Stone{50, binary32(0x44961e00U), binary32(0x43bda000U), binary32(0x4330f000U), binary32(0x43bda000U), binary32(0x3f700000U), binary32(0xbf400000U), 5.0F, 5}},
    UpdateCase{"negative_velocity_quarter_second", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0xbf000000U), binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3e800000U),
        640.0F, 0,
        world::Stone{50, binary32(0x4495ea00U), binary32(0x43bda000U), binary32(0x432f5000U), binary32(0x43bda000U), binary32(0xbf300000U), binary32(0xbf400000U), 5.0F, 5}},
    UpdateCase{"zero_velocity_accelerates_left", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, 0.0F, binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3e800000U), 640.0F, 0,
        world::Stone{50, binary32(0x4495fa00U), binary32(0x43bda000U), binary32(0x432fd000U), binary32(0x43bda000U), binary32(0xbe400000U), binary32(0xbf400000U), 5.0F, 5}},
    UpdateCase{"zero_delta_still_moves_position", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}, 0.0F, 640.0F, 0,
        world::Stone{50, binary32(0x44961800U), binary32(0x43bd4000U), binary32(0x4330c000U), binary32(0x43bd4000U), binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}},
    UpdateCase{"fractional_camera_below_integer", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3e000000U),
        binary32(0x42c13fffU), 0,
        world::Stone{50, binary32(0x44961b00U), binary32(0x43bd7000U), binary32(0x4482db00U), binary32(0x43bd7000U), binary32(0x3f580000U), binary32(0xbf900000U),
            binary32(0x40200000U), 2}},
    UpdateCase{"fractional_camera_exact_input", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3e000000U),
        binary32(0x42c94000U), 8,
        world::Stone{50, binary32(0x44961b00U), binary32(0x43bd7000U), binary32(0x4481fb00U), binary32(0x43c17000U), binary32(0x3f580000U), binary32(0xbf900000U),
            binary32(0x40200000U), 2}},
    UpdateCase{"screen_y_stored_before_clamp", world::Stone{50, 1200.0F, 480.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0x3e800000U), 0.0F, 0}, 0.0F, 640.0F, 0,
        world::Stone{50, binary32(0x44961800U), 500.0F, binary32(0x4330c000U), binary32(0x43f02000U), binary32(0x3f400000U), binary32(0x3e800000U), 0.0F, 0}},
    UpdateCase{"exact_480_does_not_clamp", world::Stone{50, 1200.0F, binary32(0x43efe000U), 176.0F, 380.0F, binary32(0x3f400000U), binary32(0x3e800000U), 0.0F, 0}, 0.0F, 640.0F, 0,
        world::Stone{50, binary32(0x44961800U), 480.0F, binary32(0x4330c000U), 480.0F, binary32(0x3f400000U), binary32(0x3e800000U), 0.0F, 0}},
    UpdateCase{"offscreen_still_updates_retains_slot", world::Stone{50, 1200.0F, 500.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0x3e800000U), 20.0F, 20},
        binary32(0x3e800000U), 640.0F, 0, world::Stone{50, binary32(0x44961e00U), 500.0F, binary32(0x4330f000U), 501.0F, binary32(0x3f700000U), 1.0F, 0.0F, 0}},
    UpdateCase{"animation_below_frame_one", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3d4cccccU), 640.0F,
        0,
        world::Stone{50, binary32(0x44961933U), binary32(0x43bd5333U), binary32(0x4330c998U), binary32(0x43bd5333U), binary32(0x3f49999aU), binary32(0xbfaccccdU),
            binary32(0x3f7fffffU), 0}},
    UpdateCase{"animation_frame_one", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3d4ccccdU), 640.0F, 0,
        world::Stone{50, binary32(0x44961933U), binary32(0x43bd5333U), binary32(0x4330c998U), binary32(0x43bd5333U), binary32(0x3f49999aU), binary32(0xbfaccccdU), 1.0F, 1}},
    UpdateCase{"animation_1_05f_does_not_wrap", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3f866666U),
        640.0F, 0,
        world::Stone{50, binary32(0x44963133U), binary32(0x43bed333U), binary32(0x43318998U), binary32(0x43bed333U), binary32(0x3fc4ccccU), binary32(0x3fd33332U), 21.0F, 20}},
    UpdateCase{"animation_next_float_wraps", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), 0.0F, 0}, binary32(0x3f866667U),
        640.0F, 0,
        world::Stone{50, binary32(0x44963133U), binary32(0x43bed333U), binary32(0x43318998U), binary32(0x43bed333U), binary32(0x3fc4cccdU), binary32(0x3fd33335U), 0.0F, 0}},
    UpdateCase{"animation_large_excess_discarded", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, binary32(0x3f400000U), binary32(0xbfc00000U), binary32(0x41a60000U), 20},
        binary32(0x40080000U), 640.0F, 0,
        world::Stone{50, binary32(0x44964b00U), binary32(0x43c07000U), binary32(0x43325800U), binary32(0x43c07000U), binary32(0x40160000U), binary32(0x409c0000U), 0.0F, 0}}}};

const std::array<SequenceCase, 4> sequences_cases = {{SequenceCase{"one_one_second_update", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, 0.75F, -1.5F, 0.0F, 0}, 640.0F, 0,
                                                          {UpdateStep{1.0F, world::Stone{50, binary32(0x44963000U), binary32(0x43bec000U), binary32(0x43318000U),
                                                                                binary32(0x43bec000U), binary32(0x3fc00000U), binary32(0x3fc00000U), 20.0F, 20}}}},
    SequenceCase{"two_half_second_updates", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, 0.75F, -1.5F, 0.0F, 0}, 640.0F, 0,
        {UpdateStep{0.5F, world::Stone{50, binary32(0x44962400U), 380.0F, binary32(0x43312000U), 380.0F, binary32(0x3f900000U), 0.0F, 10.0F, 10}},
            UpdateStep{0.5F, world::Stone{50, binary32(0x44965400U), binary32(0x43bec000U), binary32(0x4332a000U), binary32(0x43bec000U), binary32(0x3fc00000U),
                                 binary32(0x3fc00000U), 20.0F, 20}}}},
    SequenceCase{"dying_even_offscreen_32_updates", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, 0.75F, -1.5F, 0.0F, 0}, 640.0F, 0,
        {UpdateStep{0.25F,
             world::Stone{50, binary32(0x44961e00U), binary32(0x43bda000U), binary32(0x4330f000U), binary32(0x43bda000U), binary32(0x3f700000U), binary32(0xbf400000U), 5.0F, 5}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44964200U), binary32(0x43bda000U), binary32(0x43321000U), binary32(0x43bda000U), binary32(0x3f900000U), 0.0F, 10.0F, 10}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44966c00U), 380.0F, binary32(0x43336000U), 380.0F, binary32(0x3fa80000U), binary32(0x3f400000U), 15.0F, 15}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44969c00U), binary32(0x43bec000U), binary32(0x4334e000U), binary32(0x43bec000U), binary32(0x3fc00000U),
                                  binary32(0x3fc00000U), 20.0F, 20}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x4496d200U), binary32(0x43bfe000U), binary32(0x43369000U), binary32(0x43bfe000U), binary32(0x3fd80000U),
                                  binary32(0x40100000U), 0.0F, 0}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44970e00U), binary32(0x43c16000U), binary32(0x43387000U), binary32(0x43c16000U), binary32(0x3ff00000U), 3.0F, 5.0F, 5}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44975000U), binary32(0x43c34000U), binary32(0x433a8000U), binary32(0x43c34000U), binary32(0x40040000U),
                                  binary32(0x40700000U), 10.0F, 10}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44979800U), 395.0F, binary32(0x433cc000U), 395.0F, binary32(0x40100000U), binary32(0x40900000U), 15.0F, 15}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x4497e600U), binary32(0x43c82000U), binary32(0x433f3000U), binary32(0x43c82000U), binary32(0x401c0000U),
                                  binary32(0x40a80000U), 20.0F, 20}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44983a00U), binary32(0x43cb2000U), binary32(0x4341d000U), binary32(0x43cb2000U), binary32(0x40280000U), 6.0F, 0.0F, 0}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44989400U), 413.0F, binary32(0x4344a000U), 413.0F, binary32(0x40340000U), binary32(0x40d80000U), 5.0F, 5}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x4498f400U), binary32(0x43d24000U), binary32(0x4347a000U), binary32(0x43d24000U), 3.0F, binary32(0x40f00000U), 10.0F, 10}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44995a00U), binary32(0x43d66000U), binary32(0x434ad000U), binary32(0x43d66000U), binary32(0x404c0000U),
                                  binary32(0x41040000U), 15.0F, 15}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x4499c600U), binary32(0x43dae000U), binary32(0x434e3000U), binary32(0x43dae000U), binary32(0x40580000U), 9.0F, 20.0F, 20}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x449a3800U), binary32(0x43dfc000U), binary32(0x4351c000U), binary32(0x43dfc000U), binary32(0x40640000U),
                                  binary32(0x411c0000U), 0.0F, 0}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x449ab000U), 458.0F, binary32(0x43558000U), 458.0F, binary32(0x40700000U), binary32(0x41280000U), 5.0F, 5}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x449b2e00U), binary32(0x43eaa000U), binary32(0x43597000U), binary32(0x43eaa000U), binary32(0x407c0000U),
                                  binary32(0x41340000U), 10.0F, 10}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x449bb200U), 500.0F, binary32(0x435d9000U), binary32(0x43f0a000U), binary32(0x40840000U), 12.0F, 15.0F, 15}},
            UpdateStep{
                0.25F, world::Stone{50, binary32(0x449c3c00U), 500.0F, binary32(0x4361e000U), binary32(0x44003000U), binary32(0x408a0000U), binary32(0x414c0000U), 20.0F, 20}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x449ccc00U), 500.0F, binary32(0x43666000U), binary32(0x44006000U), binary32(0x40900000U), binary32(0x41580000U), 0.0F, 0}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x449d6200U), 500.0F, binary32(0x436b1000U), binary32(0x44009000U), binary32(0x40960000U), binary32(0x41640000U), 5.0F, 5}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x449dfe00U), 500.0F, binary32(0x436ff000U), 515.0F, binary32(0x409c0000U), 15.0F, 10.0F, 10}},
            UpdateStep{0.25F, world::Stone{50, 1269.0F, 500.0F, 245.0F, binary32(0x4400f000U), binary32(0x40a20000U), binary32(0x417c0000U), 15.0F, 15}},
            UpdateStep{
                0.25F, world::Stone{50, binary32(0x449f4800U), 500.0F, binary32(0x437a4000U), binary32(0x44012000U), binary32(0x40a80000U), binary32(0x41840000U), 20.0F, 20}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x449ff600U), 500.0F, binary32(0x437fb000U), binary32(0x44015000U), binary32(0x40ae0000U), binary32(0x418a0000U), 0.0F, 0}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44a0aa00U), 500.0F, binary32(0x4382a800U), 518.0F, binary32(0x40b40000U), 18.0F, 5.0F, 5}},
            UpdateStep{
                0.25F, world::Stone{50, binary32(0x44a16400U), 500.0F, binary32(0x43859000U), binary32(0x4401b000U), binary32(0x40ba0000U), binary32(0x41960000U), 10.0F, 10}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44a22400U), 500.0F, binary32(0x43889000U), binary32(0x4401e000U), 6.0F, binary32(0x419c0000U), 15.0F, 15}},
            UpdateStep{
                0.25F, world::Stone{50, binary32(0x44a2ea00U), 500.0F, binary32(0x438ba800U), binary32(0x44021000U), binary32(0x40c60000U), binary32(0x41a20000U), 20.0F, 20}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44a3b600U), 500.0F, binary32(0x438ed800U), 521.0F, binary32(0x40cc0000U), 21.0F, 0.0F, 0}},
            UpdateStep{0.25F, world::Stone{50, binary32(0x44a48800U), 500.0F, binary32(0x43922000U), binary32(0x44027000U), binary32(0x40d20000U), binary32(0x41ae0000U), 5.0F, 5}},
            UpdateStep{0.25F, world::Stone{50, 1323.0F, 500.0F, 299.0F, binary32(0x4402a000U), binary32(0x40d80000U), binary32(0x41b40000U), 10.0F, 10}}}},
    SequenceCase{"rounded_animation_21_then_zero_delta", world::Stone{50, 1200.0F, 380.0F, 176.0F, 380.0F, 0.75F, -1.5F, 0.0F, 0}, 640.0F, 0,
        {UpdateStep{1.0499999523162842F,
             world::Stone{50, binary32(0x44963133U), binary32(0x43bed333U), binary32(0x43318998U), binary32(0x43bed333U), binary32(0x3fc4ccccU), binary32(0x3fd33332U), 21.0F, 20}},
            UpdateStep{0.0F, world::Stone{50, binary32(0x44966266U), binary32(0x43bfa666U), binary32(0x43331330U), binary32(0x43bfa666U), binary32(0x3fc4ccccU),
                                 binary32(0x3fd33332U), 0.0F, 0}}}}}};

void check_float_bits(float actual, std::uint32_t expected, std::string_view context) {
    require(std::bit_cast<std::uint32_t>(actual) == expected, context);
}

void check_state(const world::Stone& actual, const world::Stone& expected, std::string_view context) {
    require(actual == expected, std::string(context) + ": original state differs");
    require(std::bit_cast<std::uint32_t>(actual.world_x) == std::bit_cast<std::uint32_t>(expected.world_x)
                && std::bit_cast<std::uint32_t>(actual.world_y) == std::bit_cast<std::uint32_t>(expected.world_y)
                && std::bit_cast<std::uint32_t>(actual.screen_x) == std::bit_cast<std::uint32_t>(expected.screen_x)
                && std::bit_cast<std::uint32_t>(actual.screen_y) == std::bit_cast<std::uint32_t>(expected.screen_y)
                && std::bit_cast<std::uint32_t>(actual.velocity_x) == std::bit_cast<std::uint32_t>(expected.velocity_x)
                && std::bit_cast<std::uint32_t>(actual.velocity_y) == std::bit_cast<std::uint32_t>(expected.velocity_y)
                && std::bit_cast<std::uint32_t>(actual.animation_time) == std::bit_cast<std::uint32_t>(expected.animation_time),
        std::string(context) + ": original binary32 words differ");
}

void original_impacts() {
    const auto live = updates_cases.front().before;
    for (const auto& expected : velocities_cases) {
        auto stone = live;
        world::hit_stone(stone, expected.random_values[0], expected.random_values[1]);
        const auto context = expected.id;
        check_float_bits(stone.velocity_x, expected.velocity_x_bits, context + ": horizontal impact velocity differs");
        check_float_bits(stone.velocity_y, expected.velocity_y_bits, context + ": vertical impact velocity differs");
        require(stone.type == 50 && stone.frame == 0 && stone.animation_time == 0, context + ": impact must enter dying frame0");
        require(stone.world_x == live.world_x && stone.world_y == live.world_y && stone.screen_x == live.screen_x && stone.screen_y == live.screen_y,
            context + ": impact must retain the composed pose");
        const auto dying = stone;
        bool rejected = false;
        try {
            world::hit_stone(stone, 0, 0);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && stone == dying, context + ": a dying stone cannot receive another impact");
    }
    for (const auto& initial : initializers_cases) {
        const auto& expected = initial.next_stone_hit;
        auto stone = live;
        world::hit_stone(stone, expected.random_values[0], expected.random_values[1]);
        require(stone.velocity_x == expected.velocity_x && stone.velocity_y == expected.velocity_y, initial.id + ": original seeded impact velocity differs");
    }
}

void original_updates() {
    for (const auto& expected : updates_cases) {
        auto stone = expected.before;
        world::update_stone(stone, expected.delta_seconds, expected.camera, expected.recoil);
        check_state(stone, expected.after, expected.id);
    }
    for (const auto& sequence : sequences_cases) {
        auto stone = sequence.before;
        std::size_t index = 0;
        for (const auto& expected : sequence.steps) {
            world::update_stone(stone, expected.delta_seconds, sequence.camera, sequence.recoil);
            check_state(stone, expected.after, sequence.id + " step " + std::to_string(index++));
        }
    }
}

void drawing_and_live_state() {
    world::Stone stone;
    require(stone.type == 10 && stone.frame == 0 && stone.animation_time == 0, "New stone must be live at unanimated frame0");
    stone.world_x = 123.25F;
    stone.world_y = 345.5F;
    stone.velocity_x = 1.25F;
    stone.velocity_y = -2.0F;
    world::update_stone(stone, 90.0F, 80.0F, 8);
    require(stone.world_x == 123.25F && stone.world_y == 345.5F && stone.screen_x == -4.75F && stone.screen_y == 353.5F && stone.velocity_x == 1.25F && stone.velocity_y == -2.0F
                && stone.frame == 0 && stone.animation_time == 0,
        "Live stone updates only its stored screen pose");
    const auto live_draw = world::stone_draw(stone);
    require(live_draw.image == "gfx/stone" && live_draw.source.x == 0 && live_draw.source.y == 0 && live_draw.source.width == 146 && live_draw.source.height == 110
                && live_draw.destination.x == -4 && live_draw.destination.y == 353 && live_draw.color_key && !live_draw.mirror_x,
        "Live draw uses frame0, original cell dimensions and truncation toward zero");

    stone.type = 50;
    stone.frame = 20;
    stone.screen_x = -12.75F;
    stone.screen_y = 381.75F;
    const auto draw = world::stone_draw(stone);
    require(draw.image == "gfx/stone" && draw.source.x == 0 && draw.source.y == 2200 && draw.source.width == 146 && draw.source.height == 110 && draw.destination.x == -12
                && draw.destination.y == 381 && draw.color_key && !draw.mirror_x,
        "Dying draw selects the final vertical sheet cell and stored screen pose");
    stone.frame = 21;
    bool rejected = false;
    try {
        static_cast<void>(world::stone_draw(stone));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "Drawing must reject a frame outside the original 21-cell sheet");
}

} // namespace

int main() {
    try {
        original_impacts();
        original_updates();
        drawing_and_live_state();
        std::cout << "Original stone impact, binary32 motion, camera, frame boundary, offscreen retention and draw vectors passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
