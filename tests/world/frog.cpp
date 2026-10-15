#include "world/frog.hpp"

#include "support/numeric.hpp"
#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct ImpactExpectation {
    int points;
    world::FrogPopup popup;
};

struct UpdateCase {
    std::string id;
    world::Frog before;
    float delta;
    float camera;
    int recoil;
    std::optional<std::uint32_t> random;
    world::Frog after;
    std::optional<float> croak_pan;
};

struct ImpactCase {
    std::string id;
    world::Frog before;
    world::Frog after;
    ImpactExpectation effect;
};

struct DrawCase {
    std::string id;
    world::Frog state;
    std::array<int, 4> source;
    std::array<int, 2> destination;
    std::string image;
};

struct BonusCase {
    std::string id;
    int before_frog_hits;
    float before_remaining;
    int stone_hits;
    bool triggered;
    int after_frog_hits;
    std::uint32_t after_remaining_bits;
};

struct SequenceStep {
    std::string action;
    int stones;
    int frogs;
    float remaining;
    bool triggered;
};

struct SequenceCase {
    std::string id;
    int initial_stones;
    int initial_frogs;
    float initial_remaining;
    std::vector<SequenceStep> steps;
};

const world::Frog expected_initial_state = world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5};

const std::array<UpdateCase, 39> updates_cases = {{UpdateCase{"zero_time_still_consumes_random", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F,
                                                       640.0F, 0, 3U, world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 0.0F, 3.0F, 0, 0, 1, 5}, -1.0F},
    UpdateCase{"first_animation_update", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, binary32(0x3e000000U), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, binary32(0x40100000U), 3.0F, 0, 1, 1, 5}, std::nullopt},
    UpdateCase{"below_cell_boundary", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, binary32(0x3da3d709U), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, binary32(0x407ffffeU), 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"at_nominal_cell_boundary", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"large_time_only_one_cell", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, binary32(0x3f900000U), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, binary32(0x42510000U), 3.0F, 0, 1, 1, 5}, std::nullopt},
    UpdateCase{"oscillation_01", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 0, 1, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 1, 1, 5}, std::nullopt},
    UpdateCase{"oscillation_02", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 1, 1, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 2, 1, 5}, std::nullopt},
    UpdateCase{"oscillation_03", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 2, 1, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 3, 1, 5}, std::nullopt},
    UpdateCase{"oscillation_04", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 3, 1, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 4, 1, 5}, std::nullopt},
    UpdateCase{"oscillation_05", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 4, 1, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 5, 2, 5}, std::nullopt},
    UpdateCase{"oscillation_06", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 5, 2, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 4, 2, 5}, std::nullopt},
    UpdateCase{"oscillation_07", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 4, 2, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 3, 2, 5}, std::nullopt},
    UpdateCase{"oscillation_08", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 3, 2, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 2, 2, 5}, std::nullopt},
    UpdateCase{"oscillation_09", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 2, 2, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 1, 2, 5}, std::nullopt},
    UpdateCase{"oscillation_10", world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 1, 2, 5}, binary32(0x3da3d70aU), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"retained_sum_below4_but_store4", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, binary32(0x407fffffU), 3.0F, 0, 0, 1, 5}, binary32(0x31800000U), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, 4.0F, 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"retained_sum_above4", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, binary32(0x407fffffU), 3.0F, 0, 0, 1, 5}, binary32(0x32000000U), 640.0F, 0, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 330.0F, binary32(0x34100000U), 3.0F, 0, 1, 1, 5}, std::nullopt},
    UpdateCase{"croak_previous_pose_-1024", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 1280.0F, 0, 515U,
        world::Frog{11, 440.0F, 330.0F, -1608.0F, 330.0F, 0.0F, 3.0F, 0, 0, 1, 5}, -1.0F},
    UpdateCase{"croak_previous_pose_-380", world::Frog{11, 440.0F, 330.0F, -380.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 1280.0F, 0, 515U,
        world::Frog{11, 440.0F, 330.0F, -1608.0F, 330.0F, 0.0F, 3.0F, 0, 0, 1, 5}, -1.0F},
    UpdateCase{"croak_previous_pose_260", world::Frog{11, 440.0F, 330.0F, 260.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 1280.0F, 0, 515U,
        world::Frog{11, 440.0F, 330.0F, -1608.0F, 330.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F},
    UpdateCase{"croak_previous_pose_440", world::Frog{11, 440.0F, 330.0F, 440.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 1280.0F, 0, 515U,
        world::Frog{11, 440.0F, 330.0F, -1608.0F, 330.0F, 0.0F, 3.0F, 0, 0, 1, 5}, binary32(0x3e900000U)},
    UpdateCase{"croak_previous_pose_900", world::Frog{11, 440.0F, 330.0F, 900.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 1280.0F, 0, 515U,
        world::Frog{11, 440.0F, 330.0F, -1608.0F, 330.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 1.0F},
    UpdateCase{"croak_previous_pose_1280", world::Frog{11, 440.0F, 330.0F, 1280.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 1280.0F, 0, 515U,
        world::Frog{11, 440.0F, 330.0F, -1608.0F, 330.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 1.0F},
    UpdateCase{"camera_00000000", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 0.0F, 8, 0U,
        world::Frog{11, 440.0F, 330.0F, 440.0F, 338.0F, 0.0F, 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"camera_44200000", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 640.0F, 8, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 338.0F, 0.0F, 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"camera_44a00000", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, 1280.0F, 8, 0U,
        world::Frog{11, 440.0F, 330.0F, -1608.0F, 338.0F, 0.0F, 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"camera_441c0aaa", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, binary32(0x441c0aaaU), 8, 0U,
        world::Frog{11, 440.0F, 330.0F, -558.0F, 338.0F, 0.0F, 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"camera_442027ff", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5}, 0.0F, binary32(0x442027ffU), 8, 0U,
        world::Frog{11, 440.0F, 330.0F, -584.0F, 338.0F, 0.0F, 3.0F, 0, 0, 1, 5}, std::nullopt},
    UpdateCase{"death_00", world::Frog{51, 440.0F, 330.0F, -1024.0F, -1024.0F, 12.0F, 4.0F, 18, 12, 1, 5}, binary32(0x3d4ccccdU), 640.0F, 0, std::nullopt,
        world::Frog{51, 445.0F, 325.0F, -579.0F, 325.0F, 13.0F, 4.0F, 18, 13, 1, 5}, std::nullopt},
    UpdateCase{"death_01", world::Frog{51, 445.0F, 325.0F, -579.0F, 325.0F, 13.0F, 4.0F, 18, 13, 1, 5}, binary32(0x3d4ccccdU), 640.0F, 0, std::nullopt,
        world::Frog{51, 450.0F, 320.0F, -574.0F, 320.0F, 14.0F, 4.0F, 18, 14, 1, 5}, std::nullopt},
    UpdateCase{"death_02", world::Frog{51, 450.0F, 320.0F, -574.0F, 320.0F, 14.0F, 4.0F, 18, 14, 1, 5}, binary32(0x3d4ccccdU), 640.0F, 0, std::nullopt,
        world::Frog{51, 455.0F, 315.0F, -569.0F, 315.0F, 15.0F, 4.0F, 18, 15, 1, 5}, std::nullopt},
    UpdateCase{"death_03", world::Frog{51, 455.0F, 315.0F, -569.0F, 315.0F, 15.0F, 4.0F, 18, 15, 1, 5}, binary32(0x3d4ccccdU), 640.0F, 0, std::nullopt,
        world::Frog{51, 460.0F, 310.0F, -564.0F, 310.0F, 16.0F, 4.0F, 18, 16, 1, 5}, std::nullopt},
    UpdateCase{"death_04", world::Frog{51, 460.0F, 310.0F, -564.0F, 310.0F, 16.0F, 4.0F, 18, 16, 1, 5}, binary32(0x3d4ccccdU), 640.0F, 0, std::nullopt,
        world::Frog{51, 465.0F, 305.0F, -559.0F, 305.0F, 17.0F, 4.0F, 18, 17, 1, 5}, std::nullopt},
    UpdateCase{"death_05", world::Frog{51, 465.0F, 305.0F, -559.0F, 305.0F, 17.0F, 4.0F, 18, 17, 1, 5}, binary32(0x3d4ccccdU), 640.0F, 0, std::nullopt,
        world::Frog{-1, 470.0F, 300.0F, -554.0F, 300.0F, 18.0F, 4.0F, 18, 18, 1, 5}, std::nullopt},
    UpdateCase{"death_06", world::Frog{-1, 470.0F, 300.0F, -554.0F, 300.0F, 18.0F, 4.0F, 18, 18, 1, 5}, binary32(0x3d4ccccdU), 640.0F, 0, std::nullopt,
        world::Frog{-1, 470.0F, 300.0F, -554.0F, 300.0F, 18.0F, 4.0F, 18, 18, 1, 5}, std::nullopt},
    UpdateCase{"death_zero_delta", world::Frog{51, 440.0F, 330.0F, -1024.0F, -1024.0F, 12.0F, 4.0F, 18, 12, 1, 5}, 0.0F, 640.0F, 0, std::nullopt,
        world::Frog{51, 440.0F, 330.0F, -584.0F, 330.0F, 12.0F, 4.0F, 18, 12, 1, 5}, std::nullopt},
    UpdateCase{"death_large_delta_removes", world::Frog{51, 440.0F, 330.0F, -1024.0F, -1024.0F, 12.0F, 4.0F, 18, 12, 1, 5}, binary32(0x3f900000U), 640.0F, 0, std::nullopt,
        world::Frog{-1, binary32(0x440a2000U), binary32(0x43598000U), binary32(0xc3ebc000U), binary32(0x43598000U), binary32(0x420a0000U), 4.0F, 18, 34, 1, 5}, std::nullopt},
    UpdateCase{"death_retained_frame_before18_stores18", world::Frog{51, 440.0F, 330.0F, -1024.0F, -1024.0F, binary32(0x418fffffU), 4.0F, 18, 12, 1, 5}, binary32(0x33800000U),
        640.0F, 0, std::nullopt, world::Frog{51, 440.0F, 330.0F, -584.0F, 330.0F, 18.0F, 4.0F, 18, 17, 1, 5}, std::nullopt},
    UpdateCase{"death_exact_frame18_removes", world::Frog{51, 440.0F, 330.0F, -1024.0F, -1024.0F, binary32(0x418c0000U), 4.0F, 18, 12, 1, 5}, binary32(0x3ccccccdU), 640.0F, 0,
        std::nullopt, world::Frog{-1, binary32(0x43dd4000U), binary32(0x43a3c000U), binary32(0xc4116000U), binary32(0x43a3c000U), 18.0F, 4.0F, 18, 18, 1, 5}, std::nullopt}}};

const std::array<ImpactCase, 2> impacts_cases = {
    {ImpactCase{"ordinary_initial", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 0, 0, 1, 5},
         world::Frog{51, 440.0F, 330.0F, -1024.0F, -1024.0F, 12.0F, 4.0F, 18, 12, 1, 5}, ImpactExpectation{5, world::FrogPopup{490.0F, 370.0F, 5}}},
        ImpactCase{"ordinary_late_animation", world::Frog{11, 440.0F, 330.0F, -1024.0F, -1024.0F, binary32(0x40200000U), 3.0F, 0, 5, 2, 5},
            world::Frog{51, 440.0F, 330.0F, -1024.0F, -1024.0F, 12.0F, 4.0F, 18, 12, 2, 5}, ImpactExpectation{5, world::FrogPopup{490.0F, 370.0F, 5}}}}};

const std::array<DrawCase, 12> draws_cases = {{DrawCase{"cell_00", world::Frog{11, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 0.0F, 3.0F, 0, 0, 1, 5},
                                                   {{0, 0, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_01", world::Frog{11, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 0.0F, 3.0F, 0, 1, 1, 5}, {{120, 0, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_02", world::Frog{11, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 0.0F, 3.0F, 0, 2, 1, 5}, {{240, 0, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_03", world::Frog{11, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 0.0F, 3.0F, 0, 3, 1, 5}, {{360, 0, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_04", world::Frog{11, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 0.0F, 3.0F, 0, 4, 1, 5}, {{480, 0, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_05", world::Frog{11, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 0.0F, 3.0F, 0, 5, 1, 5}, {{600, 0, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_12", world::Frog{51, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 12.0F, 4.0F, 18, 12, 1, 5}, {{0, 160, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_13", world::Frog{51, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 13.0F, 4.0F, 18, 13, 1, 5}, {{120, 160, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_14", world::Frog{51, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 14.0F, 4.0F, 18, 14, 1, 5}, {{240, 160, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_15", world::Frog{51, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 15.0F, 4.0F, 18, 15, 1, 5}, {{360, 160, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{"cell_16", world::Frog{51, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 16.0F, 4.0F, 18, 16, 1, 5}, {{480, 160, 120, 80}}, {{-12, 330}}, "gfx/frog"},
    DrawCase{
        "cell_17", world::Frog{51, 440.0F, 330.0F, binary32(0xc14c0000U), binary32(0x43a56000U), 17.0F, 4.0F, 18, 17, 1, 5}, {{600, 160, 120, 80}}, {{-12, 330}}, "gfx/frog"}}};

const std::array<BonusCase, 13> bonuses_cases = {{BonusCase{"bonus_s4_f1_t426fffff", 1, 59.999996185302734F, 4, true, 2, 0x42820000U},
    BonusCase{"bonus_s4_f1_t42700000", 1, 60.0F, 4, false, 1, 0x42700000U}, BonusCase{"bonus_s4_f1_t42700001", 1, 60.000003814697266F, 4, false, 1, 0x42700001U},
    BonusCase{"bonus_s4_f1_t00000000", 1, 0.0F, 4, true, 2, 0x40a00000U}, BonusCase{"bonus_s4_f1_tbf800000", 1, -1.0F, 4, true, 2, 0x40800000U},
    BonusCase{"bonus_s4_f1_t426e0000", 1, 59.5F, 4, true, 2, 0x42810000U}, BonusCase{"bonus_s0_f1_t41f00000", 1, 30.0F, 0, false, 1, 0x41f00000U},
    BonusCase{"bonus_s3_f1_t41f00000", 1, 30.0F, 3, false, 1, 0x41f00000U}, BonusCase{"bonus_s5_f1_t41f00000", 1, 30.0F, 5, false, 1, 0x41f00000U},
    BonusCase{"bonus_s8_f1_t41f00000", 1, 30.0F, 8, false, 1, 0x41f00000U}, BonusCase{"bonus_s4_f0_t41f00000", 0, 30.0F, 4, false, 0, 0x41f00000U},
    BonusCase{"bonus_s4_f2_t41f00000", 2, 30.0F, 4, false, 2, 0x41f00000U}, BonusCase{"bonus_s4_f3_t41f00000", 3, 30.0F, 4, false, 3, 0x41f00000U}}};

const std::array<SequenceCase, 4> sequences_cases = {
    {SequenceCase{"frog_first_stones_reach_four_later", 0, 0, 55.0F,
         {SequenceStep{"frog_hit", 0, 1, 55.0F, false}, SequenceStep{"stone_hit", 1, 1, 55.0F, false}, SequenceStep{"stone_hit", 2, 1, 55.0F, false},
             SequenceStep{"stone_hit", 3, 1, 55.0F, false}, SequenceStep{"stone_hit", 4, 1, 55.0F, false}, SequenceStep{"special_update", 4, 2, 60.0F, true},
             SequenceStep{"special_update", 4, 2, 60.0F, false}}},
        SequenceCase{"four_stones_first_frog_later", 4, 0, 55.0F,
            {SequenceStep{"special_update", 4, 0, 55.0F, false}, SequenceStep{"frog_hit", 4, 1, 55.0F, false}, SequenceStep{"special_update", 4, 2, 60.0F, true},
                SequenceStep{"reset", 0, 0, 90.0F, false}}},
        SequenceCase{"no_deadline_latch_at60", 4, 1, 60.0F,
            {SequenceStep{"special_update", 4, 1, 60.0F, false}, SequenceStep{"clock_tick", 4, 1, 59.5F, false}, SequenceStep{"special_update", 4, 2, 64.5F, true}}},
        SequenceCase{"fifth_stone_before_special_cancels", 4, 1, 55.0F,
            {SequenceStep{"stone_hit", 5, 1, 55.0F, false}, SequenceStep{"special_update", 5, 1, 55.0F, false}, SequenceStep{"reset", 0, 0, 90.0F, false}}}}};

void original_updates() {
    for (const auto& expected : updates_cases) {
        auto frog = expected.before;
        const auto effect = world::update_frog(frog, expected.delta, expected.camera, expected.recoil, expected.random);
        const auto context = expected.id;
        require(frog == expected.after, context + ": original update fields differ");
        require(effect.croak_pan.has_value() == expected.croak_pan.has_value(), context + ": croak guard differs");
        if (effect.croak_pan) {
            require(std::bit_cast<std::uint32_t>(*effect.croak_pan) == std::bit_cast<std::uint32_t>(*expected.croak_pan), context + ": prior pose or stored pan differs");
        }
    }
}

void original_impacts_and_draws() {
    for (const auto& expected : impacts_cases) {
        auto frog = expected.before;
        const auto effect = world::hit_frog(frog);
        const auto& popup = expected.effect.popup;
        require(frog == expected.after && effect.points == expected.effect.points && effect.popup == world::FrogPopup{popup.world_x, popup.world_y, popup.points},
            expected.id + ": original impact award, popup or death transition differs");
    }
    for (const auto& expected : draws_cases) {
        const auto draw = world::frog_draw(expected.state);
        const auto& rect = expected.source;
        const auto& point = expected.destination;
        require(draw.image == expected.image && draw.source.x == rect[0] && draw.source.y == rect[1] && draw.source.width == rect[2] && draw.source.height == rect[3]
                    && draw.destination.x == point[0] && draw.destination.y == point[1] && draw.color_key && !draw.mirror_x,
            expected.id + ": original sprite cell or pose differs");
    }
}

void original_bonuses_and_sequences() {
    for (const auto& expected : bonuses_cases) {
        int frog_hits = expected.before_frog_hits;
        float remaining = expected.before_remaining;
        const bool triggered = world::apply_frog_time_bonus(frog_hits, expected.stone_hits, remaining);
        require(triggered == expected.triggered && frog_hits == expected.after_frog_hits && std::bit_cast<std::uint32_t>(remaining) == expected.after_remaining_bits,
            expected.id + ": original bonus guard, counter or stored time differs");
    }
    for (const auto& expected : sequences_cases) {
        int stones = expected.initial_stones;
        int frogs = expected.initial_frogs;
        float remaining = expected.initial_remaining;
        world::Frog frog;
        for (const auto& step : expected.steps) {
            const auto action = step.action;
            bool triggered = false;
            if (action == "frog_hit") {
                static_cast<void>(world::hit_frog(frog));
                ++frogs;
            } else if (action == "stone_hit") {
                ++stones;
            } else if (action == "clock_tick") {
                remaining -= 0.5F;
            } else if (action == "special_update") {
                triggered = world::apply_frog_time_bonus(frogs, stones, remaining);
            } else if (action == "reset") {
                stones = 0;
                frogs = 0;
                remaining = 90;
                frog = world::Frog{};
            } else {
                throw std::runtime_error("Unknown source sequence action");
            }
            require(stones == step.stones && frogs == step.frogs && remaining == step.remaining && triggered == step.triggered,
                expected.id + ": original missing prerequisite, order, deadline or reset differs");
        }
    }
}

void rejected_inputs() {
    world::Frog frog;
    const auto before = frog;
    const auto reject = [&](auto action) {
        bool rejected = false;
        try {
            action();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && frog == before, "Invalid frog input must fail before changing its state");
    };
    reject([&] {
        static_cast<void>(world::update_frog(frog, 0, 640, 0));
    });
    reject([&] {
        static_cast<void>(world::update_frog(frog, -1, 640, 0, 0));
    });
    reject([&] {
        static_cast<void>(world::update_frog(frog, 0, 1281, 0, 0));
    });
    reject([&] {
        static_cast<void>(world::update_frog(frog, 0, 640, 0, 32768));
    });
    static_cast<void>(world::hit_frog(frog));
    const auto dying = frog;
    bool rejected = false;
    try {
        static_cast<void>(world::hit_frog(frog));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && frog == dying, "A repeated frog impact must be rejected immediately, including before the first death update");
    rejected = false;
    try {
        static_cast<void>(world::update_frog(frog, 0, 640, 0, 0));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && frog == dying, "The death update must not consume a CRT draw");
    static_cast<void>(world::update_frog(frog, 1, 640, 0));
    require(frog.type == -1, "Completed frog death must release its scene slot");
    rejected = false;
    try {
        static_cast<void>(world::frog_draw(frog));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "Removed frog must no longer draw");
    int hits = 1;
    float remaining = 55;
    rejected = false;
    try {
        static_cast<void>(world::apply_frog_time_bonus(hits, -1, remaining));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && hits == 1 && remaining == 55, "Invalid bonus prerequisites must not change counters or clock");
}

} // namespace

int main() {
    try {
        require(world::Frog{} == expected_initial_state, "Default frog must match original round initializer");
        original_updates();
        original_impacts_and_draws();
        original_bonuses_and_sequences();
        rejected_inputs();
        std::cout << "Original frog animation, prior-pose croak, jump/removal, sprite cells, four-stone bonus, house suppression and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
