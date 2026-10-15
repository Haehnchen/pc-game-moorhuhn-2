#include "world/airplane.hpp"

#include "support/numeric.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct SpawnCase {
    std::string id;
    world::AirplaneState before_state;
    std::vector<std::uint32_t> random;
    std::optional<world::Airplane> after;
    world::AirplaneState after_state;
    std::vector<world::AirplaneEvent> events;
};

struct UpdateCase {
    std::string id;
    world::Airplane before;
    world::AirplaneState before_state;
    std::vector<std::uint32_t> random;
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    world::Airplane after;
    world::AirplaneState after_state;
    std::vector<world::AirplaneEvent> events;
};

struct ImpactCase {
    std::string id;
    world::Airplane before;
    world::AirplaneState before_state;
    world::Airplane after;
    world::AirplaneState after_state;
    std::vector<world::AirplaneEvent> events;
};

struct SmokeSpawnCase {
    std::string id;
    std::vector<std::uint32_t> random;
    std::string family;
    int source_type;
    std::uint32_t world_x_bits;
    std::uint32_t world_y_bits;
    world::AircraftSmoke after;
};

struct SmokeUpdateCase {
    std::string id;
    world::AircraftSmoke before;
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    world::AircraftSmoke after;
};

const world::Airplane expected_reset = world::Airplane{-1, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25};

const world::AirplaneState expected_reset_state = world::AirplaneState{false, 0, false, false, false};

const std::array<SpawnCase, 5> spawns_cases = {
    {SpawnCase{"right-extremes", world::AirplaneState{false, 0, false, false, false}, {0U, 0U, 0U, 0U},
         world::Airplane{18, 4, -120.0F, 64.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
         {world::AirplaneEvent{world::AirplaneEventKind::start_engine, 0.0F, 0.0F, 0.0F, 0, 0}}},
        SpawnCase{"left-extremes", world::AirplaneState{false, 0, false, false, false}, {1U, 199U, 63U, 15U},
            world::Airplane{19, 4, 2599.0F, 1.0F, -1024.0F, -1024.0F, 15.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
            {world::AirplaneEvent{world::AirplaneEventKind::start_engine, 0.0F, 0.0F, 0.0F, 0, 0}}},
        SpawnCase{"crt-upper-limit", world::AirplaneState{false, 0, false, false, false}, {32767U, 32767U, 32767U, 32767U},
            world::Airplane{19, 4, 2567.0F, 1.0F, -1024.0F, -1024.0F, 15.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
            {world::AirplaneEvent{world::AirplaneEventKind::start_engine, 0.0F, 0.0F, 0.0F, 0, 0}}},
        SpawnCase{"next-plane-clears-hit-globals", world::AirplaneState{false, 4, true, false, false}, {2U, 201U, 65U, 17U},
            world::Airplane{18, 4, -121.0F, 63.0F, -1024.0F, -1024.0F, 1.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
            {world::AirplaneEvent{world::AirplaneEventKind::start_engine, 0.0F, 0.0F, 0.0F, 0, 0}}},
        SpawnCase{"existing-plane-rejects-without-rng", world::AirplaneState{true, 3, true, false, true}, {}, std::nullopt, world::AirplaneState{true, 3, true, false, true}, {}}}};

const std::array<UpdateCase, 22> updates_cases = {{UpdateCase{"inactive-keeps-storage", world::Airplane{-1, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
                                                       world::AirplaneState{false, 0, false, false, false}, {}, 0x3f800000U, 0x00000000U, 0,
                                                       world::Airplane{-1, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
                                                       world::AirplaneState{false, 0, false, false, false}, {}},
    UpdateCase{"right-pose-and-pan", world::Airplane{18, 4, 200.0F, 30.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x3e000000U, 0x42c90000U, 8,
        world::Airplane{18, 4, binary32(0x435ac000U), 30.0F, binary32(0x42c58000U), binary32(0x420b3333U), binary32(0xbe000000U), binary32(0x3fa00000U), 0.0F, 1, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbe8d0000U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"left-delayed-motion", world::Airplane{19, 4, 2200.0F, 10.0F, -1024.0F, -1024.0F, 2.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x3f800000U, 0x44a00000U, 3,
        world::Airplane{19, 4, 2050.0F, 10.0F, 514.0F, binary32(0x413ccccdU), 1.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0x3ebf3333U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"exact-wait-zero-does-not-animate", world::Airplane{18, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, binary32(0x3e000000U), 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x3e000000U, 0x00000000U, 0,
        world::Airplane{18, 4, binary32(0x41960000U), 0.0F, binary32(0x41960000U), 0.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbecd0000U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"extended-move-before-screen-subtraction", world::Airplane{18, 4, 240.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x33800000U, 0x43480000U, 0,
        world::Airplane{18, 4, binary32(0x43700001U), 0.0F, binary32(0x37160000U), 0.0F, binary32(0xb3800000U), binary32(0x36480000U), 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbedc0000U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"extended-animation-below-rounded-five", world::Airplane{18, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, binary32(0x409fffffU), 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x32000000U, 0x00000000U, 0,
        world::Airplane{18, 4, binary32(0x35960000U), 0.0F, binary32(0x35960000U), 0.0F, binary32(0xb2000000U), 5.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbedc0000U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"exact-animation-threshold", world::Airplane{18, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, binary32(0x3ff00000U), 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x3d800000U, 0x00000000U, 0,
        world::Airplane{18, 4, binary32(0x41160000U), 0.0F, binary32(0x41160000U), 0.0F, binary32(0xbd800000U), 0.0F, 0.0F, 1, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbed48000U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"one-frame-only-large-delta", world::Airplane{18, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x40a00000U, 0x00000000U, 0,
        world::Airplane{18, 4, 750.0F, 0.0F, 750.0F, 0.0F, -5.0F, 245.0F, 0.0F, 1, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0x3f3e0000U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"animation-wrap-draws-once", world::Airplane{19, 4, 1000.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 19, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {31U}, 0x3e000000U, 0x00000000U, 0,
        world::Airplane{19, 4, binary32(0x44755000U), 0.0F, binary32(0x44755000U), 0.0F, 15.0F, binary32(0x3fa00000U), 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, 1.0F, 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"right-exit-equality-live", world::Airplane{18, 4, 2400.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x00000000U, 0x00000000U, 0,
        world::Airplane{18, 4, 2400.0F, 0.0F, 2400.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, 1.0F, 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"left-exit-equality-live", world::Airplane{19, 4, -120.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x00000000U, 0x00000000U, 0,
        world::Airplane{19, 4, -120.0F, 0.0F, -120.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbf1e0000U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"right-next-float-exit", world::Airplane{18, 4, binary32(0x45160001U), 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x00000000U, 0x00000000U, 0,
        world::Airplane{-1, 4, binary32(0x45160001U), 0.0F, binary32(0x45160001U), 0.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{false, 0, false, false, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, 1.0F, 0.0F, 0.0F, 0, 0}, world::AirplaneEvent{world::AirplaneEventKind::stop_engine, 0.0F, 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"left-next-float-exit", world::Airplane{19, 4, binary32(0xc2f00001U), 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, false}, {}, 0x00000000U, 0x00000000U, 0,
        world::Airplane{-1, 4, binary32(0xc2f00001U), 0.0F, binary32(0xc2f00001U), 0.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25}, world::AirplaneState{false, 0, false, false, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbf1e0000U), 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::stop_engine, 0.0F, 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"both-retained-loops-pan-stop-order", world::Airplane{18, 4, 2401.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 0, false, true, true}, {}, 0x00000000U, 0x00000000U, 0, world::Airplane{-1, 4, 2401.0F, 0.0F, 2401.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{false, 0, false, false, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, 1.0F, 0.0F, 0.0F, 0, 0}, world::AirplaneEvent{world::AirplaneEventKind::pan_failure, 1.0F, 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::stop_engine, 0.0F, 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::stop_failure, 0.0F, 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"strict-smoke-exact-one", world::Airplane{18, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, binary32(0x3ec00000U), 0, 20, 5.0F, -25},
        world::AirplaneState{true, 1, false, true, false}, {}, 0x3d800000U, 0x00000000U, 0,
        world::Airplane{18, 4, binary32(0x41160000U), 0.0F, binary32(0x41160000U), 0.0F, binary32(0xbd800000U), binary32(0x40480000U), 1.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 1, false, true, false}, {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbed48000U), 0.0F, 0.0F, 0, 0}}},
    UpdateCase{"extended-smoke-above-rounded-one", world::Airplane{18, 4, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, binary32(0x3ec00001U), 0, 20, 5.0F, -25},
        world::AirplaneState{true, 1, false, true, false}, {}, 0x3d800000U, 0x00000000U, 0,
        world::Airplane{18, 4, binary32(0x41160000U), 0.0F, binary32(0x41160000U), 0.0F, binary32(0xbd800000U), binary32(0x40480000U), binary32(0x33000000U), 0, 20, 5.0F, -25},
        world::AirplaneState{true, 1, false, true, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbed48000U), 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::smoke, 0.0F, binary32(0x41160000U), 0.0F, 0, 18}}},
    UpdateCase{"hit-two-smoke-no-fall", world::Airplane{19, 4, 1000.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 2, false, true, false}, {}, 0x3e000000U, 0x00000000U, 0,
        world::Airplane{19, 4, binary32(0x44755000U), 60.0F, binary32(0x44755000U), 60.0F, binary32(0xbe000000U), binary32(0x3fa00000U), binary32(0x3fc00000U), 1, 20, 5.0F, -25},
        world::AirplaneState{true, 2, false, true, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, 1.0F, 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::smoke, 0.0F, binary32(0x44755000U), 60.0F, 0, 19}}},
    UpdateCase{"third-hit-delay-and-pre-fall-screen", world::Airplane{18, 4, 500.0F, 60.0F, -1024.0F, -1024.0F, 16.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 3, true, false, true}, {1U}, 0x3e000000U, 0x43480000U, 8,
        world::Airplane{18, 4, binary32(0x44017000U), 65.0F, binary32(0x438b6000U), binary32(0x4281999aU), binary32(0x417e0000U), 0.0F, binary32(0x40300000U), 0, 20, 5.0F, -25},
        world::AirplaneState{true, 3, true, false, true},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_failure, binary32(0x3bc00000U), 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::smoke, 0.0F, binary32(0x44017000U), 65.0F, 0, 18}}},
    UpdateCase{"fall-y-equality-retains-plane", world::Airplane{19, 4, 500.0F, 240.0F, -1024.0F, -1024.0F, 16.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 3, true, false, true}, {1U}, 0x3e800000U, 0x00000000U, 0,
        world::Airplane{19, 4, binary32(0x43e7c000U), 250.0F, binary32(0x43e74000U), 240.0F, binary32(0x417c0000U), 0.0F, binary32(0x40d00000U), 0, 20, 5.0F, -25},
        world::AirplaneState{true, 3, true, false, true},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_failure, binary32(0x3e960000U), 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::smoke, 0.0F, binary32(0x43e7c000U), 250.0F, 0, 19}}},
    UpdateCase{"fall-y-next-float-crash", world::Airplane{19, 4, 500.0F, binary32(0x43700001U), -1024.0F, -1024.0F, 16.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 3, true, false, true}, {0U}, 0x3e800000U, 0x00000000U, 0,
        world::Airplane{-1, 4, binary32(0x43e74000U), binary32(0x437a0001U), binary32(0x43e74000U), binary32(0x43700001U), binary32(0x417c0000U), 0.0F, binary32(0x40d00000U), 0,
            20, 5.0F, -25},
        world::AirplaneState{false, 3, true, false, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_failure, binary32(0x3e960000U), 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::crash, 0.0F, 0.0F, 0.0F, 0, 0}, world::AirplaneEvent{world::AirplaneEventKind::stop_failure, 0.0F, 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::smoke, 0.0F, binary32(0x43e74000U), binary32(0x437a0001U), 0, -1}}},
    UpdateCase{"exit-then-crash-still-animates-emits", world::Airplane{19, 4, -120.0F, 251.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 19, 20, 5.0F, -25},
        world::AirplaneState{true, 4, true, false, true}, {1U, 15U}, 0x3e000000U, 0x00000000U, 0,
        world::Airplane{-1, 4, binary32(0xc309c000U), 256.0F, binary32(0xc30ac000U), 251.0F, 15.0F, binary32(0x3fa00000U), 4.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{false, 4, true, false, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_failure, binary32(0xbf258000U), 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::stop_failure, 0.0F, 0.0F, 0.0F, 0, 0}, world::AirplaneEvent{world::AirplaneEventKind::crash, 0.0F, 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::smoke, 0.0F, binary32(0xc309c000U), 256.0F, 0, -1}}},
    UpdateCase{"removed-left-smoke-selects-default-origin", world::Airplane{19, 4, -121.0F, 50.0F, -1024.0F, -1024.0F, 16.0F, 0.0F, 0.0F, 0, 20, 5.0F, -25},
        world::AirplaneState{true, 1, false, true, false}, {}, 0x3e000000U, 0x00000000U, 0,
        world::Airplane{-1, 4, binary32(0xc30bc000U), 50.0F, binary32(0xc30bc000U), 50.0F, binary32(0x417e0000U), 0.0F, binary32(0x3e800000U), 0, 20, 5.0F, -25},
        world::AirplaneState{false, 1, false, false, false},
        {world::AirplaneEvent{world::AirplaneEventKind::pan_engine, binary32(0xbf25e667U), 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::stop_engine, 0.0F, 0.0F, 0.0F, 0, 0},
            world::AirplaneEvent{world::AirplaneEventKind::smoke, 0.0F, binary32(0xc30bc000U), 50.0F, 0, -1}}}}};

const std::array<ImpactCase, 5> impacts_cases = {
    {ImpactCase{"hit-1", world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 7.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 0, false, true, false},
         world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 0.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 1, false, true, false},
         {world::AirplaneEvent{world::AirplaneEventKind::shot, 0.0F, 0.0F, 0.0F, 0, 0}}},
        ImpactCase{"hit-2", world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 7.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 1, false, true, false},
            world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 0.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 2, false, true, false},
            {world::AirplaneEvent{world::AirplaneEventKind::shot, 0.0F, 0.0F, 0.0F, 0, 0}}},
        ImpactCase{"hit-3", world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 7.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 2, false, true, false},
            world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 16.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 3, true, false, true},
            {world::AirplaneEvent{world::AirplaneEventKind::shot, 0.0F, 0.0F, 0.0F, 0, 0}, world::AirplaneEvent{world::AirplaneEventKind::penalty, 0.0F, 1015.0F, 90.0F, -25, 0},
                world::AirplaneEvent{world::AirplaneEventKind::stop_engine, 0.0F, 0.0F, 0.0F, 0, 0},
                world::AirplaneEvent{world::AirplaneEventKind::start_failure, 0.0F, 0.0F, 0.0F, 0, 0}}},
        ImpactCase{"hit-4", world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 7.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 3, true, false, true},
            world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 0.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 4, true, false, true},
            {world::AirplaneEvent{world::AirplaneEventKind::shot, 0.0F, 0.0F, 0.0F, 0, 0}}},
        ImpactCase{"hit-8", world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 7.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 7, true, false, true},
            world::Airplane{19, 4, 1000.0F, 40.0F, -1024.0F, -1024.0F, 0.0F, 2.0F, 0.0F, 14, 20, 5.0F, -25}, world::AirplaneState{true, 8, true, false, true},
            {world::AirplaneEvent{world::AirplaneEventKind::shot, 0.0F, 0.0F, 0.0F, 0, 0}}}}};

const std::array<SmokeSpawnCase, 8> smoke_spawns_cases = {{SmokeSpawnCase{"plane-18-0", {0U, 0U}, "airplane", 18, 0x42c80000U, 0x42700000U,
                                                               world::AircraftSmoke{5, 171.0F, 110.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbdccccccU), 0.0F, 5}},
    SmokeSpawnCase{"plane-18-31", {31U, 19U}, "airplane", 18, 0x42c80000U, 0x42700000U,
        world::AircraftSmoke{5, 171.0F, 110.0F, -1024.0F, -1024.0F, binary32(0xbf1eb852U), binary32(0x3db851ebU), 0.0F, 5}},
    SmokeSpawnCase{
        "plane-19-0", {0U, 0U}, "airplane", 19, 0x42c80000U, 0x42700000U, world::AircraftSmoke{5, 115.0F, 110.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbdccccccU), 0.0F, 5}},
    SmokeSpawnCase{"plane-19-31", {31U, 19U}, "airplane", 19, 0x42c80000U, 0x42700000U,
        world::AircraftSmoke{5, 115.0F, 110.0F, -1024.0F, -1024.0F, binary32(0x3f1eb852U), binary32(0x3db851ebU), 0.0F, 5}},
    SmokeSpawnCase{
        "plane--1-0", {0U, 0U}, "airplane", -1, 0x42c80000U, 0x42700000U, world::AircraftSmoke{5, 171.0F, 110.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbdccccccU), 0.0F, 5}},
    SmokeSpawnCase{"plane--1-31", {31U, 19U}, "airplane", -1, 0x42c80000U, 0x42700000U,
        world::AircraftSmoke{5, 171.0F, 110.0F, -1024.0F, -1024.0F, binary32(0xbf1eb852U), binary32(0x3db851ebU), 0.0F, 5}},
    SmokeSpawnCase{"blimp-0", {0U, 0U}, "blimp", 0, 0x42c90000U, 0x42710000U,
        world::AircraftSmoke{4, binary32(0x42c90000U), binary32(0x42710000U), -1024.0F, -1024.0F, binary32(0xbea3d70aU), binary32(0xbd4cccccU), 0.0F, 4}},
    SmokeSpawnCase{"blimp-63", {63U, 19U}, "blimp", 0, 0x42c90000U, 0x42710000U,
        world::AircraftSmoke{4, binary32(0x42c90000U), binary32(0x42710000U), -1024.0F, -1024.0F, binary32(0x3e9eb852U), binary32(0x3e0f5c29U), 0.0F, 4}}}};

const std::array<SmokeUpdateCase, 8> smoke_updates_cases = {{SmokeUpdateCase{"inactive", world::AircraftSmoke{-1, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 5}, 0x3f800000U,
                                                                 0x00000000U, 0, world::AircraftSmoke{-1, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 5}},
    SmokeUpdateCase{"plane-zero-velocity-moves-negative", world::AircraftSmoke{5, 171.0F, 110.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 5}, 0x3e000000U, 0x42c90000U, 8,
        world::AircraftSmoke{
            5, binary32(0x432aec00U), binary32(0x42dbf800U), binary32(0x424bb000U), binary32(0x42e5919aU), binary32(0xbbcccccdU), binary32(0xbaa3d70aU), binary32(0x3e000000U), 5}},
    SmokeUpdateCase{"plane-positive-acceleration", world::AircraftSmoke{5, 115.0F, 110.0F, -1024.0F, -1024.0F, binary32(0x3f1eb852U), binary32(0x3db851ecU), 0.0F, 5}, 0x3e800000U,
        0x43480000U, 3,
        world::AircraftSmoke{
            5, binary32(0x4302d000U), binary32(0x42e0a000U), binary32(0xc2da6000U), binary32(0x42e4399aU), binary32(0x3f21eb85U), binary32(0x3dbd70a4U), binary32(0x3e800000U), 5}},
    SmokeUpdateCase{"plane-exact-two-seconds-live", world::AircraftSmoke{5, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, binary32(0x3fc00000U), 5}, 0x3f000000U, 0x00000000U, 0,
        world::AircraftSmoke{5, binary32(0xbfa00000U), binary32(0xbe800000U), binary32(0xbfa00000U), binary32(0xbe800000U), binary32(0xbccccccdU), binary32(0xbba3d70aU), 2.0F, 5}},
    SmokeUpdateCase{"plane-extended-age-above-rounded-two", world::AircraftSmoke{5, 0.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 2.0F, 5}, 0x33000000U, 0x00000000U, 0,
        world::AircraftSmoke{
            -1, binary32(0xa7a00000U), binary32(0xa6800000U), binary32(0xa7a00000U), binary32(0xa6800000U), binary32(0xb0cccccdU), binary32(0xafa3d70aU), 2.0F, 5}},
    SmokeUpdateCase{"blimp-upward-acceleration",
        world::AircraftSmoke{4, binary32(0x42c90000U), binary32(0x42710000U), -1024.0F, -1024.0F, binary32(0xbea3d70aU), binary32(0xbd4ccccdU), 0.0F, 4}, 0x3e800000U, 0x43480000U,
        8,
        world::AircraftSmoke{4, binary32(0x42b8e000U), binary32(0x42670000U), binary32(0xc3139000U), binary32(0x427a3333U), binary32(0xbea51eb8U), binary32(0xbdcccccdU), 0.0F, 4}},
    SmokeUpdateCase{"blimp-exact-minus-eight-live", world::AircraftSmoke{4, 0.0F, -8.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 4}, 0x00000000U, 0x00000000U, 0,
        world::AircraftSmoke{4, 0.0F, -8.0F, 0.0F, -8.0F, 0.0F, 0.0F, 0.0F, 4}},
    SmokeUpdateCase{"blimp-next-float-below-minus-eight-removes", world::AircraftSmoke{4, 0.0F, binary32(0xc1000001U), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 4}, 0x00000000U,
        0x00000000U, 0, world::AircraftSmoke{-1, 0.0F, binary32(0xc1000001U), 0.0F, binary32(0xc1000001U), 0.0F, 0.0F, 0.0F, 4}}}};

void events(std::span<const world::AirplaneEvent> actual, std::span<const world::AirplaneEvent> expected, std::string_view id) {
    require(std::ranges::equal(actual, expected), std::string(id) + ": original command order/payload differs");
}

struct Random {
    const std::vector<std::uint32_t>& values;
    std::size_t consumed{};

    std::uint32_t operator()() {
        require(consumed < values.size(), "Unexpected aircraft random draw");
        return values[consumed++];
    }
};

void draw(const world::Airplane& airplane) {
    for (const auto& command : {world::airplane_draw(airplane), world::airplane_collision(airplane)}) {
        require(command.has_value() == (airplane.type != -1), "Airplane collision/draw eligibility differs");
        if (!command) {
            continue;
        }
        require(command->image == (airplane.type == 18 ? "gfx/rplane" : "gfx/lplane") && command->source.width == 90 && command->source.height == 100
                    && command->source.x == airplane.frame % 10 * 90 && command->source.y == airplane.frame / 10 * 100,
            "Original directional plane cell differs");
        require(command->destination.x == static_cast<int>(airplane.screen_x) && command->destination.y == static_cast<int>(airplane.screen_y) && command->color_key
                    && !command->mirror_x,
            "Original plane pose/key differs");
    }
}

void draw(const world::AircraftSmoke& value) {
    const auto command = world::aircraft_smoke_draw(value);
    require(command.has_value() == (value.type != -1), "Aircraft smoke draw eligibility differs");
    if (command) {
        require(command->image == "gfx/particle2" && command->source.x == 0 && command->source.y == value.frame * 8 && command->source.width == 8 && command->source.height == 8
                    && command->color_key && !command->mirror_x && command->destination.x == static_cast<int>(value.screen_x)
                    && command->destination.y == static_cast<int>(value.screen_y),
            "Original aircraft smoke cell/pose differs");
    }
}

void vectors() {
    require(world::Airplane{} == expected_reset && world::AirplaneState{} == expected_reset_state, "Airplane defaults retain a round");
    for (const auto& value : spawns_cases) {
        auto state = value.before_state;
        Random random{value.random};
        std::vector<world::AirplaneEvent> commands;
        const auto result = world::make_airplane(
            state,
            [&] {
                return random();
            },
            commands);
        require(result.has_value() == value.after.has_value(), value.id + ": spawn gate differs");
        if (result) {
            require(*result == *value.after, value.id + ": original spawn fields differ");
            draw(*result);
        }
        require(state == value.after_state && random.consumed == random.values.size(), "Spawn globals or random order differs");
        events(commands, value.events, value.id);
    }
    for (const auto& value : updates_cases) {
        auto airplane = value.before;
        auto state = value.before_state;
        Random random{value.random};
        const auto commands = world::update_airplane(airplane, state, binary32(value.delta_bits), binary32(value.camera_bits), value.recoil, [&] {
            return random();
        });
        require(airplane == value.after, value.id + ": original update fields differ");
        require(state == value.after_state && random.consumed == random.values.size(), "Update globals or random order differs");
        events(commands, value.events, value.id);
        draw(airplane);
    }
    for (const auto& value : impacts_cases) {
        auto airplane = value.before;
        auto state = value.before_state;
        const auto commands = world::hit_airplane(airplane, state);
        require(airplane == value.after && state == value.after_state, value.id + ": original impact differs");
        events(commands, value.events, value.id);
        draw(airplane);
    }
    for (const auto& value : smoke_spawns_cases) {
        Random random{value.random};
        const world::AircraftRandom next = [&] {
            return random();
        };
        const auto result = value.family == "airplane" ? world::make_airplane_smoke(value.source_type, binary32(value.world_x_bits), binary32(value.world_y_bits), next)
                                                       : world::make_blimp_smoke(binary32(value.world_x_bits), binary32(value.world_y_bits), next);
        require(result == value.after && random.consumed == random.values.size(), value.id + ": smoke initializer differs");
        draw(result);
    }
    for (const auto& value : smoke_updates_cases) {
        auto result = value.before;
        world::update_aircraft_smoke(result, binary32(value.delta_bits), binary32(value.camera_bits), value.recoil);
        require(result == value.after, value.id + ": smoke motion/expiry differs");
        draw(result);
    }
}

void full_update_batch() {
    world::Airplane airplane;
    airplane.type = 18;
    airplane.world_x = 2400;
    airplane.world_y = 250;
    world::AirplaneState state{true, 3, true, true, true};
    const auto result = world::update_airplane(airplane, state, .125F, 0, 0, [] {
        return 0;
    });
    const std::array expected{world::AirplaneEventKind::pan_engine, world::AirplaneEventKind::pan_failure, world::AirplaneEventKind::stop_engine,
        world::AirplaneEventKind::stop_failure, world::AirplaneEventKind::crash, world::AirplaneEventKind::smoke};
    require(std::ranges::equal(result, expected, {}, &world::AirplaneEvent::kind), "Simultaneous exit, crash and smoke must retain all six ordered events");
}

void rejected_inputs() {
    world::AirplaneState state;
    std::vector<world::AirplaneEvent> commands;
    const auto fresh = world::make_airplane(
        state,
        [] {
            return 0;
        },
        commands);
    require(fresh && state.active && state.hits == 0 && !state.penalty_applied, "Next round cannot spawn plane");
    for (const float delta : {-0.1F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()}) {
        auto airplane = *fresh;
        const auto before = airplane;
        const auto original = state;
        bool rejected = false;
        try {
            static_cast<void>(world::update_airplane(airplane, state, delta, 0, 0, [] {
                return 0;
            }));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && airplane == before && state == original, "Invalid update accepted or mutated model");
    }
    bool rejected = false;
    auto vacant = world::AirplaneState{};
    try {
        static_cast<void>(world::make_airplane(
            vacant,
            [] {
                return 32768;
            },
            commands));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && vacant == world::AirplaneState{}, "Non-CRT random input accepted");
    rejected = false;
    auto inactive = world::Airplane{};
    try {
        static_cast<void>(world::hit_airplane(inactive, state));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "Inactive airplane remains selectable");
}

} // namespace

int main() {
    try {
        vectors();
        full_update_batch();
        rejected_inputs();
        std::cout << "PASS: " << spawns_cases.size() << " spawns," << updates_cases.size() << " updates," << impacts_cases.size() << " hits," << smoke_spawns_cases.size()
                  << " smoke spawns," << smoke_updates_cases.size() << " smoke updates, draw/collision, rejected inputs\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
