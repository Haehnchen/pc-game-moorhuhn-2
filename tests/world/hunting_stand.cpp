#include "world/hunting_stand.hpp"

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

struct UpdateCase {
    world::HuntingStand before;
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    world::HuntingStand after;
};

struct ImpactExpectation {
    int points;
    std::vector<std::string> sounds;
    std::optional<world::HuntingStandPopup> popup;
};

struct ImpactCase {
    world::HuntingStand before;
    bool special_ammo;
    world::HuntingStand after;
    ImpactExpectation effects;
};

struct DrawCase {
    world::HuntingStand state;
    std::uint32_t screen_x_bits;
    std::uint32_t screen_y_bits;
    std::string main_image;
    int main_source_y;
    std::array<int, 2> main_destination;
    std::array<int, 2> cover_destination;
    std::optional<int> collision_source_y;
};

struct SequenceCase {
    int initial_type;
    std::vector<bool> inputs;
    int final_type;
    int points;
    int ladder_hits;
    int stand_hits;
};

const world::HuntingStand expected_initial = world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0};

const std::array<float, 2> initial_screen_cases = {{-1024.0F, -1024.0F}};

const std::array<UpdateCase, 63> updates_cases = {{UpdateCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x3e800000U, 0x00000000U, 0,
                                                       world::HuntingStand{13, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 2, 0, 0, 0}},
    UpdateCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x3e800000U, 0x3f36db6eU, 1,
        world::HuntingStand{13, 252.0F, 158.0F, 251.0F, binary32(0x431ecccdU), 0.0F, 2, 0, 0, 0}},
    UpdateCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x3e800000U, 0x43a00000U, 6,
        world::HuntingStand{13, 252.0F, 158.0F, -195.0F, binary32(0x4322cccdU), 0.0F, 2, 0, 0, 0}},
    UpdateCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x3e800000U, 0x44a00000U, 8,
        world::HuntingStand{13, 252.0F, 158.0F, -1539.0F, binary32(0x43246666U), 0.0F, 2, 0, 0, 0}},
    UpdateCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e800000U, 0x00000000U, 0,
        world::HuntingStand{14, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e800000U, 0x3f36db6eU, 1,
        world::HuntingStand{14, 252.0F, 158.0F, 251.0F, binary32(0x431ecccdU), 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e800000U, 0x43a00000U, 6,
        world::HuntingStand{14, 252.0F, 158.0F, -195.0F, binary32(0x4322cccdU), 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e800000U, 0x44a00000U, 8,
        world::HuntingStand{14, 252.0F, 158.0F, -1539.0F, binary32(0x43246666U), 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e800000U, 0x00000000U, 0,
        world::HuntingStand{15, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e800000U, 0x3f36db6eU, 1,
        world::HuntingStand{15, 252.0F, 158.0F, 251.0F, binary32(0x431ecccdU), 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e800000U, 0x43a00000U, 6,
        world::HuntingStand{15, 252.0F, 158.0F, -195.0F, binary32(0x4322cccdU), 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e800000U, 0x44a00000U, 8,
        world::HuntingStand{15, 252.0F, 158.0F, -1539.0F, binary32(0x43246666U), 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x3df5c28fU, 0x00000000U, 0,
        world::HuntingStand{53, 252.0F, 158.0F, 252.0F, 158.0F, 6.0F, 2, 0, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 6.0F, 2, 0, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{53, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 2, 1, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 1, 0, 0}, 0x3f800000U, 0x00000000U, 0,
        world::HuntingStand{14, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{53, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 2, 0, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 2, 0, 0, 0}, 0x3c23d70aU, 0x00000000U, 0,
        world::HuntingStand{53, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x40b00000U), 2, 0, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 2, 0, 0, 0}, 0x3ca3d70aU, 0x00000000U, 0,
        world::HuntingStand{53, 252.0F, 158.0F, 252.0F, 158.0F, 6.0F, 2, 0, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 6.0F, 2, 0, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{53, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 2, 1, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x3f800000U, 0x44a00000U, 8,
        world::HuntingStand{53, 252.0F, 158.0F, -1539.0F, binary32(0x43246666U), 44.0F, 2, 1, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 44.0F, 2, 1, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{14, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 2, 1, 0, 0}, 0x3d000000U, 0x00000000U, 0,
        world::HuntingStand{14, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 10, 0, 0, 0}, 0x3c23d70aU, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x40b00000U), 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 10, 0, 0, 0}, 0x3ca3d70aU, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, 6.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 6.0F, 10, 0, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 1, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3f800000U, 0x44a00000U, 8,
        world::HuntingStand{54, 252.0F, 158.0F, -1539.0F, binary32(0x43246666U), 44.0F, 10, 1, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 44.0F, 10, 1, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, 38.0F, 10, 2, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 10, 9, 0, 0}, 0x3d000000U, 0x00000000U, 0,
        world::HuntingStand{15, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 10, 0, 0, 0}, 0x3c23d70aU, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x40b00000U), 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 10, 0, 0, 0}, 0x3ca3d70aU, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, 6.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 6.0F, 10, 0, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 1, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3f800000U, 0x44a00000U, 8,
        world::HuntingStand{55, 252.0F, 158.0F, -1539.0F, binary32(0x43246666U), 44.0F, 10, 1, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 44.0F, 10, 1, 0, 0}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, 38.0F, 10, 2, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 5.0F, 10, 9, 0, 0}, 0x3d000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3f100000U), 10, 9, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{53, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3e800000U), 2, 1, 0, 0}},
    UpdateCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3e800000U), 2, 1, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{14, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3e800000U), 10, 1, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3e800000U), 10, 1, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3f000000U), 10, 2, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f000000U), 10, 2, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3f400000U), 10, 3, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 3, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, 1.0F, 10, 4, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 1.0F, 10, 4, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3fa00000U), 10, 5, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3fa00000U), 10, 5, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3fc00000U), 10, 6, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3fc00000U), 10, 6, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3fe00000U), 10, 7, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3fe00000U), 10, 7, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, 2.0F, 10, 8, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 2.0F, 10, 8, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{54, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x40100000U), 10, 9, 0, 0}},
    UpdateCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x40100000U), 10, 9, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{15, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{15, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{15, 252.0F, 158.0F, 252.0F, 158.0F, 0.0F, 10, 0, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3e800000U), 10, 1, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3e800000U), 10, 1, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3f000000U), 10, 2, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f000000U), 10, 2, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3f400000U), 10, 3, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 3, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, 1.0F, 10, 4, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 1.0F, 10, 4, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3fa00000U), 10, 5, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3fa00000U), 10, 5, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3fc00000U), 10, 6, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3fc00000U), 10, 6, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x3fe00000U), 10, 7, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3fe00000U), 10, 7, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, 2.0F, 10, 8, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 2.0F, 10, 8, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x40100000U), 10, 9, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x40100000U), 10, 9, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x40200000U), 10, 9, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x40200000U), 10, 9, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, binary32(0x40300000U), 10, 9, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x40300000U), 10, 9, 0, 0}, 0x3e000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, 3.0F, 10, 9, 0, 0}},
    UpdateCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 44.0F, 10, 9, 2, 3}, 0x00000000U, 0x00000000U, 0,
        world::HuntingStand{55, 252.0F, 158.0F, 252.0F, 158.0F, 38.0F, 10, 9, 2, 3}}}};

const std::array<ImpactCase, 12> impacts_cases = {{ImpactCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 2, 0, 0, 0}, false,
                                                       world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 2, 0, 0, 0},
                                                       ImpactExpectation{25, {"chicken_shot_far"}, world::HuntingStandPopup{292.0F, 208.0F, 25}}},
    ImpactCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 2, 0, 0, 0}, true,
        world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 2, 0, 0, 0},
        ImpactExpectation{25, {"chicken_shot_far"}, world::HuntingStandPopup{292.0F, 208.0F, 25}}},
    ImpactCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 0}, false,
        world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 0}, ImpactExpectation{0, {"wood_shot"}, std::nullopt}},
    ImpactCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 1, 0}, false,
        world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 1, 0}, ImpactExpectation{0, {"wood_shot"}, std::nullopt}},
    ImpactCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 0}, true,
        world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 1, 0}, ImpactExpectation{0, {"wood_shot"}, std::nullopt}},
    ImpactCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 1, 0}, true,
        world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 2, 0},
        ImpactExpectation{25, {"wood_shot", "guardhouse_ladder_shot"}, world::HuntingStandPopup{292.0F, 208.0F, 25}}},
    ImpactCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 0}, false,
        world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 0}, ImpactExpectation{0, {"wood_shot"}, std::nullopt}},
    ImpactCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 1}, false,
        world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 1}, ImpactExpectation{0, {"wood_shot"}, std::nullopt}},
    ImpactCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 2}, false,
        world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 2}, ImpactExpectation{0, {"wood_shot"}, std::nullopt}},
    ImpactCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 0}, true,
        world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 1}, ImpactExpectation{0, {"wood_shot"}, std::nullopt}},
    ImpactCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 1}, true,
        world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 2}, ImpactExpectation{0, {"wood_shot"}, std::nullopt}},
    ImpactCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 2}, true,
        world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, binary32(0x3f400000U), 10, 0, 0, 3},
        ImpactExpectation{25, {"wood_shot", "guardhouse_breaks_down"}, world::HuntingStandPopup{292.0F, 208.0F, 25}}}}};

const std::array<DrawCase, 24> draws_cases = {
    {DrawCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz3", 0, {{252, 158}}, {{2172, 158}}, 0},
        DrawCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz3", 0, {{-1539, 164}}, {{381, 164}}, 0},
        DrawCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 1, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz3", 100, {{252, 158}}, {{2172, 158}}, 0},
        DrawCase{world::HuntingStand{13, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 1, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz3", 100, {{-1539, 164}}, {{381, 164}}, 0},
        DrawCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz1", 0, {{252, 158}}, {{2172, 158}}, 100},
        DrawCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz1", 0, {{-1539, 164}}, {{381, 164}}, 100},
        DrawCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 9, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz1", 900, {{252, 158}}, {{2172, 158}}, 100},
        DrawCase{world::HuntingStand{14, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 9, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz1", 900, {{-1539, 164}}, {{381, 164}}, 100},
        DrawCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz2", 0, {{252, 158}}, {{2172, 158}}, 200},
        DrawCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz2", 0, {{-1539, 164}}, {{381, 164}}, 200},
        DrawCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 9, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz2", 900, {{252, 158}}, {{2172, 158}}, 200},
        DrawCase{world::HuntingStand{15, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 9, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz2", 900, {{-1539, 164}}, {{381, 164}}, 200},
        DrawCase{
            world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz3", 0, {{252, 158}}, {{2172, 158}}, std::nullopt},
        DrawCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 0, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz3", 0, {{-1539, 164}}, {{381, 164}},
            std::nullopt},
        DrawCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 1, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz3", 100, {{252, 158}}, {{2172, 158}},
            std::nullopt},
        DrawCase{world::HuntingStand{53, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 2, 1, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz3", 100, {{-1539, 164}}, {{381, 164}},
            std::nullopt},
        DrawCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz1", 0, {{252, 158}}, {{2172, 158}},
            std::nullopt},
        DrawCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz1", 0, {{-1539, 164}}, {{381, 164}},
            std::nullopt},
        DrawCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 9, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz1", 900, {{252, 158}}, {{2172, 158}},
            std::nullopt},
        DrawCase{world::HuntingStand{54, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 9, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz1", 900, {{-1539, 164}}, {{381, 164}},
            std::nullopt},
        DrawCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz2", 0, {{252, 158}}, {{2172, 158}},
            std::nullopt},
        DrawCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 0, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz2", 0, {{-1539, 164}}, {{381, 164}},
            std::nullopt},
        DrawCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 9, 0, 0}, 0x437c0000U, 0x431e0000U, "gfx/hochsitz2", 900, {{252, 158}}, {{2172, 158}},
            std::nullopt},
        DrawCase{world::HuntingStand{55, 252.0F, 158.0F, -1024.0F, -1024.0F, 0.0F, 10, 9, 0, 0}, 0xc4c06000U, 0x43246666U, "gfx/hochsitz2", 900, {{-1539, 164}}, {{381, 164}},
            std::nullopt}}};

const std::array<SequenceCase, 3> sequences_cases = {{SequenceCase{14, {false, false, false, false, false}, 14, 0, 0, 0}, SequenceCase{14, {true, false, true}, 54, 25, 2, 0},
    SequenceCase{15, {true, false, true, false, true}, 55, 25, 0, 3}}};

void compare(const world::HuntingStand& stand, const world::HuntingStand& expected, bool check_pose = false) {
    require(stand.type == expected.type && stand.frame == expected.frame && stand.frame_count == expected.frame_count && stand.ladder_special_hits == expected.ladder_special_hits
                && stand.stand_special_hits == expected.stand_special_hits
                && std::bit_cast<std::uint32_t>(stand.animation_time) == std::bit_cast<std::uint32_t>(expected.animation_time),
        "Source state or binary32 animation word differs");
    require(stand.world_x == 252 && stand.world_y == 158, "Hunting stand moved from original world position");
    if (check_pose) {
        require(std::bit_cast<std::uint32_t>(stand.screen_x) == std::bit_cast<std::uint32_t>(expected.screen_x)
                    && std::bit_cast<std::uint32_t>(stand.screen_y) == std::bit_cast<std::uint32_t>(expected.screen_y),
            "Source pose binary32 words differ");
    }
}

std::string_view sound_name(world::HuntingStandSound sound) {
    switch (sound) {
        case world::HuntingStandSound::chicken_shot_far:
            return "chicken_shot_far";
        case world::HuntingStandSound::wood_shot:
            return "wood_shot";
        case world::HuntingStandSound::guardhouse_ladder_shot:
            return "guardhouse_ladder_shot";
        case world::HuntingStandSound::guardhouse_breaks_down:
            return "guardhouse_breaks_down";
    }
    throw std::runtime_error("Unknown hunting stand sound");
}

void source_vectors() {
    const auto initial = world::initial_hunting_stand();
    compare(initial, expected_initial);
    require(initial.screen_x == initial_screen_cases.at(0) && initial.screen_y == initial_screen_cases.at(1), "Initial source sentinel differs");
    for (const auto& item : updates_cases) {
        auto stand = item.before;
        world::update_hunting_stand(stand, binary32(item.delta_bits), binary32(item.camera_bits), item.recoil);
        compare(stand, item.after, true);
    }
    for (const auto& item : impacts_cases) {
        auto stand = item.before;
        const auto impact = world::hit_hunting_stand(stand, item.special_ammo);
        compare(stand, item.after);
        const auto& effects = item.effects;
        require(impact.points == effects.points && impact.sound_count == effects.sounds.size(), "Impact score or sound count differs");
        for (std::size_t index = 0; index < impact.sound_count; ++index) {
            require(sound_name(impact.sounds[index]) == effects.sounds.at(index), "Impact sound ordering differs");
        }
        if (!effects.popup.has_value()) {
            require(!impact.popup, "Unexpected score popup");
        } else {
            require(
                impact.popup && impact.popup->world_x == effects.popup->world_x && impact.popup->world_y == effects.popup->world_y && impact.popup->points == effects.popup->points,
                "Source selected popup differs");
        }
    }
    for (const auto& item : draws_cases) {
        auto stand = item.state;
        stand.screen_x = binary32(item.screen_x_bits);
        stand.screen_y = binary32(item.screen_y_bits);
        const auto draws = world::hunting_stand_draw(stand);
        const auto& main = draws[0];
        const auto& cover = draws[1];
        require(main.image == item.main_image && main.source.x == 0 && main.source.y == item.main_source_y && main.source.width == 120 && main.source.height == 100
                    && main.color_key && !main.mirror_x && main.destination.x == item.main_destination.at(0) && main.destination.y == item.main_destination.at(1),
            "Source main draw differs");
        require(cover.image == "gfx/hochsitz4" && cover.source.x == 0 && cover.source.y == 0 && cover.source.width == 120 && cover.source.height == 100 && cover.color_key
                    && !cover.mirror_x && cover.destination.x == item.cover_destination.at(0) && cover.destination.y == item.cover_destination.at(1),
            "Source duplicate cover draw differs");
        if (item.collision_source_y.has_value()) {
            const auto mask = world::hunting_stand_collision(stand);
            require(mask.image == "gfx/hochsitzcol" && mask.source.x == 0 && mask.source.y == *item.collision_source_y && mask.source.width == 120 && mask.source.height == 100
                        && !mask.color_key && !mask.mirror_x && mask.destination.x == main.destination.x && mask.destination.y == main.destination.y,
                "Live fixed collision mask differs");
        }
    }
    for (const auto& item : sequences_cases) {
        auto stand = initial;
        stand.type = item.initial_type;
        stand.frame_count = 10;
        int score = 0;
        for (const auto& special : item.inputs) {
            score += world::hit_hunting_stand(stand, special).points;
        }
        require(stand.type == item.final_type && score == item.points && stand.ladder_special_hits == item.ladder_hits && stand.stand_special_hits == item.stand_hits,
            "Mixed ammunition sequence differs");
    }
}

template <class Action> void rejected(Action action, world::HuntingStand stand = {}) {
    const auto before = stand;
    bool failed = false;
    try {
        action(stand);
    } catch (const std::invalid_argument&) {
        failed = true;
    }
    require(failed && stand == before, "Invalid input was accepted or mutated stand state");
}

void boundary_checks() {
    for (const float invalid : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        rejected([invalid](auto& stand) {
            world::update_hunting_stand(stand, invalid, 0, 0);
        });
        rejected([invalid](auto& stand) {
            world::update_hunting_stand(stand, 0, invalid, 0);
        });
    }
    rejected([](auto& stand) {
        world::update_hunting_stand(stand, 0, 1281, 0);
    });
    rejected([](auto& stand) {
        world::update_hunting_stand(stand, 0, 0, -1);
    });
    rejected([](auto& stand) {
        world::update_hunting_stand(stand, 0, 0, 9);
    });
    for (const int type : {-1, 12, 16, 52, 56, 500}) {
        world::HuntingStand stand;
        stand.type = type;
        rejected(
            [](auto& value) {
                world::update_hunting_stand(value, 0, 0, 0);
            },
            stand);
    }
    for (const int type : {53, 54, 55}) {
        world::HuntingStand stand;
        stand.type = type;
        stand.frame_count = type == 53 ? 2 : 10;
        rejected(
            [](auto& value) {
                static_cast<void>(world::hit_hunting_stand(value, true));
            },
            stand);
        rejected(
            [](auto& value) {
                static_cast<void>(world::hunting_stand_collision(value));
            },
            stand);
    }
    world::HuntingStand overflow;
    overflow.type = 53;
    overflow.animation_time = std::numeric_limits<float>::max();
    rejected(
        [](auto& value) {
            world::update_hunting_stand(value, std::numeric_limits<float>::max(), 0, 0);
        },
        overflow);
    world::HuntingStand unsupported;
    unsupported.type = 14;
    unsupported.frame_count = 10;
    unsupported.ladder_special_hits = 2;
    rejected(
        [](auto& value) {
            static_cast<void>(world::hit_hunting_stand(value, true));
        },
        unsupported);
    unsupported.screen_x = std::numeric_limits<float>::max();
    rejected(
        [](auto& value) {
            static_cast<void>(world::hunting_stand_draw(value));
        },
        unsupported);
}

void lifecycle() {
    auto stand = world::initial_hunting_stand();
    int score = world::hit_hunting_stand(stand, false).points;
    world::update_hunting_stand(stand, 1, 0, 0);
    require(stand.type == 53 && stand.frame == 1, "Long sample advanced more than one frame");
    world::update_hunting_stand(stand, 0, 0, 0);
    require(stand.type == 14 && stand.animation_time == 0, "First transition failed to discard remainder");
    score += world::hit_hunting_stand(stand, true).points;
    score += world::hit_hunting_stand(stand, true).points;
    for (int index = 0; index < 10; ++index) {
        world::update_hunting_stand(stand, 0.125F, 0, 0);
    }
    require(stand.type == 15 && stand.frame == 0 && stand.animation_time == 0 && stand.ladder_special_hits == 2, "Ladder reaction transition differs");
    for (int index = 0; index < 3; ++index) {
        score += world::hit_hunting_stand(stand, true).points;
    }
    for (int index = 0; index < 20; ++index) {
        world::update_hunting_stand(stand, 0.125F, 1280, 8);
    }
    require(score == 75 && stand.type == 55 && stand.frame == 9 && stand.ladder_special_hits == 2 && stand.stand_special_hits == 3,
        "Full source lifecycle, score or permanent occupied55 differs");
    const auto draws = world::hunting_stand_draw(stand);
    require(draws[0].source.y == 900 && draws[1].source.y == 0, "Final main or persistent cover disappeared");
}

} // namespace

int main() {
    try {
        source_vectors();
        boundary_checks();
        lifecycle();
        std::cout << "Hunting stand source initialization, " << updates_cases.size() << " updates, " << impacts_cases.size() << " impacts, " << draws_cases.size()
                  << " draw pairs, mixed ammunition, binary32 boundaries, retained55 and reset passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
