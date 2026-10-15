#include "world/special_chicken.hpp"

#include "support/numeric.hpp"
#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct UpdateCase {
    std::string id;
    world::SpecialChicken before;
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    world::SpecialChicken after;
};

struct ImpactCase {
    world::SpecialChicken before;
    world::SpecialChicken after;
    world::SpecialChickenPopup popup;
    int points;
};

struct ArtCase {
    int frame;
    std::array<int, 4> source;
};

const world::SpecialChicken expected_initial = world::SpecialChicken{5, 2, 1447.0F, 72.0F, -1024.0F, -1024.0F, 0.0F, 4, 5, 5.0F, 20};

const std::array<int, 65> live_frames_cases = {{4, 3, 2, 1, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 7, 8, 9, 9, 9, 9, 8, 7, 6, 5, 5, 5, 5, 5, 10, 11, 12, 13, 14, 14, 14, 14, 13, 12,
    11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 16, 17, 18, 19, 18, 17, 16, 1, 2, 3, 4}};

const std::array<int, 5> death_frames_cases = {{20, 21, 22, 23, 24}};

const std::array<UpdateCase, 9> updates_cases = {{UpdateCase{"live-init-first-update", world::SpecialChicken{5, 2, 1447.0F, 72.0F, -1024.0F, -1024.0F, 0.0F, 4, 5, 5.0F, 20},
                                                      0x00000000U, 0x44200000U, 0, world::SpecialChicken{5, 2, 1447.0F, 72.0F, 423.0F, 72.0F, 0.0F, 4, 5, 5.0F, 20}},
    UpdateCase{"live-keeps-extended-index-at-six-point-one", world::SpecialChicken{5, 2, 1447.0F, 72.0F, -1024.0F, -1024.0F, 0.0F, 4, 5, 5.0F, 20}, 0x40c33333U, 0x44200000U, 8,
        world::SpecialChicken{5, 2, 1447.0F, 72.0F, 423.0F, 80.0F, 61.0F, 16, 5, 5.0F, 20}},
    UpdateCase{"next-update-reads-back-stored-clock", world::SpecialChicken{5, 2, 1447.0F, 72.0F, 423.0F, 80.0F, 61.0F, 16, 5, 5.0F, 20}, 0x00000000U, 0x44200000U, 0,
        world::SpecialChicken{5, 2, 1447.0F, 72.0F, 423.0F, 72.0F, 61.0F, 1, 5, 5.0F, 20}},
    UpdateCase{"camera-truncates-product-before-pose", world::SpecialChicken{5, 2, 1447.0F, 72.0F, -1024.0F, -1024.0F, 0.0F, 4, 5, 5.0F, 20}, 0x00000000U, 0x441fffffU, 1,
        world::SpecialChicken{5, 2, 1447.0F, 72.0F, 424.0F, 73.0F, 0.0F, 4, 5, 5.0F, 20}},
    UpdateCase{"live-last-cell", world::SpecialChicken{5, 2, 1447.0F, 72.0F, -1024.0F, -1024.0F, 64.0F, 3, 5, 5.0F, 20}, 0x00000000U, 0x44a00000U, 0,
        world::SpecialChicken{5, 2, 1447.0F, 72.0F, -601.0F, 72.0F, 64.0F, 4, 5, 5.0F, 20}},
    UpdateCase{"live-first-sentinel", world::SpecialChicken{5, 2, 1447.0F, 72.0F, -1024.0F, -1024.0F, 64.0F, 4, 5, 5.0F, 20}, 0x3dcccccdU, 0x43a00000U, 8,
        world::SpecialChicken{-1, 2, 1447.0F, 72.0F, 935.0F, 80.0F, 65.0F, -1, 5, 5.0F, 20}},
    UpdateCase{"death-first-update", world::SpecialChicken{45, 2, 1447.0F, 72.0F, 423.0F, 72.0F, 0.0F, 0, 5, 5.0F, 20}, 0x00000000U, 0x437a0000U, 8,
        world::SpecialChicken{45, 2, 1447.0F, 72.0F, 1047.0F, 80.0F, 0.0F, 20, 5, 5.0F, 20}},
    UpdateCase{"death-last-cell", world::SpecialChicken{45, 2, 1447.0F, 72.0F, 423.0F, 72.0F, 0.0F, 0, 5, 5.0F, 20}, 0x3effffffU, 0x447a0000U, 0,
        world::SpecialChicken{45, 2, 1447.0F, 72.0F, -153.0F, 72.0F, binary32(0x409fffffU), 24, 5, 5.0F, 20}},
    UpdateCase{"death-first-sentinel", world::SpecialChicken{45, 2, 1447.0F, 72.0F, 423.0F, 72.0F, 0.0F, 0, 5, 5.0F, 20}, 0x3f000000U, 0x447a0000U, 0,
        world::SpecialChicken{-1, 2, 1447.0F, 72.0F, -153.0F, 72.0F, 5.0F, -1, 5, 5.0F, 20}}}};

const ImpactCase expected_impact = ImpactCase{world::SpecialChicken{5, 2, 1447.0F, 72.0F, 1047.0F, 80.0F, binary32(0x425b0000U), 17, 5, 5.0F, 20},
    world::SpecialChicken{45, 2, 1447.0F, 72.0F, 1047.0F, 80.0F, 0.0F, 0, 5, 5.0F, 20}, world::SpecialChickenPopup{1527.0F, 138.0F, 20}, 20};

const std::array<ArtCase, 8> frames_cases = {
    {ArtCase{0, {{0, 0, 200, 133}}}, ArtCase{4, {{800, 0, 200, 133}}}, ArtCase{5, {{0, 133, 200, 133}}}, ArtCase{9, {{800, 133, 200, 133}}}, ArtCase{14, {{800, 266, 200, 133}}},
        ArtCase{19, {{800, 399, 200, 133}}}, ArtCase{20, {{0, 532, 200, 133}}}, ArtCase{24, {{800, 532, 200, 133}}}}};

void draw_eligibility(const world::SpecialChicken& chicken) {
    const auto draw = world::special_chicken_draw(chicken);
    const auto collision = world::special_chicken_collision(chicken);
    require(draw.has_value() == (chicken.type != -1) && collision.has_value() == (chicken.type == 5), "Original draw/selection eligibility differs");
    for (const auto& command : {draw, collision}) {
        if (command) {
            require(command->image == "gfx/specialhuhn" && command->color_key && !command->mirror_x, "Original special chicken sheet/key differs");
            require(command->destination.x == static_cast<int>(chicken.screen_x) && command->destination.y == static_cast<int>(chicken.screen_y),
                "Drawing did not use the retained pose");
        }
    }
}

void prerequisites_and_reset() {
    require(world::SpecialChicken{}.type == -1, "Fresh round exposes an unspawned target");
    for (const bool left : {false, true}) {
        for (const bool right : {false, true}) {
            for (const bool already_spawned : {false, true}) {
                auto spawned = already_spawned;
                const auto result = world::make_special_chicken(left, right, spawned);
                const bool expected = left && right && !already_spawned;
                require(result.has_value() == expected && spawned == (already_spawned || expected), "Both-crowns/one-shot spawn gate differs");
                if (result) {
                    require(*result == expected_initial, "Original initializer fields differ");
                }
            }
        }
    }
    bool spawned = false;
    auto chicken = *world::make_special_chicken(true, true, spawned);
    world::update_special_chicken(chicken, 6.5F, 640, 0);
    require(chicken.type == -1 && !world::make_special_chicken(true, true, spawned), "Unhit expiry clears the one-shot latch");
    spawned = false;
    require(world::make_special_chicken(true, true, spawned) == expected_initial, "New round does not restore spawn eligibility");
}

void original_updates() {
    for (const auto& value : updates_cases) {
        auto chicken = value.before;
        draw_eligibility(chicken);
        world::update_special_chicken(chicken, binary32(value.delta_bits), binary32(value.camera_bits), value.recoil);
        require(chicken == value.after, value.id + ": original update differs");
        draw_eligibility(chicken);
    }
    // Sample within each table interval using an exactly represented clock.
    // Expected frames are literal words independently extracted from the EXE.
    for (const int type : {5, 45}) {
        const auto& frames = type == 5 ? std::span<const int>(live_frames_cases) : std::span<const int>(death_frames_cases);
        for (std::size_t index = 0; index < frames.size(); ++index) {
            world::SpecialChicken chicken;
            chicken.type = type;
            chicken.frame = type == 5 ? 4 : 0;
            chicken.animation_time = static_cast<float>(index) + 0.25F;
            world::update_special_chicken(chicken, 0, 640, 0);
            require(chicken.type == type && chicken.frame == frames[index], "Original scripted animation table differs");
            draw_eligibility(chicken);
        }
    }
    auto inactive = world::SpecialChicken{};
    world::update_special_chicken(inactive, 100, 1280, 8);
    require(inactive == world::SpecialChicken{}, "Inactive slot advances");
}

void original_impact() {
    const auto& value = expected_impact;
    auto chicken = value.before;
    const auto impact = world::hit_special_chicken(chicken);
    require(chicken == value.after, "Hit does not reset into reaction45/frame0/time0");
    const auto& popup = value.popup;
    require(impact.points == value.points && impact.popup == popup, "Original score/popup differs");
    draw_eligibility(chicken);
    require(
        world::special_chicken_draw(chicken)->source.x == 0 && world::special_chicken_draw(chicken)->source.y == 0, "Immediately after impact the retained frame0 is not drawn");
    const auto before_repeat = chicken;
    bool rejected = false;
    try {
        static_cast<void>(world::hit_special_chicken(chicken));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && chicken == before_repeat, "Repeated hit accepts reaction45");
}

void art_rectangles() {
    for (const auto& value : frames_cases) {
        world::SpecialChicken chicken;
        chicken.frame = value.frame;
        chicken.type = chicken.frame < 20 ? 5 : 45;
        chicken.screen_x = 423;
        chicken.screen_y = 72;
        const auto draw = world::special_chicken_draw(chicken);
        const auto& source = value.source;
        require(draw && draw->source.x == source[0] && draw->source.y == source[1] && draw->source.width == source[2] && draw->source.height == source[3],
            "Original sampled art rectangle differs");
    }
}

template <class Action> void rejected_without_mutation(world::SpecialChicken chicken, Action action) {
    const auto before = chicken;
    bool rejected = false;
    try {
        action(chicken);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && chicken == before, "Invalid special chicken operation was accepted or partially changed state");
}

void invalid_inputs() {
    world::SpecialChicken chicken;
    chicken.type = 5;
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max()}) {
        rejected_without_mutation(chicken, [delta](auto& target) {
            world::update_special_chicken(target, delta, 640, 0);
        });
    }
    for (const float camera : {-1.0F, 1281.0F, std::numeric_limits<float>::quiet_NaN()}) {
        rejected_without_mutation(chicken, [camera](auto& target) {
            world::update_special_chicken(target, 0, camera, 0);
        });
    }
    rejected_without_mutation(chicken, [](auto& target) {
        world::update_special_chicken(target, 0, 640, 9);
    });
    chicken.type = 44;
    rejected_without_mutation(chicken, [](auto& target) {
        static_cast<void>(world::hit_special_chicken(target));
    });
    chicken.type = 5;
    chicken.frame = 20;
    rejected_without_mutation(chicken, [](auto& target) {
        world::update_special_chicken(target, 0, 640, 0);
    });
}

} // namespace

int main() {
    try {
        prerequisites_and_reset();
        original_updates();
        original_impact();
        art_rectangles();
        invalid_inputs();
        std::cout << "PASS: original spawn gate,70 scripted samples,9 updates,impact,8 art rectangles,reset and invalid-input atomicity\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
