#include "world/spider.hpp"

#include "support/numeric.hpp"
#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
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
    bool swings_sound;
    std::optional<world::SpiderPopup> popup;
};

struct UpdateCase {
    std::string id;
    world::Spider before;
    float delta;
    float camera;
    int recoil;
    world::Spider after;
};

struct ImpactCase {
    std::string id;
    world::Spider before;
    int before_hits;
    bool special_ammunition;
    world::Spider after;
    int after_hits;
    ImpactExpectation effect;
};

struct BonusCase {
    int before_hits;
    bool before_latch;
    world::SpiderBonus effect;
    int after_hits;
    bool after_latch;
};

struct DrawCase {
    std::string id;
    world::Spider state;
    bool collision;
    std::array<int, 4> source;
    std::array<int, 2> destination;
    std::string image;
};

struct SequenceStep {
    std::string action;
    int count;
    float delta;
    bool special;
    world::SpiderBonus effect;
    world::Spider state;
    int hits;
    bool latch;
    int score;
};

struct SequenceCase {
    std::string id;
    std::vector<SequenceStep> steps;
};

const world::Spider expected_initial_state = world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50};

const std::array<UpdateCase, 38> updates_cases = {{UpdateCase{"startup_zero", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, 0.0F,
                                                       640.0F, 0, world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}},
    UpdateCase{"startup_hold", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, 4.0F, 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 8, 1.0F, 1.0F, 50}},
    UpdateCase{"startup_just_below5", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, binary32(0x409fffffU), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 8, binary32(0x35000000U), 1.0F, 50}},
    UpdateCase{"startup_exact5", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, 5.0F, 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 247.0F, 3.0F, 25, 7, 0.0F, 1.0F, 50}},
    UpdateCase{"startup_above5", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, binary32(0x40a80000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x4381c000U), 3.0F, 25, 7, binary32(0xbe800000U), 1.0F, 50}},
    UpdateCase{"startup_huge_one_cell", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, 100.0F, 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 4997.0F, 3.0F, 25, 7, -95.0F, 1.0F, 50}},
    UpdateCase{"retract_frame8_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 8, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 7, binary32(0xbd800000U), 1.0F, 50}},
    UpdateCase{"retract_frame7_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 7, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 6, binary32(0xbd800000U), 1.0F, 50}},
    UpdateCase{"retract_frame6_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 6, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 5, binary32(0xbd800000U), 1.0F, 50}},
    UpdateCase{"retract_frame5_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 5, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 4, binary32(0xbd800000U), 1.0F, 50}},
    UpdateCase{"retract_frame4_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 4, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 3, binary32(0xbd800000U), 1.0F, 50}},
    UpdateCase{"retract_frame3_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 3, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 2, binary32(0xbd800000U), 1.0F, 50}},
    UpdateCase{"retract_frame2_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 2, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 1, binary32(0xbd800000U), 1.0F, 50}},
    UpdateCase{"retract_frame1_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 1, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 0, binary32(0xbd800000U), 1.0F, 50}},
    UpdateCase{"retract_frame0_one_step", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 0, 0.0F, 0.0F, 50}},
    UpdateCase{"retract_stored_threshold_below", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x403fffffU), 3.0F, 25, 8, 0.0F, 1.0F, 50}, binary32(0x31a18937U),
        640.0F, 0, world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 3.0F, 3.0F, 25, 8, binary32(0xb1a18937U), 1.0F, 50}},
    UpdateCase{"retract_exact_threshold", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x40200000U), 3.0F, 25, 8, 0.0F, 1.0F, 50}, binary32(0x3c23d70aU), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 3.0F, 3.0F, 25, 8, binary32(0xbc23d70aU), 1.0F, 50}},
    UpdateCase{"retract_above_threshold", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x40200000U), 3.0F, 25, 8, 0.0F, 1.0F, 50}, binary32(0x3c343958U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3d4cccccU), 3.0F, 25, 7, binary32(0xbc343958U), 1.0F, 50}},
    UpdateCase{"retract_zero_with_backlog", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 250.0F, 3.0F, 25, 0, 0.0F, 1.0F, 50}, 0.0F, 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 247.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}},
    UpdateCase{"idle_zero", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0.0F, 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}},
    UpdateCase{"idle_fractional", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3fa00000U), 3.0F, 25, 0, 0.0F, 0.0F, 50}, binary32(0x3d000000U),
        binary32(0x44202800U), 8, world::Spider{20, 1224.0F, -4.0F, 199.0F, 4.0F, binary32(0x3fdc0000U), 3.0F, 25, 1, 0.0F, 0.0F, 50}},
    UpdateCase{"idle_negative_screen", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, binary32(0x3e000000U), binary32(0x447a0800U), 1,
        world::Spider{20, 1224.0F, -4.0F, -376.0F, -3.0F, binary32(0x3ff00000U), 3.0F, 25, 1, 0.0F, 0.0F, 50}},
    UpdateCase{"idle_multicell", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1.0F, 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 15.0F, 3.0F, 25, 15, 0.0F, 0.0F, 50}},
    UpdateCase{"idle_exact_wrap", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2.0F, 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}},
    UpdateCase{"idle_stored25_frame24", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x41c7ffffU), 3.0F, 25, 24, 0.0F, 0.0F, 50}, binary32(0x340637bdU), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 25.0F, 3.0F, 25, 24, 0.0F, 0.0F, 50}},
    UpdateCase{"idle_stored25_next_zero", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 25.0F, 3.0F, 25, 24, 0.0F, 0.0F, 50}, 0.0F, 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}},
    UpdateCase{"idle_camera_prestore_boundary", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0.0F, binary32(0x433dbfffU), 0,
        world::Spider{20, 1224.0F, -4.0F, 921.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}},
    UpdateCase{"reaction_normal_frame0", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 1, 0.0F, 0.0F, 50}},
    UpdateCase{"reaction_normal_frame7", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 7, 0.0F, 0.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 8, 0.0F, 0.0F, 50}},
    UpdateCase{"reaction_normal_frame23", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 23, 0.0F, 0.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 24, 0.0F, 0.0F, 50}},
    UpdateCase{"reaction_normal_frame24", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 24, 0.0F, 0.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}},
    UpdateCase{"reaction_initial_mode_retained", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 24, 0.0F, 1.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 1.0F, 50}},
    UpdateCase{"reaction_huge_one_cell", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 4.0F, 640.0F, 0,
        world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 197.0F, 3.0F, 25, 1, 0.0F, 0.0F, 50}},
    UpdateCase{"reaction_stored3_no_advance", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x403fffffU), 3.0F, 25, 0, 0.0F, 0.0F, 50}, binary32(0x31a18937U), 640.0F,
        0, world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 3.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}},
    UpdateCase{"reaction_stored3_next_zero", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 3.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0.0F, 640.0F, 0,
        world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 1, 0.0F, 0.0F, 50}},
    UpdateCase{"reaction_special_frame0", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 1.0F, 0.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 1, 1.0F, 0.0F, 50}},
    UpdateCase{"reaction_special_removed", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 7, 1.0F, 0.0F, 50}, binary32(0x3d800000U), 640.0F, 0,
        world::Spider{-1, 1224.0F, -4.0F, 200.0F, -4.0F, binary32(0x3e000000U), 3.0F, 25, 8, 1.0F, 0.0F, 50}},
    UpdateCase{"removed_frozen", world::Spider{-1, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 8, 1.0F, 0.0F, 50}, 1.0F, 0.0F, 8,
        world::Spider{-1, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 8, 1.0F, 0.0F, 50}}}};

const std::array<ImpactCase, 11> impacts_cases = {
    {ImpactCase{"ordinary_first", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0, false,
         world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, ImpactExpectation{50, true, world::SpiderPopup{1229.0F, 96.0F, 50}}},
        ImpactCase{"ordinary_second", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, false,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2, ImpactExpectation{0, true, std::nullopt}},
        ImpactCase{"ordinary_third", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2, false,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 3, ImpactExpectation{100, true, world::SpiderPopup{1219.0F, 96.0F, 100}}},
        ImpactCase{"ordinary_after_first_bonus", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 4, false,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 5, ImpactExpectation{0, true, std::nullopt}},
        ImpactCase{"ordinary_before25", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 24, false,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 25, ImpactExpectation{0, true, std::nullopt}},
        ImpactCase{"ordinary_hero_hit", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 25, false,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 26, ImpactExpectation{200, true, world::SpiderPopup{1219.0F, 96.0F, 200}}},
        ImpactCase{"ordinary_afterhero", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 26, false,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 27, ImpactExpectation{0, true, std::nullopt}},
        ImpactCase{"ordinary_during_initial_phase", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, 0, false,
            world::Spider{60, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 0, 0.0F, 1.0F, 50}, 1, ImpactExpectation{50, true, world::SpiderPopup{1229.0F, 96.0F, 50}}},
        ImpactCase{"special_first", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0, true,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 1.0F, 0.0F, 100}, 0, ImpactExpectation{100, false, world::SpiderPopup{1219.0F, 96.0F, 100}}},
        ImpactCase{"special_after_third", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 4, true,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 1.0F, 0.0F, 200}, 4, ImpactExpectation{200, false, world::SpiderPopup{1219.0F, 96.0F, 200}}},
        ImpactCase{"special_afterhero", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 26, true,
            world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 1.0F, 0.0F, 400}, 26, ImpactExpectation{400, false, world::SpiderPopup{1219.0F, 96.0F, 400}}}}};

const std::array<BonusCase, 12> bonuses_cases = {{BonusCase{0, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 0, false},
    BonusCase{1, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 1, false},
    BonusCase{2, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 2, false},
    BonusCase{3, false, world::SpiderBonus{world::SpiderBonusKind::first, 0, 16, std::nullopt}, 4, false},
    BonusCase{3, true, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 3, true},
    BonusCase{4, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 4, false},
    BonusCase{6, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 6, false},
    BonusCase{24, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 24, false},
    BonusCase{25, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 25, false},
    BonusCase{26, false, world::SpiderBonus{world::SpiderBonusKind::hero, 100, 20, world::Notice{true, false, 0.0F, 180.0F, 101}}, 26, true},
    BonusCase{26, true, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 26, true},
    BonusCase{27, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt}, 27, false}}};

const std::array<DrawCase, 9> draws_cases = {{DrawCase{"initial_draw", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, false,
                                                  {{400, 0, 50, 200}}, {{-1024, -1024}}, "gfx/spinne2"},
    DrawCase{
        "initial_collision", world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, true, {{400, 0, 50, 200}}, {{-1024, -1024}}, "gfx/spinne1"},
    DrawCase{"idle_frame0", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, false, {{0, 0, 50, 200}}, {{200, -4}}, "gfx/spinne1"},
    DrawCase{"idle_frame24", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 24, 0.0F, 0.0F, 50}, false, {{1200, 0, 50, 200}}, {{200, -4}}, "gfx/spinne1"},
    DrawCase{"idle_collision24", world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 24, 0.0F, 0.0F, 50}, true, {{1200, 0, 50, 200}}, {{200, -4}}, "gfx/spinne1"},
    DrawCase{"reaction_frame0", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, false, {{0, 0, 50, 200}}, {{200, -4}}, "gfx/spinne2"},
    DrawCase{"reaction_frame24", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 24, 0.0F, 0.0F, 50}, false, {{1200, 0, 50, 200}}, {{200, -4}}, "gfx/spinne2"},
    DrawCase{"special_frame7", world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 7, 1.0F, 0.0F, 50}, false, {{350, 0, 50, 200}}, {{200, -4}}, "gfx/spinne2"},
    DrawCase{"negative_pose", world::Spider{20, 1224.0F, -4.0F, binary32(0xc0700000U), binary32(0xc0500000U), 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, false, {{0, 0, 50, 200}},
        {{-3, -3}}, "gfx/spinne1"}}};

const std::array<SequenceCase, 3> sequences_cases = {
    {SequenceCase{"ordinary_three_and_reset", {SequenceStep{"update", 1, 5.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                   world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 247.0F, 3.0F, 25, 7, 0.0F, 1.0F, 50}, 0, false, 0},
                                                  SequenceStep{"update", 8, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 223.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0, false, 0},
                                                  SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, false, 50},
                                                  SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, false, 50},
                                                  SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, false, 50},
                                                  SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2, false, 50},
                                                  SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2, false, 50},
                                                  SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2, false, 50},
                                                  SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 3, false, 150},
                                                  SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::first, 0, 16, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 4, false, 150},
                                                  SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 4, false, 150},
                                                  SequenceStep{"reset", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, 0, false, 0},
                                                  SequenceStep{"update", 1, 5.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 247.0F, 3.0F, 25, 7, 0.0F, 1.0F, 50}, 0, false, 0},
                                                  SequenceStep{"update", 8, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 223.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0, false, 0},
                                                  SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, false, 50}}},
        SequenceCase{"ordinary_twentyfive_hero_latch",
            {SequenceStep{"update", 1, 5.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                 world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 247.0F, 3.0F, 25, 7, 0.0F, 1.0F, 50}, 0, false, 0},
                SequenceStep{"update", 8, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 223.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0, false, 0},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, false, 50},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, false, 50},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 1, false, 50},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2, false, 50},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2, false, 50},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 2, false, 50},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 3, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::first, 0, 16, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 4, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 4, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 5, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 5, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 5, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 6, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 6, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 6, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 7, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 7, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 7, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 8, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 8, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 8, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 9, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 9, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 9, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 10, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 10, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 10, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 11, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 11, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 11, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 12, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 12, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 12, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 13, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 13, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 13, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 14, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 14, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 14, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 15, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 15, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 15, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 16, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 16, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 16, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 17, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 17, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 17, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 18, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 18, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 18, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 19, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 19, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 19, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 20, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 20, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 20, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 21, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 21, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 21, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 22, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 22, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 22, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 23, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 23, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 23, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 24, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 24, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 24, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 25, false, 150},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 25, false, 150},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 100}, 25, false, 150},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 26, false, 350},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::hero, 100, 20, world::Notice{true, false, 0.0F, 180.0F, 101}},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 26, true, 450},
                SequenceStep{"update", 25, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 26, true, 450},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 26, true, 450},
                SequenceStep{"hit", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 27, true, 450},
                SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 0.0F, 0.0F, 200}, 27, true, 450},
                SequenceStep{"reset", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                    world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, 0, false, 0}}},
        SequenceCase{"special_removal_reset", {SequenceStep{"update", 1, 5.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                   world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 247.0F, 3.0F, 25, 7, 0.0F, 1.0F, 50}, 0, false, 0},
                                                  SequenceStep{"update", 8, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{20, 1224.0F, -4.0F, 200.0F, -4.0F, 223.0F, 3.0F, 25, 0, 0.0F, 0.0F, 50}, 0, false, 0},
                                                  SequenceStep{"hit", 0, 0.0F, true, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 1.0F, 0.0F, 100}, 0, false, 100},
                                                  SequenceStep{"bonus", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{60, 1224.0F, -4.0F, 200.0F, -4.0F, 0.0F, 3.0F, 25, 0, 1.0F, 0.0F, 100}, 0, false, 100},
                                                  SequenceStep{"update", 8, 0.0625F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{-1, 1224.0F, -4.0F, 200.0F, -4.0F, 1.0F, 3.0F, 25, 8, 1.0F, 0.0F, 100}, 0, false, 100},
                                                  SequenceStep{"update", 1, 1.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{-1, 1224.0F, -4.0F, 200.0F, -4.0F, 1.0F, 3.0F, 25, 8, 1.0F, 0.0F, 100}, 0, false, 100},
                                                  SequenceStep{"reset", 0, 0.0F, false, world::SpiderBonus{world::SpiderBonusKind::none, 0, 0, std::nullopt},
                                                      world::Spider{20, 1224.0F, -4.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 25, 8, 5.0F, 1.0F, 50}, 0, false, 0}}}}};

void check_bonus(const world::SpiderBonus& actual, const world::SpiderBonus& expected) {
    require(actual == expected, "Original spider bonus kind, score, allocation count or notice request differs");
}

void original_updates() {
    for (const auto& expected : updates_cases) {
        auto spider = expected.before;
        world::update_spider(spider, expected.delta, expected.camera, expected.recoil);
        require(spider == expected.after, expected.id + ": original spider update fields differ");
    }
}

void original_impacts_bonuses_and_draws() {
    for (const auto& expected : impacts_cases) {
        auto spider = expected.before;
        int hits = expected.before_hits;
        const auto impact = world::hit_spider(spider, hits, expected.special_ammunition);
        const auto& effect = expected.effect;
        require(spider == expected.after && hits == expected.after_hits && impact.points == effect.points && impact.swings_sound == effect.swings_sound
                    && impact.popup.has_value() == effect.popup.has_value(),
            expected.id + ": original impact award, counter, sound, popup or reaction differs");
        if (impact.popup) {
            const auto& popup = effect.popup;
            require(*impact.popup == *popup, "Original spider score popup differs");
        }
    }
    for (const auto& expected : bonuses_cases) {
        int hits = expected.before_hits;
        bool latch = expected.before_latch;
        check_bonus(world::apply_spider_bonus(hits, latch), expected.effect);
        require(hits == expected.after_hits && latch == expected.after_latch, "Original spider bonus guard, counter skip or latch differs");
    }
    for (const auto& expected : draws_cases) {
        const auto& spider = expected.state;
        const auto draw = expected.collision ? world::spider_collision(spider) : world::spider_draw(spider);
        const auto& rectangle = expected.source;
        const auto& point = expected.destination;
        require(draw.image == expected.image && draw.source.x == rectangle[0] && draw.source.y == rectangle[1] && draw.source.width == rectangle[2]
                    && draw.source.height == rectangle[3] && draw.destination.x == point[0] && draw.destination.y == point[1] && draw.color_key && !draw.mirror_x,
            expected.id + ": original drawing/collision sheet, cell or pose differs");
    }
}

void original_sequences() {
    for (const auto& expected : sequences_cases) {
        world::Spider spider;
        int hits = 0;
        bool latch = false;
        int score = 0;
        for (const auto& step : expected.steps) {
            const auto action = step.action;
            if (action == "update") {
                for (int update = 0; update < step.count; ++update) {
                    world::update_spider(spider, step.delta, 640, 0);
                }
            } else if (action == "hit") {
                score += world::hit_spider(spider, hits, step.special).points;
            } else if (action == "bonus") {
                const auto effect = world::apply_spider_bonus(hits, latch);
                score += effect.points;
                check_bonus(effect, step.effect);
            } else if (action == "reset") {
                spider = world::Spider{};
                hits = 0;
                latch = false;
                score = 0;
            } else {
                throw std::runtime_error("Unknown source-derived spider sequence action");
            }
            require(spider == step.state && hits == step.hits && latch == step.latch && score == step.score,
                expected.id + ": original repeated return, first/hero bonus, special removal or reset differs");
        }
    }
}

void rejected_inputs() {
    world::Spider spider;
    int hits = 0;
    const auto reject = [&](auto action) {
        const auto before = spider;
        const int old_hits = hits;
        bool rejected = false;
        try {
            action();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && spider == before && hits == old_hits, "Invalid spider state/input must fail before mutating fields or shared counter");
    };
    reject([&] {
        world::update_spider(spider, -1, 640, 0);
    });
    reject([&] {
        world::update_spider(spider, 0, 1281, 0);
    });
    reject([&] {
        world::update_spider(spider, 0, 640, 9);
    });
    hits = -1;
    reject([&] {
        static_cast<void>(world::hit_spider(spider, hits, false));
    });
    hits = 0;
    static_cast<void>(world::hit_spider(spider, hits, false));
    reject([&] {
        static_cast<void>(world::hit_spider(spider, hits, false));
    });
    reject([&] {
        static_cast<void>(world::spider_collision(spider));
    });
    spider = world::Spider{};
    static_cast<void>(world::hit_spider(spider, hits, true));
    for (int frame = 0; frame < 8; ++frame) {
        world::update_spider(spider, 0.0625F, 640, 0);
    }
    require(spider.type == -1 && hits == 1, "Special spider ammunition must remove the same slot without incrementing its ordinary counter");
    reject([&] {
        static_cast<void>(world::spider_draw(spider));
    });
    spider = world::Spider{};
    spider.mode = 0;
    spider.countdown = 0;
    spider.frame = 0;
    reject([&] {
        world::update_spider(spider, std::numeric_limits<float>::max(), 640, 0);
    });
    hits = std::numeric_limits<int>::max();
    reject([&] {
        static_cast<void>(world::hit_spider(spider, hits, false));
    });
    hits = 0;
    spider.points = std::numeric_limits<int>::max();
    reject([&] {
        static_cast<void>(world::hit_spider(spider, hits, true));
    });
    bool latch = false;
    hits = -1;
    reject([&] {
        static_cast<void>(world::apply_spider_bonus(hits, latch));
    });
    require(!latch, "Invalid bonus counter must retain the hero latch");
}

} // namespace

int main() {
    try {
        require(world::Spider{} == expected_initial_state, "Default spider must match original slot5 initializer/reset");
        original_updates();
        original_impacts_bonuses_and_draws();
        original_sequences();
        rejected_inputs();
        std::cout << "Original spider startup/idle, ordinary returns, special removal, score/counters, both balloon bonuses and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
