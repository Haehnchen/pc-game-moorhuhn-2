#include "world/big_chicken.hpp"

#include "support/numeric.hpp"
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

struct SpawnCase {
    std::string id;
    std::uint32_t random;
    std::uint32_t camera_bits;
    world::BigChicken after;
    std::uint32_t pan_bits;
};

struct UpdateCase {
    std::string id;
    world::BigChicken before;
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    world::BigChicken after;
};

struct ImpactCase {
    std::string id;
    world::BigChicken before;
    world::BigChickenPopup popup;
    int points;
    world::BigChicken after;
};

const world::BigChicken expected_reset = world::BigChicken{-1, 0.0F, 100.0F, -1024.0F, -1024.0F, 0.0F, 0, 15, 5.0F, 25};

const std::array<SpawnCase, 5> spawns_cases = {
    {SpawnCase{"left-clamp", 0U, 0x44a00000U, world::BigChicken{4, 0.0F, 100.0F, -2048.0F, -1024.0F, 0.0F, 0, 15, 5.0F, 25}, 0xbf800000U},
        SpawnCase{"unclamped-center", 300U, 0x437a0000U, world::BigChicken{4, 300.0F, 100.0F, -100.0F, -1024.0F, 0.0F, 0, 15, 5.0F, 25}, 0xbed40000U},
        SpawnCase{"modulo-right-clamp", 32767U, 0x00000000U, world::BigChicken{4, 959.0F, 100.0F, 959.0F, -1024.0F, 0.0F, 0, 15, 5.0F, 25}, 0x3f800000U},
        SpawnCase{"maximum-world-x", 2271U, 0x00000000U, world::BigChicken{4, 2271.0F, 100.0F, 2271.0F, -1024.0F, 0.0F, 0, 15, 5.0F, 25}, 0x3f800000U},
        SpawnCase{"camera-retained-product", 300U, 0x3f1fffffU, world::BigChicken{4, 300.0F, 100.0F, 300.0F, -1024.0F, 0.0F, 0, 15, 5.0F, 25}, 0x3e580000U}}};

const std::array<UpdateCase, 15> updates_cases = {{UpdateCase{"initial-sentinel-refresh", world::BigChicken{4, 300.0F, 100.0F, -100.0F, -1024.0F, 0.0F, 0, 15, 5.0F, 25},
                                                       0x00000000U, 0x437a0000U, 8, world::BigChicken{4, 300.0F, 100.0F, -100.0F, 108.0F, 0.0F, 0, 15, 5.0F, 25}},
    UpdateCase{"open-last-cell", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 0.0F, 0, 15, 5.0F, 25}, 0x3f000000U, 0x437a0000U, 0,
        world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 10.0F, 10, 15, 5.0F, 25}},
    UpdateCase{"hold-start", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 0.0F, 0, 15, 5.0F, 25}, 0x3f800000U, 0x00000000U, 2,
        world::BigChicken{4, 300.0F, 100.0F, 300.0F, 102.0F, 20.0F, 11, 15, 5.0F, 25}},
    UpdateCase{"hold-last-entry", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 50.0F, 11, 15, 5.0F, 25}, 0x00000000U, 0x00000000U, 0,
        world::BigChicken{4, 300.0F, 100.0F, 300.0F, 100.0F, 50.0F, 11, 15, 5.0F, 25}},
    UpdateCase{"retreat-first-entry", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 51.0F, 11, 15, 5.0F, 25}, 0x00000000U, 0x44a00000U, 0,
        world::BigChicken{4, 300.0F, 100.0F, -1748.0F, 100.0F, 51.0F, 10, 15, 5.0F, 25}},
    UpdateCase{"retreat-last-entry", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 61.0F, 1, 15, 5.0F, 25}, 0x00000000U, 0x437a0000U, 0,
        world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 61.0F, 0, 15, 5.0F, 25}},
    UpdateCase{"live-clock-rounds-up-frame-stays-zero", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, binary32(0x33000000U), 0, 15, 5.0F, 25}, 0x3d4cccccU, 0x437a0000U, 0,
        world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 1.0F, 0, 15, 5.0F, 25}},
    UpdateCase{"live-reloads-rounded-clock", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 1.0F, 0, 15, 5.0F, 25}, 0x00000000U, 0x437a0000U, 0,
        world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 1.0F, 1, 15, 5.0F, 25}},
    UpdateCase{"retained-sentinel-edge-remains-live", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 61.0F, 0, 15, 5.0F, 25}, 0x3d4cccccU, 0x437a0000U, 0,
        world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 62.0F, 0, 15, 5.0F, 25}},
    UpdateCase{"stored-sentinel-edge-removes-next-update", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 62.0F, 0, 15, 5.0F, 25}, 0x00000000U, 0x437a0000U, 0,
        world::BigChicken{-1, 300.0F, 100.0F, -100.0F, 100.0F, 62.0F, -1, 15, 5.0F, 25}},
    UpdateCase{"bounded-large-delta-sentinel", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 0.0F, 0, 15, 5.0F, 25}, 0x40800000U, 0x437a0000U, 8,
        world::BigChicken{-1, 300.0F, 100.0F, -100.0F, 108.0F, 80.0F, -1, 15, 5.0F, 25}},
    UpdateCase{"death-retained-rounding", world::BigChicken{44, 300.0F, 100.0F, -100.0F, 100.0F, binary32(0x32800000U), 0, 16, 5.0F, 25}, 0x3d23d70aU, 0x437a0000U, 0,
        world::BigChicken{44, 300.0F, 100.0F, -100.0F, 100.0F, 1.0F, 0, 16, 5.0F, 25}},
    UpdateCase{"death-last-cell-rounded-clock", world::BigChicken{44, 300.0F, 100.0F, -100.0F, 100.0F, 0.0F, 0, 16, 5.0F, 25}, 0x3f23d70aU, 0x437a0000U, 8,
        world::BigChicken{44, 300.0F, 100.0F, -100.0F, 108.0F, 16.0F, 15, 16, 5.0F, 25}},
    UpdateCase{"death-rounded-clock-removes-next-update", world::BigChicken{44, 300.0F, 100.0F, -100.0F, 100.0F, 16.0F, 15, 16, 5.0F, 25}, 0x00000000U, 0x437a0000U, 0,
        world::BigChicken{-1, 300.0F, 100.0F, -100.0F, 100.0F, 16.0F, 16, 16, 5.0F, 25}},
    UpdateCase{"inactive-no-time-or-pose-change", world::BigChicken{-1, 300.0F, 100.0F, -100.0F, 100.0F, 16.0F, 16, 16, 5.0F, 25}, 0x3f800000U, 0x00000000U, 8,
        world::BigChicken{-1, 300.0F, 100.0F, -100.0F, 100.0F, 16.0F, 16, 16, 5.0F, 25}}}};

const std::array<ImpactCase, 2> impacts_cases = {
    {ImpactCase{"visible-hold-hit", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 100.0F, 40.0F, 11, 15, 5.0F, 25}, world::BigChickenPopup{435.0F, 292.0F, 25}, 25,
         world::BigChicken{44, 300.0F, 100.0F, -100.0F, 100.0F, 0.0F, 0, 16, 5.0F, 25}},
        ImpactCase{"retreat-hit", world::BigChicken{4, 300.0F, 100.0F, -100.0F, 108.0F, 61.0F, 0, 15, 5.0F, 25}, world::BigChickenPopup{435.0F, 292.0F, 25}, 25,
            world::BigChicken{44, 300.0F, 100.0F, -100.0F, 108.0F, 0.0F, 0, 16, 5.0F, 25}}}};

void draws(const world::BigChicken& chicken) {
    const auto draw = world::big_chicken_draw(chicken);
    const auto collision = world::big_chicken_collision(chicken);
    require(draw.has_value() == (chicken.type != -1), "Original draw eligibility differs");
    require(collision.has_value() == (chicken.type == 4), "Original collision eligibility differs");
    for (const auto& command : {draw, collision}) {
        if (!command) {
            continue;
        }
        const bool dead = chicken.type == 44;
        require(command->image == (dead ? "gfx/big_hit" : "gfx/big_appear") && command->source.x == 0 && command->source.y == chicken.frame * 384
                    && command->source.width == (dead ? 330 : 310) && command->source.height == 384,
            "Original big chicken cell differs");
        require(command->destination.x == static_cast<int>(chicken.screen_x) && command->destination.y == static_cast<int>(chicken.screen_y) && command->color_key
                    && !command->mirror_x,
            "Original big chicken pose/key differs");
    }
}

void original_vectors() {
    require(world::BigChicken{} == expected_reset, "Default big chicken slot is not inactive");
    for (const auto& value : spawns_cases) {
        const auto chicken = world::initial_big_chicken(value.random, binary32(value.camera_bits));
        require(chicken == value.after, value.id + ": original spawn fields differ");
        require(std::bit_cast<std::uint32_t>(world::big_chicken_spawn_pan(chicken)) == std::bit_cast<std::uint32_t>(binary32(value.pan_bits)),
            value.id + ": original spawn pan differs");
        draws(chicken);
    }
    for (const auto& value : updates_cases) {
        auto chicken = value.before;
        draws(chicken);
        world::update_big_chicken(chicken, binary32(value.delta_bits), binary32(value.camera_bits), value.recoil);
        require(chicken == value.after, value.id + ": original update fields differ");
        draws(chicken);
    }
    for (const auto& value : impacts_cases) {
        auto chicken = value.before;
        const auto impact = world::hit_big_chicken(chicken);
        const auto& popup = value.popup;
        require(impact == world::BigChickenImpact{value.points, {popup.world_x, popup.world_y, popup.points}}, value.id + ": original award/popup differs");
        require(chicken == value.after, value.id + ": original reaction fields differ");
        draws(chicken);
    }
}

template <class Action> void rejected_without_mutation(world::BigChicken chicken, Action action) {
    const auto before = chicken;
    bool rejected = false;
    try {
        action(chicken);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && chicken == before, "Invalid big chicken operation accepted or partially changed state");
}

void invalid_inputs() {
    const auto live = world::initial_big_chicken(300, 250);
    for (const float delta : {-0.1F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()}) {
        rejected_without_mutation(live, [delta](auto& chicken) {
            world::update_big_chicken(chicken, delta, 250, 0);
        });
    }
    for (const float camera : {-1.0F, 1281.0F, std::numeric_limits<float>::infinity()}) {
        rejected_without_mutation(live, [camera](auto& chicken) {
            world::update_big_chicken(chicken, 0, camera, 0);
        });
    }
    for (const int recoil : {-1, 9}) {
        rejected_without_mutation(live, [recoil](auto& chicken) {
            world::update_big_chicken(chicken, 0, 250, recoil);
        });
    }
    for (const int type : {-1, 5, 44, 45}) {
        auto chicken = live;
        chicken.type = type;
        if (type == 44) {
            chicken.frame_count = 16;
        }
        rejected_without_mutation(chicken, [](auto& value) {
            static_cast<void>(world::hit_big_chicken(value));
        });
    }
    auto dead = live;
    static_cast<void>(world::hit_big_chicken(dead));
    rejected_without_mutation(dead, [](auto& value) {
        world::update_big_chicken(value, 1000000000, 250, 0);
    });
    bool rejected = false;
    try {
        static_cast<void>(world::initial_big_chicken(32768, 250));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "Initialization accepted a draw outside original CRT range");
}

} // namespace

int main() {
    try {
        original_vectors();
        invalid_inputs();
        std::cout << "PASS: " << spawns_cases.size() << " original spawns," << updates_cases.size() << " updates," << impacts_cases.size()
                  << " impacts, draw/collision, reset and invalid-input atomicity\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
