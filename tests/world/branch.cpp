#include "world/branch.hpp"

#include "support/numeric.hpp"
#include "support/reject_unchanged.hpp"
#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct UpdateCase {
    std::string id;
    world::Branch before;
    float delta;
    float camera;
    int recoil;
    world::Branch after;
    bool ground_sound;
};

struct ImpactCase {
    std::string id;
    world::Branch before;
    std::array<std::uint32_t, 2> random;
    world::Branch after;
};

struct DrawCase {
    world::Branch state;
    std::string image;
    std::array<int, 4> source;
    std::array<int, 2> destination;
    bool color_key;
    bool mirror_x;
};

struct ResetCase {
    float delta;
    float camera;
    int recoil;
    std::array<world::Branch, 4> native_reset;
    std::array<world::Branch, 4> raw_original_reset;
    std::array<world::Branch, 4> first_normal_update;
};

const std::array<world::Branch, 4> initial_branches_cases = {
    {world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
        world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}};

const std::array<UpdateCase, 87> updates_cases = {{UpdateCase{"live_slot20_camera00000000", world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                                                       100.0F, 0.0F, 0, world::Branch{22, 87.0F, 5.0F, 87.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot20_camera44200000", world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 640.0F, 8,
        world::Branch{22, 87.0F, 5.0F, -937.0F, 13.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot20_camera44a00000", world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 1280.0F, 3,
        world::Branch{22, 87.0F, 5.0F, -1961.0F, 8.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot20_camera441c0aaa", world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 624.1666259765625F, 1,
        world::Branch{22, 87.0F, 5.0F, -911.0F, 6.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot20_camera442027ff", world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 640.6249389648438F, 8,
        world::Branch{22, 87.0F, 5.0F, -937.0F, 13.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot21_camera00000000", world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 0.0F, 0,
        world::Branch{22, 2007.0F, 5.0F, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot21_camera44200000", world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 640.0F, 8,
        world::Branch{22, 2007.0F, 5.0F, 983.0F, 13.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot21_camera44a00000", world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 1280.0F, 3,
        world::Branch{22, 2007.0F, 5.0F, -41.0F, 8.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot21_camera441c0aaa", world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 624.1666259765625F, 1,
        world::Branch{22, 2007.0F, 5.0F, 1009.0F, 6.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot21_camera442027ff", world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 640.6249389648438F, 8,
        world::Branch{22, 2007.0F, 5.0F, 983.0F, 13.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot22_camera00000000", world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 0.0F, 0,
        world::Branch{23, 215.0F, 2.0F, 215.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot22_camera44200000", world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 640.0F, 8,
        world::Branch{23, 215.0F, 2.0F, -809.0F, 10.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot22_camera44a00000", world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 1280.0F, 3,
        world::Branch{23, 215.0F, 2.0F, -1833.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot22_camera441c0aaa", world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 624.1666259765625F, 1,
        world::Branch{23, 215.0F, 2.0F, -783.0F, 3.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot22_camera442027ff", world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 640.6249389648438F, 8,
        world::Branch{23, 215.0F, 2.0F, -809.0F, 10.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot23_camera00000000", world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 0.0F, 0,
        world::Branch{23, 2135.0F, 2.0F, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot23_camera44200000", world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 640.0F, 8,
        world::Branch{23, 2135.0F, 2.0F, 1111.0F, 10.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot23_camera44a00000", world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 1280.0F, 3,
        world::Branch{23, 2135.0F, 2.0F, 87.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot23_camera441c0aaa", world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 624.1666259765625F, 1,
        world::Branch{23, 2135.0F, 2.0F, 1137.0F, 3.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"live_slot23_camera442027ff", world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 100.0F, 640.6249389648438F, 8,
        world::Branch{23, 2135.0F, 2.0F, 1111.0F, 10.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"fall_slot20_delta00000000", world::Branch{62, 87.0F, 5.0F, -937.0F, 5.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}, 0.0F, 640.6249389648438F, 8,
        world::Branch{62, 87.0F, 5.0F, -937.0F, 13.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"fall_slot20_delta3a83126f", world::Branch{62, 87.0F, 5.0F, -937.0F, 5.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}, 0.0010000000474974513F,
        640.6249389648438F, 8,
        world::Branch{
            62, binary32(0x42ade666U), binary32(0x40a000d2U), binary32(0xc46a4333U), binary32(0x41500069U), binary32(0xbf0000a8U), binary32(0x3a83126fU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot20_delta3e000000", world::Branch{62, 87.0F, 5.0F, -937.0F, 5.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}, 0.125F, 640.6249389648438F, 8,
        world::Branch{
            62, binary32(0x42a17800U), binary32(0x40d20000U), binary32(0xc46bd100U), binary32(0x41690000U), binary32(0xbf0051ecU), binary32(0x3e000000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot20_delta3f900000", world::Branch{62, 87.0F, 5.0F, -937.0F, 5.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}, 1.125F, 640.6249389648438F, 8,
        world::Branch{
            62, binary32(0x41ebdfffU), binary32(0x43039000U), binary32(0xc478a100U), binary32(0x430b9000U), binary32(0xbf02e148U), binary32(0x3f900000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot20_delta41200000", world::Branch{62, 87.0F, 5.0F, -937.0F, 5.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}, 10.0F, 640.6249389648438F, 8,
        world::Branch{-1, -513.0F, 10005.0F, -1537.0F, 10013.0F, binary32(0xbf19999aU), 10.0F, 0.0F, 3.0F, 1, 0, 0}, true},
    UpdateCase{"fall_slot21_delta00000000", world::Branch{62, 2007.0F, 5.0F, 983.0F, 5.0F, binary32(0x3ecccccdU), binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 0}, 0.0F,
        640.6249389648438F, 8, world::Branch{62, 2007.0F, 5.0F, 983.0F, 13.0F, binary32(0x3ecccccdU), binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"fall_slot21_delta3a83126f", world::Branch{62, 2007.0F, 5.0F, 983.0F, 5.0F, binary32(0x3ecccccdU), binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 0},
        0.0010000000474974513F, 640.6249389648438F, 8,
        world::Branch{
            62, binary32(0x44fae148U), binary32(0x40a6154dU), binary32(0x4475c290U), binary32(0x41530aa6U), binary32(0x3eccce1dU), binary32(0x3ff353f8U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot21_delta3e000000", world::Branch{62, 2007.0F, 5.0F, 983.0F, 5.0F, binary32(0x3ecccccdU), binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 0}, 0.125F,
        640.6249389648438F, 8,
        world::Branch{
            62, binary32(0x44fb8080U), binary32(0x41f28001U), binary32(0x44770100U), binary32(0x42194000U), binary32(0x3ecd70a4U), binary32(0x4001999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot21_delta3f900000", world::Branch{62, 2007.0F, 5.0F, 983.0F, 5.0F, binary32(0x3ecccccdU), binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 0}, 1.125F,
        640.6249389648438F, 8,
        world::Branch{
            62, binary32(0x45005440U), binary32(0x43aca800U), binary32(0x4480a880U), binary32(0x43b0a800U), binary32(0x3ed28f5cU), binary32(0x4041999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot21_delta41200000", world::Branch{62, 2007.0F, 5.0F, 983.0F, 5.0F, binary32(0x3ecccccdU), binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 0}, 10.0F,
        640.6249389648438F, 8, world::Branch{-1, 2507.0F, 11905.0F, 1483.0F, 11913.0F, binary32(0x3f000000U), binary32(0x413e6666U), 0.0F, 3.0F, 1, 0, 0}, true},
    UpdateCase{"fall_slot22_delta00000000", world::Branch{63, 215.0F, 2.0F, -809.0F, 2.0F, 0.0F, 1.0F, 0.0F, 3.0F, 1, 0, 0}, 0.0F, 640.6249389648438F, 8,
        world::Branch{63, 215.0F, 2.0F, -809.0F, 10.0F, 0.0F, 1.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"fall_slot22_delta3a83126f", world::Branch{63, 215.0F, 2.0F, -809.0F, 2.0F, 0.0F, 1.0F, 0.0F, 3.0F, 1, 0, 0}, 0.0010000000474974513F, 640.6249389648438F, 8,
        world::Branch{63, 215.0F, binary32(0x4006680aU), -809.0F, binary32(0x41219a02U), binary32(0xb727c5adU), binary32(0x3f8020c5U), 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"fall_slot22_delta3e000000", world::Branch{63, 215.0F, 2.0F, -809.0F, 2.0F, 0.0F, 1.0F, 0.0F, 3.0F, 1, 0, 0}, 0.125F, 640.6249389648438F, 8,
        world::Branch{
            63, binary32(0x4356fc00U), binary32(0x41808000U), binary32(0xc44a4100U), binary32(0x41c08000U), binary32(0xbaa3d70aU), binary32(0x3f900000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot22_delta3f900000", world::Branch{63, 215.0F, 2.0F, -809.0F, 2.0F, 0.0F, 1.0F, 0.0F, 3.0F, 1, 0, 0}, 1.125F, 640.6249389648438F, 8,
        world::Branch{
            63, binary32(0x4355bc00U), binary32(0x43711000U), binary32(0xc44a9100U), binary32(0x43791000U), binary32(0xbc3851ebU), binary32(0x40080000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot22_delta41200000", world::Branch{63, 215.0F, 2.0F, -809.0F, 2.0F, 0.0F, 1.0F, 0.0F, 3.0F, 1, 0, 0}, 10.0F, 640.6249389648438F, 8,
        world::Branch{-1, binary32(0x42e60001U), 11002.0F, -909.0F, 11010.0F, binary32(0xbdccccccU), 11.0F, 0.0F, 3.0F, 1, 0, 0}, true},
    UpdateCase{"fall_slot23_delta00000000", world::Branch{63, 2135.0F, 2.0F, 1111.0F, 2.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 3.0F, 1, 0, 0}, 0.0F,
        640.6249389648438F, 8, world::Branch{63, 2135.0F, 2.0F, 1111.0F, 10.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"fall_slot23_delta3a83126f", world::Branch{63, 2135.0F, 2.0F, 1111.0F, 2.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 3.0F, 1, 0, 0},
        0.0010000000474974513F, 640.6249389648438F, 8,
        world::Branch{
            63, binary32(0x45057052U), binary32(0x40047c85U), binary32(0x448ae0a4U), binary32(0x41211f21U), binary32(0x3e4ccf6cU), binary32(0x3f3374bcU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot23_delta3e000000", world::Branch{63, 2135.0F, 2.0F, 1111.0F, 2.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 3.0F, 1, 0, 0}, 0.125F,
        640.6249389648438F, 8,
        world::Branch{
            63, binary32(0x45059840U), binary32(0x41450000U), binary32(0x448b3080U), binary32(0x41a28000U), binary32(0x3e4e147bU), binary32(0x3f533333U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot23_delta3f900000", world::Branch{63, 2135.0F, 2.0F, 1111.0F, 2.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 3.0F, 1, 0, 0}, 1.125F,
        640.6249389648438F, 8,
        world::Branch{
            63, binary32(0x4506ec40U), binary32(0x434f5000U), binary32(0x448dd880U), binary32(0x43575000U), binary32(0x3e5851ecU), binary32(0x3fe9999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"fall_slot23_delta41200000", world::Branch{63, 2135.0F, 2.0F, 1111.0F, 2.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 3.0F, 1, 0, 0}, 10.0F,
        640.6249389648438F, 8, world::Branch{-1, 2435.0F, 10702.0F, 1411.0F, 10710.0F, binary32(0x3e99999aU), binary32(0x412b3333U), 0.0F, 3.0F, 1, 0, 0}, true},
    UpdateCase{"ground_y43f00000_delta00000000", world::Branch{62, 87.0F, 480.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 0.0F, 0.0F, 8,
        world::Branch{62, 87.0F, 480.0F, 87.0F, 488.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"ground_y43f00000_delta3727c5ac", world::Branch{62, 87.0F, 480.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 9.999999747378752e-06F, 0.0F, 8,
        world::Branch{62, 87.0F, 480.0F, 87.0F, 488.0F, binary32(0xb3d6bf94U), binary32(0x3727c5acU), 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"ground_y43f00000_delta3a83126f", world::Branch{62, 87.0F, 480.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 0.0010000000474974513F, 0.0F, 8,
        world::Branch{-1, 87.0F, binary32(0x43f00003U), 87.0F, binary32(0x43f40003U), binary32(0xb727c5adU), binary32(0x3a83126fU), 0.0F, 3.0F, 1, 0, 0}, true},
    UpdateCase{"ground_y43efffff_delta00000000", world::Branch{62, 87.0F, binary32(0x43efffffU), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 0.0F, 0.0F, 8,
        world::Branch{62, 87.0F, binary32(0x43efffffU), 87.0F, binary32(0x43f3ffffU), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"ground_y43f00001_delta00000000", world::Branch{62, 87.0F, binary32(0x43f00001U), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 0.0F, 0.0F, 8,
        world::Branch{-1, 87.0F, binary32(0x43f00001U), 87.0F, binary32(0x43f40001U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, true},
    UpdateCase{"removed_unchanged_no_second_sound", world::Branch{-1, 87.0F, 600.0F, -1024.0F, 608.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, 20.0F, 1280.0F, 8,
        world::Branch{-1, 87.0F, 600.0F, -1024.0F, 608.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"sequence_slot20_step1", world::Branch{62, 87.0F, 5.0F, -1024.0F, -1024.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}, 0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x42a17800U), binary32(0x40d20000U), binary32(0x42a17800U), binary32(0x40d20000U), binary32(0xbf0051ecU), binary32(0x3e000000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step2",
        world::Branch{
            62, binary32(0x42a17800U), binary32(0x40d20000U), binary32(0x42a17800U), binary32(0x40d20000U), binary32(0xbf0051ecU), binary32(0x3e000000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x4294e800U), binary32(0x411b0000U), binary32(0x4294e800U), binary32(0x411b0000U), binary32(0xbf00a3d8U), binary32(0x3e800000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step3",
        world::Branch{
            62, binary32(0x4294e800U), binary32(0x411b0000U), binary32(0x4294e800U), binary32(0x411b0000U), binary32(0xbf00a3d8U), binary32(0x3e800000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x42885000U), binary32(0x41660000U), binary32(0x42885000U), binary32(0x41660000U), binary32(0xbf00f5c4U), binary32(0x3ec00000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step4",
        world::Branch{
            62, binary32(0x42885000U), binary32(0x41660000U), binary32(0x42885000U), binary32(0x41660000U), binary32(0xbf00f5c4U), binary32(0x3ec00000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x42776000U), binary32(0x41a50000U), binary32(0x42776000U), binary32(0x41a50000U), binary32(0xbf0147b0U), binary32(0x3f000000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step5",
        world::Branch{
            62, binary32(0x42776000U), binary32(0x41a50000U), binary32(0x42776000U), binary32(0x41a50000U), binary32(0xbf0147b0U), binary32(0x3f000000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x425e1000U), binary32(0x41e38000U), binary32(0x425e1000U), binary32(0x41e38000U), binary32(0xbf01999cU), binary32(0x3f200000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step6",
        world::Branch{
            62, binary32(0x425e1000U), binary32(0x41e38000U), binary32(0x425e1000U), binary32(0x41e38000U), binary32(0xbf01999cU), binary32(0x3f200000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x4244afffU), binary32(0x42174000U), binary32(0x4244afffU), binary32(0x42174000U), binary32(0xbf01eb88U), binary32(0x3f400000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step7",
        world::Branch{
            62, binary32(0x4244afffU), binary32(0x42174000U), binary32(0x4244afffU), binary32(0x42174000U), binary32(0xbf01eb88U), binary32(0x3f400000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x422b3ffeU), binary32(0x42430000U), binary32(0x422b3ffeU), binary32(0x42430000U), binary32(0xbf023d74U), binary32(0x3f600000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step8",
        world::Branch{
            62, binary32(0x422b3ffeU), binary32(0x42430000U), binary32(0x422b3ffeU), binary32(0x42430000U), binary32(0xbf023d74U), binary32(0x3f600000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{62, binary32(0x4211bffdU), binary32(0x42750000U), binary32(0x4211bffdU), binary32(0x42750000U), binary32(0xbf028f60U), 1.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"sequence_slot20_step9",
        world::Branch{62, binary32(0x4211bffdU), binary32(0x42750000U), binary32(0x4211bffdU), binary32(0x42750000U), binary32(0xbf028f60U), 1.0F, 0.0F, 3.0F, 1, 0, 0}, 0.125F,
        0.0F, 0,
        world::Branch{
            62, binary32(0x41f05ff8U), binary32(0x4296a000U), binary32(0x41f05ff8U), binary32(0x4296a000U), binary32(0xbf02e14cU), binary32(0x3f900000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step10",
        world::Branch{
            62, binary32(0x41f05ff8U), binary32(0x4296a000U), binary32(0x41f05ff8U), binary32(0x4296a000U), binary32(0xbf02e14cU), binary32(0x3f900000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x41bd1ff6U), binary32(0x42b5e000U), binary32(0x41bd1ff6U), binary32(0x42b5e000U), binary32(0xbf033338U), binary32(0x3fa00000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step11",
        world::Branch{
            62, binary32(0x41bd1ff6U), binary32(0x42b5e000U), binary32(0x41bd1ff6U), binary32(0x42b5e000U), binary32(0xbf033338U), binary32(0x3fa00000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x4189bff4U), binary32(0x42d84000U), binary32(0x4189bff4U), binary32(0x42d84000U), binary32(0xbf038524U), binary32(0x3fb00000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step12",
        world::Branch{
            62, binary32(0x4189bff4U), binary32(0x42d84000U), binary32(0x4189bff4U), binary32(0x42d84000U), binary32(0xbf038524U), binary32(0x3fb00000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x412c7fe4U), binary32(0x42fdc000U), binary32(0x412c7fe4U), binary32(0x42fdc000U), binary32(0xbf03d710U), binary32(0x3fc00000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step13",
        world::Branch{
            62, binary32(0x412c7fe4U), binary32(0x42fdc000U), binary32(0x412c7fe4U), binary32(0x42fdc000U), binary32(0xbf03d710U), binary32(0x3fc00000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0x408a7fbeU), binary32(0x43133000U), binary32(0x408a7fbeU), binary32(0x43133000U), binary32(0xbf0428fcU), binary32(0x3fd00000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step14",
        world::Branch{
            62, binary32(0x408a7fbeU), binary32(0x43133000U), binary32(0x408a7fbeU), binary32(0x43133000U), binary32(0xbf0428fcU), binary32(0x3fd00000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0xc0090099U), binary32(0x43291000U), binary32(0xc0090099U), binary32(0x43291000U), binary32(0xbf047ae8U), binary32(0x3fe00000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step15",
        world::Branch{
            62, binary32(0xc0090099U), binary32(0x43291000U), binary32(0xc0090099U), binary32(0x43291000U), binary32(0xbf047ae8U), binary32(0x3fe00000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0xc10a002cU), binary32(0x43408000U), binary32(0xc10a002cU), binary32(0x43408000U), binary32(0xbf04ccd4U), binary32(0x3ff00000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step16",
        world::Branch{
            62, binary32(0xc10a002cU), binary32(0x43408000U), binary32(0xc10a002cU), binary32(0x43408000U), binary32(0xbf04ccd4U), binary32(0x3ff00000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{62, binary32(0xc1720032U), binary32(0x43598000U), binary32(0xc1720032U), binary32(0x43598000U), binary32(0xbf051ec0U), 2.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"sequence_slot20_step17",
        world::Branch{62, binary32(0xc1720032U), binary32(0x43598000U), binary32(0xc1720032U), binary32(0x43598000U), binary32(0xbf051ec0U), 2.0F, 0.0F, 3.0F, 1, 0, 0}, 0.125F,
        0.0F, 0,
        world::Branch{
            62, binary32(0xc1ad201cU), binary32(0x43741000U), binary32(0xc1ad201cU), binary32(0x43741000U), binary32(0xbf0570acU), binary32(0x40080000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step18",
        world::Branch{
            62, binary32(0xc1ad201cU), binary32(0x43741000U), binary32(0xc1ad201cU), binary32(0x43741000U), binary32(0xbf0570acU), binary32(0x40080000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0xc1e1601fU), binary32(0x43881800U), binary32(0xc1e1601fU), binary32(0x43881800U), binary32(0xbf05c298U), binary32(0x40100000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step19",
        world::Branch{
            62, binary32(0xc1e1601fU), binary32(0x43881800U), binary32(0xc1e1601fU), binary32(0x43881800U), binary32(0xbf05c298U), binary32(0x40100000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0xc20ae011U), binary32(0x4396f000U), binary32(0xc20ae011U), binary32(0x4396f000U), binary32(0xbf061484U), binary32(0x40180000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step20",
        world::Branch{
            62, binary32(0xc20ae011U), binary32(0x4396f000U), binary32(0xc20ae011U), binary32(0x4396f000U), binary32(0xbf061484U), binary32(0x40180000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0xc2252013U), binary32(0x43a69000U), binary32(0xc2252013U), binary32(0x43a69000U), binary32(0xbf066670U), binary32(0x40200000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step21",
        world::Branch{
            62, binary32(0xc2252013U), binary32(0x43a69000U), binary32(0xc2252013U), binary32(0x43a69000U), binary32(0xbf066670U), binary32(0x40200000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0xc23f7015U), binary32(0x43b6f800U), binary32(0xc23f7015U), binary32(0x43b6f800U), binary32(0xbf06b85cU), binary32(0x40280000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step22",
        world::Branch{
            62, binary32(0xc23f7015U), binary32(0x43b6f800U), binary32(0xc23f7015U), binary32(0x43b6f800U), binary32(0xbf06b85cU), binary32(0x40280000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0xc259d017U), binary32(0x43c82800U), binary32(0xc259d017U), binary32(0x43c82800U), binary32(0xbf070a48U), binary32(0x40300000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step23",
        world::Branch{
            62, binary32(0xc259d017U), binary32(0x43c82800U), binary32(0xc259d017U), binary32(0x43c82800U), binary32(0xbf070a48U), binary32(0x40300000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            62, binary32(0xc2744019U), binary32(0x43da2000U), binary32(0xc2744019U), binary32(0x43da2000U), binary32(0xbf075c34U), binary32(0x40380000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot20_step24",
        world::Branch{
            62, binary32(0xc2744019U), binary32(0x43da2000U), binary32(0xc2744019U), binary32(0x43da2000U), binary32(0xbf075c34U), binary32(0x40380000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{62, binary32(0xc287600eU), binary32(0x43ece000U), binary32(0xc287600eU), binary32(0x43ece000U), binary32(0xbf07ae20U), 3.0F, 0.0F, 3.0F, 1, 0, 0}, false},
    UpdateCase{"sequence_slot20_step25",
        world::Branch{62, binary32(0xc287600eU), binary32(0x43ece000U), binary32(0xc287600eU), binary32(0x43ece000U), binary32(0xbf07ae20U), 3.0F, 0.0F, 3.0F, 1, 0, 0}, 0.125F,
        0.0F, 0,
        world::Branch{
            -1, binary32(0xc294a80fU), binary32(0x44003400U), binary32(0xc294a80fU), binary32(0x44003400U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F, 3.0F, 1, 0, 0},
        true},
    UpdateCase{"sequence_slot20_after_removal",
        world::Branch{
            -1, binary32(0xc294a80fU), binary32(0x44003400U), binary32(0xc294a80fU), binary32(0x44003400U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 8,
        world::Branch{
            -1, binary32(0xc294a80fU), binary32(0x44003400U), binary32(0xc294a80fU), binary32(0x44003400U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step1", world::Branch{63, 215.0F, 2.0F, -1024.0F, -1024.0F, binary32(0x3ecccccdU), binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 0}, 0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x435c0400U), binary32(0x41da8001U), binary32(0x435c0400U), binary32(0x41da8001U), binary32(0x3ecd70a4U), binary32(0x4001999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step2",
        world::Branch{
            63, binary32(0x435c0400U), binary32(0x41da8001U), binary32(0x435c0400U), binary32(0x41da8001U), binary32(0x3ecd70a4U), binary32(0x4001999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x43610c00U), binary32(0x4258c001U), binary32(0x43610c00U), binary32(0x4258c001U), binary32(0x3ece147bU), binary32(0x4009999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step3",
        world::Branch{
            63, binary32(0x43610c00U), binary32(0x4258c001U), binary32(0x43610c00U), binary32(0x4258c001U), binary32(0x3ece147bU), binary32(0x4009999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x43661800U), binary32(0x42a54001U), binary32(0x43661800U), binary32(0x42a54001U), binary32(0x3eceb852U), binary32(0x4011999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step4",
        world::Branch{
            63, binary32(0x43661800U), binary32(0x42a54001U), binary32(0x43661800U), binary32(0x42a54001U), binary32(0x3eceb852U), binary32(0x4011999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x436b2800U), binary32(0x42e14001U), binary32(0x436b2800U), binary32(0x42e14001U), binary32(0x3ecf5c29U), binary32(0x4019999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step5",
        world::Branch{
            63, binary32(0x436b2800U), binary32(0x42e14001U), binary32(0x436b2800U), binary32(0x42e14001U), binary32(0x3ecf5c29U), binary32(0x4019999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x43703c00U), binary32(0x43103001U), binary32(0x43703c00U), binary32(0x43103001U), binary32(0x3ed00000U), binary32(0x4021999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step6",
        world::Branch{
            63, binary32(0x43703c00U), binary32(0x43103001U), binary32(0x43703c00U), binary32(0x43103001U), binary32(0x3ed00000U), binary32(0x4021999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x43755400U), binary32(0x43315001U), binary32(0x43755400U), binary32(0x43315001U), binary32(0x3ed0a3d7U), binary32(0x4029999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step7",
        world::Branch{
            63, binary32(0x43755400U), binary32(0x43315001U), binary32(0x43755400U), binary32(0x43315001U), binary32(0x3ed0a3d7U), binary32(0x4029999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x437a7000U), binary32(0x43540001U), binary32(0x437a7000U), binary32(0x43540001U), binary32(0x3ed147aeU), binary32(0x4031999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step8",
        world::Branch{
            63, binary32(0x437a7000U), binary32(0x43540001U), binary32(0x437a7000U), binary32(0x43540001U), binary32(0x3ed147aeU), binary32(0x4031999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x437f9000U), binary32(0x43784001U), binary32(0x437f9000U), binary32(0x43784001U), binary32(0x3ed1eb85U), binary32(0x4039999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step9",
        world::Branch{
            63, binary32(0x437f9000U), binary32(0x43784001U), binary32(0x437f9000U), binary32(0x43784001U), binary32(0x3ed1eb85U), binary32(0x4039999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x43825a00U), binary32(0x438f0801U), binary32(0x43825a00U), binary32(0x438f0801U), binary32(0x3ed28f5cU), binary32(0x4041999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step10",
        world::Branch{
            63, binary32(0x43825a00U), binary32(0x438f0801U), binary32(0x43825a00U), binary32(0x438f0801U), binary32(0x3ed28f5cU), binary32(0x4041999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x4384ee00U), binary32(0x43a2b801U), binary32(0x4384ee00U), binary32(0x43a2b801U), binary32(0x3ed33333U), binary32(0x4049999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step11",
        world::Branch{
            63, binary32(0x4384ee00U), binary32(0x43a2b801U), binary32(0x4384ee00U), binary32(0x43a2b801U), binary32(0x3ed33333U), binary32(0x4049999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x43878400U), binary32(0x43b73001U), binary32(0x43878400U), binary32(0x43b73001U), binary32(0x3ed3d70aU), binary32(0x4051999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step12",
        world::Branch{
            63, binary32(0x43878400U), binary32(0x43b73001U), binary32(0x43878400U), binary32(0x43b73001U), binary32(0x3ed3d70aU), binary32(0x4051999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x438a1c00U), binary32(0x43cc7001U), binary32(0x438a1c00U), binary32(0x43cc7001U), binary32(0x3ed47ae1U), binary32(0x4059999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step13",
        world::Branch{
            63, binary32(0x438a1c00U), binary32(0x43cc7001U), binary32(0x438a1c00U), binary32(0x43cc7001U), binary32(0x3ed47ae1U), binary32(0x4059999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            63, binary32(0x438cb600U), binary32(0x43e27801U), binary32(0x438cb600U), binary32(0x43e27801U), binary32(0x3ed51eb8U), binary32(0x4061999aU), 0.0F, 3.0F, 1, 0, 0},
        false},
    UpdateCase{"sequence_slot22_step14",
        world::Branch{
            63, binary32(0x438cb600U), binary32(0x43e27801U), binary32(0x438cb600U), binary32(0x43e27801U), binary32(0x3ed51eb8U), binary32(0x4061999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 0,
        world::Branch{
            -1, binary32(0x438f5200U), binary32(0x43f94801U), binary32(0x438f5200U), binary32(0x43f94801U), binary32(0x3ed5c28fU), binary32(0x4069999aU), 0.0F, 3.0F, 1, 0, 0},
        true},
    UpdateCase{"sequence_slot22_after_removal",
        world::Branch{
            -1, binary32(0x438f5200U), binary32(0x43f94801U), binary32(0x438f5200U), binary32(0x43f94801U), binary32(0x3ed5c28fU), binary32(0x4069999aU), 0.0F, 3.0F, 1, 0, 0},
        0.125F, 0.0F, 8,
        world::Branch{
            -1, binary32(0x438f5200U), binary32(0x43f94801U), binary32(0x438f5200U), binary32(0x43f94801U), binary32(0x3ed5c28fU), binary32(0x4069999aU), 0.0F, 3.0F, 1, 0, 0},
        false}}};

const std::array<ImpactCase, 4> impacts_cases = {{ImpactCase{"slot20", world::Branch{22, 87.0F, 5.0F, 87.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, {{0U, 0U}},
                                                      world::Branch{62, 87.0F, 5.0F, 87.0F, 5.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}},
    ImpactCase{"slot21", world::Branch{22, 2007.0F, 5.0F, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, {{32767U, 32767U}},
        world::Branch{62, 2007.0F, 5.0F, 2007.0F, 5.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 3.0F, 1, 0, 0}},
    ImpactCase{"slot22", world::Branch{23, 215.0F, 2.0F, 215.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, {{0U, 0U}},
        world::Branch{63, 215.0F, 2.0F, 215.0F, 2.0F, binary32(0xbf000000U), 0.0F, 0.0F, 3.0F, 1, 0, 0}},
    ImpactCase{"slot23", world::Branch{23, 2135.0F, 2.0F, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, {{32767U, 32767U}},
        world::Branch{63, 2135.0F, 2.0F, 2135.0F, 2.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 3.0F, 1, 0, 0}}}};

const std::array<DrawCase, 16> draws_cases = {{DrawCase{world::Branch{22, 87.0F, 5.0F, binary32(0xbf600000U), binary32(0xbf200000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                                                   "gfx/branch1", {{0, 0, 78, 60}}, {{0, 0}}, true, false},
    DrawCase{
        world::Branch{22, 87.0F, 5.0F, binary32(0x4302c000U), binary32(0x43f47000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch1", {{0, 0, 78, 60}}, {{130, 488}}, true, false},
    DrawCase{world::Branch{62, 87.0F, 5.0F, binary32(0xbf600000U), binary32(0xbf200000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch1", {{0, 0, 78, 60}}, {{0, 0}}, true, false},
    DrawCase{
        world::Branch{62, 87.0F, 5.0F, binary32(0x4302c000U), binary32(0x43f47000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch1", {{0, 0, 78, 60}}, {{130, 488}}, true, false},
    DrawCase{
        world::Branch{22, 2007.0F, 5.0F, binary32(0xbf600000U), binary32(0xbf200000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch1", {{0, 0, 78, 60}}, {{0, 0}}, true, false},
    DrawCase{world::Branch{22, 2007.0F, 5.0F, binary32(0x4302c000U), binary32(0x43f47000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch1", {{0, 0, 78, 60}}, {{130, 488}}, true,
        false},
    DrawCase{
        world::Branch{62, 2007.0F, 5.0F, binary32(0xbf600000U), binary32(0xbf200000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch1", {{0, 0, 78, 60}}, {{0, 0}}, true, false},
    DrawCase{world::Branch{62, 2007.0F, 5.0F, binary32(0x4302c000U), binary32(0x43f47000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch1", {{0, 0, 78, 60}}, {{130, 488}}, true,
        false},
    DrawCase{
        world::Branch{23, 215.0F, 2.0F, binary32(0xbf600000U), binary32(0xbf200000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch2", {{0, 0, 118, 58}}, {{0, 0}}, true, false},
    DrawCase{world::Branch{23, 215.0F, 2.0F, binary32(0x4302c000U), binary32(0x43f47000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch2", {{0, 0, 118, 58}}, {{130, 488}}, true,
        false},
    DrawCase{
        world::Branch{63, 215.0F, 2.0F, binary32(0xbf600000U), binary32(0xbf200000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch2", {{0, 0, 118, 58}}, {{0, 0}}, true, false},
    DrawCase{world::Branch{63, 215.0F, 2.0F, binary32(0x4302c000U), binary32(0x43f47000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch2", {{0, 0, 118, 58}}, {{130, 488}}, true,
        false},
    DrawCase{
        world::Branch{23, 2135.0F, 2.0F, binary32(0xbf600000U), binary32(0xbf200000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch2", {{0, 0, 118, 58}}, {{0, 0}}, true, false},
    DrawCase{world::Branch{23, 2135.0F, 2.0F, binary32(0x4302c000U), binary32(0x43f47000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch2", {{0, 0, 118, 58}}, {{130, 488}}, true,
        false},
    DrawCase{
        world::Branch{63, 2135.0F, 2.0F, binary32(0xbf600000U), binary32(0xbf200000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch2", {{0, 0, 118, 58}}, {{0, 0}}, true, false},
    DrawCase{world::Branch{63, 2135.0F, 2.0F, binary32(0x4302c000U), binary32(0x43f47000U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, "gfx/branch2", {{0, 0, 118, 58}}, {{130, 488}}, true,
        false}}};

const std::array<ResetCase, 3> reset_cases = {
    {ResetCase{0.0F, 0.0F, 0,
         {{world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
             world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}},
         {{world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 51.0F, 708.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
             world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
             world::Branch{23, 2135.0F, 2.0F, 53.0F, 708.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}},
         {{world::Branch{22, 87.0F, 5.0F, 87.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
             world::Branch{23, 215.0F, 2.0F, 215.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{23, 2135.0F, 2.0F, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}}},
        ResetCase{0.0F, 1280.0F, 0,
            {{world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}},
            {{world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 51.0F, 708.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 2135.0F, 2.0F, 53.0F, 708.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}},
            {{world::Branch{22, 87.0F, 5.0F, -1961.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, -41.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 215.0F, 2.0F, -1833.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{23, 2135.0F, 2.0F, 87.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}}},
        ResetCase{0.0F, 640.6249389648438F, 0,
            {{world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 2135.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}},
            {{world::Branch{22, 87.0F, 5.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 51.0F, 708.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 215.0F, 2.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 2135.0F, 2.0F, 53.0F, 708.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}},
            {{world::Branch{22, 87.0F, 5.0F, -937.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}, world::Branch{22, 2007.0F, 5.0F, 983.0F, 5.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 215.0F, 2.0F, -809.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0},
                world::Branch{23, 2135.0F, 2.0F, 1111.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 0}}}}}};

void compare(const world::Branch& actual, const world::Branch& expected, std::string_view context) {
    require(actual == expected, std::string(context) + ": original state differs");
    require(std::bit_cast<std::uint32_t>(actual.world_x) == std::bit_cast<std::uint32_t>(expected.world_x)
                && std::bit_cast<std::uint32_t>(actual.world_y) == std::bit_cast<std::uint32_t>(expected.world_y)
                && std::bit_cast<std::uint32_t>(actual.screen_x) == std::bit_cast<std::uint32_t>(expected.screen_x)
                && std::bit_cast<std::uint32_t>(actual.screen_y) == std::bit_cast<std::uint32_t>(expected.screen_y)
                && std::bit_cast<std::uint32_t>(actual.velocity_x) == std::bit_cast<std::uint32_t>(expected.velocity_x)
                && std::bit_cast<std::uint32_t>(actual.velocity_y) == std::bit_cast<std::uint32_t>(expected.velocity_y)
                && std::bit_cast<std::uint32_t>(actual.animation_time) == std::bit_cast<std::uint32_t>(expected.animation_time)
                && std::bit_cast<std::uint32_t>(actual.animation_rate) == std::bit_cast<std::uint32_t>(expected.animation_rate),
        std::string(context) + ": original binary32 words differ");
}

void original_vectors() {
    const auto initial = world::initial_branches();
    for (std::size_t index = 0; index < initial.size(); ++index) {
        compare(initial[index], initial_branches_cases.at(index), "Fresh branch storage");
    }
    for (const auto& expected : updates_cases) {
        auto branch = expected.before;
        const auto effect = world::update_branch(branch, expected.delta, expected.camera, expected.recoil);
        const auto context = expected.id;
        compare(branch, expected.after, context);
        require(effect.ground_sound == expected.ground_sound, context + ": ground removal sound differs");
    }
    for (const auto& expected : impacts_cases) {
        auto branch = expected.before;
        world::hit_branch(branch, expected.random.at(0), expected.random.at(1));
        compare(branch, expected.after, expected.id);
    }
    for (const auto& expected : draws_cases) {
        const auto branch = expected.state;
        const auto draw = world::branch_draw(branch);
        const auto& source = expected.source;
        const auto& destination = expected.destination;
        require(draw.image == expected.image && draw.source.x == source.at(0) && draw.source.y == source.at(1) && draw.source.width == source.at(2)
                    && draw.source.height == source.at(3) && draw.destination.x == destination.at(0) && draw.destination.y == destination.at(1)
                    && draw.color_key == expected.color_key && draw.mirror_x == expected.mirror_x,
            "Original branch sheet, fixed cell, truncation or draw flags differ");
    }
    for (const auto& expected : reset_cases) {
        auto branches = world::initial_branches();
        for (std::size_t index = 0; index < branches.size(); ++index) {
            compare(branches[index], expected.native_reset.at(index), "Native reset storage");
            auto raw_reset = expected.raw_original_reset.at(index);
            require(!world::update_branch(branches[index], expected.delta, expected.camera, expected.recoil).ground_sound,
                "First normal branch update cannot ground a live reset branch");
            require(!world::update_branch(raw_reset, expected.delta, expected.camera, expected.recoil).ground_sound, "Raw original reset pose must normalize without removal");
            compare(branches[index], expected.first_normal_update.at(index), "Reset first normal update");
            require(branches[index] == raw_reset, "Unwritten partner poses must normalize before normal drawing");
        }
    }
}

using moorhuhn2::test::reject_unchanged;

void invalid_inputs() {
    const world::Branch live;
    auto falling = live;
    world::hit_branch(falling, 0, 0);
    const auto update = [](world::Branch& branch) {
        static_cast<void>(world::update_branch(branch, 0.125F, 640, 8));
    };
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        reject_unchanged(
            live,
            [delta](auto& branch) {
                static_cast<void>(world::update_branch(branch, delta, 0, 0));
            },
            "Invalid delta");
    }
    for (const float camera : {-1.0F, 1280.25F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        reject_unchanged(
            live,
            [camera](auto& branch) {
                static_cast<void>(world::update_branch(branch, 0.125F, camera, 0));
            },
            "Invalid camera");
    }
    for (const int recoil : {-1, 9}) {
        reject_unchanged(
            live,
            [recoil](auto& branch) {
                static_cast<void>(world::update_branch(branch, 0.125F, 0, recoil));
            },
            "Invalid recoil");
    }
    reject_unchanged(
        live,
        [](auto& branch) {
            world::hit_branch(branch, 32768, 0);
        },
        "Invalid x draw");
    reject_unchanged(
        live,
        [](auto& branch) {
            world::hit_branch(branch, 0, 32768);
        },
        "Invalid y draw");
    reject_unchanged(
        falling,
        [](auto& branch) {
            world::hit_branch(branch, 0, 0);
        },
        "Repeat falling impact");
    reject_unchanged(
        falling,
        [](auto& branch) {
            static_cast<void>(world::update_branch(branch, std::numeric_limits<float>::max(), 0, 0));
        },
        "Overflowing physics");
    auto inactive = falling;
    inactive.type = -1;
    reject_unchanged(
        inactive,
        [](auto& branch) {
            world::hit_branch(branch, 0, 0);
        },
        "Removed impact");
    reject_unchanged(
        inactive,
        [](auto& branch) {
            static_cast<void>(world::branch_draw(branch));
        },
        "Removed draw");
    for (const int type : {0, 21, 24, 61, 64}) {
        auto unsupported = live;
        unsupported.type = type;
        reject_unchanged(unsupported, update, "Unsupported type");
    }
    auto invalid = falling;
    invalid.frame = 1;
    reject_unchanged(invalid, update, "Unsupported animation frame");
    invalid = falling;
    invalid.world_x = std::numeric_limits<float>::infinity();
    reject_unchanged(invalid, update, "Nonfinite world pose");
    invalid = live;
    invalid.velocity_y = 1;
    reject_unchanged(invalid, update, "Live velocity");
    invalid = live;
    invalid.points = 1;
    reject_unchanged(
        invalid,
        [](auto& branch) {
            world::hit_branch(branch, 0, 0);
        },
        "Unsupported score field");
    invalid = falling;
    invalid.screen_x = 2147483648.0F;
    reject_unchanged(
        invalid,
        [](auto& branch) {
            static_cast<void>(world::branch_draw(branch));
        },
        "Unrepresentable integer draw pose");
}

} // namespace

int main() {
    try {
        original_vectors();
        invalid_inputs();
        std::cout << "Original four-branch initialization, live pose, zero-score impacts, stored fall/removal, fixed cells and reset passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
