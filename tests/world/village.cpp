#include "world/village.hpp"

#include "support/require.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

struct ImpactExpectation {
    int points;
    float extra_seconds;
    bool spawn_balloon;
    std::vector<std::string> sounds;
    std::optional<world::VillagePopup> popup;
};

struct ImpactCase {
    std::string id;
    world::Village before;
    int target;
    int frog_hits;
    world::Village after;
    ImpactExpectation effect;
};

struct SequenceCase {
    std::string id;
    std::vector<int> targets;
    std::vector<int> frog_hits;
    std::vector<int> expected_points;
    std::vector<int> expected_scores_after_hit;
    std::vector<bool> balloon_requests;
    world::Village final_state;
    float expected_remaining;
};

struct DrawCase {
    std::string id;
    float camera;
    int target;
    int recoil;
    std::array<int, 4> source;
    std::array<int, 2> destination;
    std::string image;
};

const std::array<ImpactCase, 18> impacts_cases = {{ImpactCase{"church31_first", world::Village{false, false, false, 25}, 0, 0, world::Village{true, false, false, 25},
                                                       ImpactExpectation{-25, 10.0F, false, {"churchbell"}, world::VillagePopup{720.0F, 177.0F, -25}}},
    ImpactCase{
        "church31_repeat", world::Village{true, false, false, 25}, 0, 0, world::Village{true, false, false, 25}, ImpactExpectation{0, 0.0F, false, {"churchbell"}, std::nullopt}},
    ImpactCase{"church32_first", world::Village{false, false, false, 25}, 1, 0, world::Village{false, true, false, 25},
        ImpactExpectation{15, 0.0F, false, {"churchbell_2"}, world::VillagePopup{633.0F, 182.0F, 15}}},
    ImpactCase{
        "church32_repeat", world::Village{false, true, false, 25}, 1, 0, world::Village{false, true, false, 25}, ImpactExpectation{0, 0.0F, false, {"churchbell_2"}, std::nullopt}},
    ImpactCase{"house_first", world::Village{false, false, false, 25}, 2, 0, world::Village{false, false, true, 25},
        ImpactExpectation{25, 0.0F, false, {"house_shot"}, world::VillagePopup{536.0F, 231.0F, 25}}},
    ImpactCase{"house_repeat_without_church31", world::Village{false, false, true, 25}, 2, 0, world::Village{false, false, true, 25},
        ImpactExpectation{0, 0.0F, false, {"house_shot"}, std::nullopt}},
    ImpactCase{"house_first_returns_even_with_church31", world::Village{true, false, false, 25}, 2, 0, world::Village{true, false, true, 25},
        ImpactExpectation{25, 0.0F, false, {"house_shot"}, world::VillagePopup{536.0F, 231.0F, 25}}},
    ImpactCase{"house_chain_25_to_20", world::Village{true, false, true, 25}, 2, 0, world::Village{true, false, true, 20},
        ImpactExpectation{20, 0.0F, true, {"house_shot", "churchbell", "churchbell_2"}, world::VillagePopup{536.0F, 231.0F, 20}}},
    ImpactCase{"house_chain_20_to_15", world::Village{true, false, true, 20}, 2, 0, world::Village{true, false, true, 15},
        ImpactExpectation{15, 0.0F, true, {"house_shot", "churchbell", "churchbell_2"}, world::VillagePopup{536.0F, 231.0F, 15}}},
    ImpactCase{"house_chain_15_to_10", world::Village{true, false, true, 15}, 2, 0, world::Village{true, false, true, 10},
        ImpactExpectation{10, 0.0F, true, {"house_shot", "churchbell", "churchbell_2"}, world::VillagePopup{536.0F, 231.0F, 10}}},
    ImpactCase{"house_chain_10_to_5", world::Village{true, false, true, 10}, 2, 0, world::Village{true, false, true, 5},
        ImpactExpectation{5, 0.0F, true, {"house_shot", "churchbell", "churchbell_2"}, world::VillagePopup{546.0F, 231.0F, 5}}},
    ImpactCase{"house_chain_5_to_0", world::Village{true, false, true, 5}, 2, 0, world::Village{true, false, true, 0},
        ImpactExpectation{0, 0.0F, true, {"house_shot", "churchbell", "churchbell_2"}, world::VillagePopup{546.0F, 231.0F, 0}}},
    ImpactCase{
        "house_chain_exhausted", world::Village{true, false, true, 0}, 2, 0, world::Village{true, false, true, 0}, ImpactExpectation{0, 0.0F, false, {"house_shot"}, std::nullopt}},
    ImpactCase{"house_chain_church32_blocks", world::Village{true, true, true, 25}, 2, 0, world::Village{true, true, true, 25},
        ImpactExpectation{0, 0.0F, false, {"house_shot"}, std::nullopt}},
    ImpactCase{"house_chain_frog_counter_1_blocks", world::Village{true, false, true, 25}, 2, 1, world::Village{true, false, true, 25},
        ImpactExpectation{0, 0.0F, false, {"house_shot"}, std::nullopt}},
    ImpactCase{"house_chain_frog_counter_2_blocks", world::Village{true, false, true, 25}, 2, 2, world::Village{true, false, true, 25},
        ImpactExpectation{0, 0.0F, false, {"house_shot"}, std::nullopt}},
    ImpactCase{"church31_after_house_chain", world::Village{false, true, true, 5}, 0, 2, world::Village{true, true, true, 5},
        ImpactExpectation{-25, 10.0F, false, {"churchbell"}, world::VillagePopup{720.0F, 177.0F, -25}}},
    ImpactCase{"church32_after_house_chain", world::Village{true, false, true, 10}, 1, 1, world::Village{true, true, true, 10},
        ImpactExpectation{15, 0.0F, false, {"churchbell_2"}, world::VillagePopup{633.0F, 182.0F, 15}}}}};

const std::array<SequenceCase, 4> sequences_cases = {
    {SequenceCase{"church31_house_chain", {0, 2, 2, 2, 2, 2, 2, 2}, {0, 0, 0, 0, 0, 0, 0, 0}, {-25, 25, 20, 15, 10, 5, 0, 0}, {-25, 25, 45, 60, 70, 75, 75, 75},
         {false, false, true, true, true, true, true, false}, world::Village{true, false, true, 0}, 100.0F},
        SequenceCase{
            "house_before_church31", {2, 2, 0, 2}, {0, 0, 0, 0}, {25, 0, -25, 20}, {25, 25, 0, 20}, {false, false, false, true}, world::Village{true, false, true, 20}, 100.0F},
        SequenceCase{"church32_cancels_chain", {0, 2, 2, 1, 2, 0, 2}, {0, 0, 0, 0, 0, 0, 0}, {-25, 25, 20, 15, 0, 0, 0}, {-25, 25, 45, 60, 60, 60, 60},
            {false, false, true, false, false, false, false}, world::Village{true, true, true, 20}, 100.0F},
        SequenceCase{"frog_cancels_chain", {0, 2, 2, 2, 2}, {0, 0, 0, 1, 2}, {-25, 25, 20, 0, 0}, {-25, 25, 45, 45, 45}, {false, false, true, false, false},
            world::Village{true, false, true, 20}, 100.0F}}};

const std::array<DrawCase, 30> draws_cases = {{DrawCase{"t0_camera_00000000_recoil0", 0.0F, 0, 0, {{0, 0, 20, 40}}, {{740, 157}}, "gfx/church"},
    DrawCase{"t1_camera_00000000_recoil0", 0.0F, 1, 0, {{0, 40, 20, 40}}, {{643, 162}}, "gfx/church"},
    DrawCase{"t2_camera_00000000_recoil0", 0.0F, 2, 0, {{0, 0, 13, 20}}, {{550, 221}}, "gfx/house"},
    DrawCase{"t0_camera_00000000_recoil8", 0.0F, 0, 8, {{0, 0, 20, 40}}, {{740, 161}}, "gfx/church"},
    DrawCase{"t1_camera_00000000_recoil8", 0.0F, 1, 8, {{0, 40, 20, 40}}, {{643, 166}}, "gfx/church"},
    DrawCase{"t2_camera_00000000_recoil8", 0.0F, 2, 8, {{0, 0, 13, 20}}, {{550, 225}}, "gfx/house"},
    DrawCase{"t0_camera_43a6aaab_recoil0", 333.3333435058594F, 0, 0, {{0, 0, 20, 40}}, {{340, 157}}, "gfx/church"},
    DrawCase{"t1_camera_43a6aaab_recoil0", 333.3333435058594F, 1, 0, {{0, 40, 20, 40}}, {{243, 162}}, "gfx/church"},
    DrawCase{"t2_camera_43a6aaab_recoil0", 333.3333435058594F, 2, 0, {{0, 0, 13, 20}}, {{150, 221}}, "gfx/house"},
    DrawCase{"t0_camera_43a6aaab_recoil8", 333.3333435058594F, 0, 8, {{0, 0, 20, 40}}, {{340, 161}}, "gfx/church"},
    DrawCase{"t1_camera_43a6aaab_recoil8", 333.3333435058594F, 1, 8, {{0, 40, 20, 40}}, {{243, 166}}, "gfx/church"},
    DrawCase{"t2_camera_43a6aaab_recoil8", 333.3333435058594F, 2, 8, {{0, 0, 13, 20}}, {{150, 225}}, "gfx/house"},
    DrawCase{"t0_camera_441c0aaa_recoil0", 624.1666259765625F, 0, 0, {{0, 0, 20, 40}}, {{-8, 157}}, "gfx/church"},
    DrawCase{"t1_camera_441c0aaa_recoil0", 624.1666259765625F, 1, 0, {{0, 40, 20, 40}}, {{-105, 162}}, "gfx/church"},
    DrawCase{"t2_camera_441c0aaa_recoil0", 624.1666259765625F, 2, 0, {{0, 0, 13, 20}}, {{-198, 221}}, "gfx/house"},
    DrawCase{"t0_camera_441c0aaa_recoil8", 624.1666259765625F, 0, 8, {{0, 0, 20, 40}}, {{-8, 161}}, "gfx/church"},
    DrawCase{"t1_camera_441c0aaa_recoil8", 624.1666259765625F, 1, 8, {{0, 40, 20, 40}}, {{-105, 166}}, "gfx/church"},
    DrawCase{"t2_camera_441c0aaa_recoil8", 624.1666259765625F, 2, 8, {{0, 0, 13, 20}}, {{-198, 225}}, "gfx/house"},
    DrawCase{"t0_camera_44200000_recoil0", 640.0F, 0, 0, {{0, 0, 20, 40}}, {{-28, 157}}, "gfx/church"},
    DrawCase{"t1_camera_44200000_recoil0", 640.0F, 1, 0, {{0, 40, 20, 40}}, {{-125, 162}}, "gfx/church"},
    DrawCase{"t2_camera_44200000_recoil0", 640.0F, 2, 0, {{0, 0, 13, 20}}, {{-218, 221}}, "gfx/house"},
    DrawCase{"t0_camera_44200000_recoil8", 640.0F, 0, 8, {{0, 0, 20, 40}}, {{-28, 161}}, "gfx/church"},
    DrawCase{"t1_camera_44200000_recoil8", 640.0F, 1, 8, {{0, 40, 20, 40}}, {{-125, 166}}, "gfx/church"},
    DrawCase{"t2_camera_44200000_recoil8", 640.0F, 2, 8, {{0, 0, 13, 20}}, {{-218, 225}}, "gfx/house"},
    DrawCase{"t0_camera_44a00000_recoil0", 1280.0F, 0, 0, {{0, 0, 20, 40}}, {{-796, 157}}, "gfx/church"},
    DrawCase{"t1_camera_44a00000_recoil0", 1280.0F, 1, 0, {{0, 40, 20, 40}}, {{-893, 162}}, "gfx/church"},
    DrawCase{"t2_camera_44a00000_recoil0", 1280.0F, 2, 0, {{0, 0, 13, 20}}, {{-986, 221}}, "gfx/house"},
    DrawCase{"t0_camera_44a00000_recoil8", 1280.0F, 0, 8, {{0, 0, 20, 40}}, {{-796, 161}}, "gfx/church"},
    DrawCase{"t1_camera_44a00000_recoil8", 1280.0F, 1, 8, {{0, 40, 20, 40}}, {{-893, 166}}, "gfx/church"},
    DrawCase{"t2_camera_44a00000_recoil8", 1280.0F, 2, 8, {{0, 0, 13, 20}}, {{-986, 225}}, "gfx/house"}}};

const world::Village expected_initial_state = world::Village{false, false, false, 25};

world::VillageTarget target(int value) {
    return static_cast<world::VillageTarget>(value);
}

std::string_view sound(world::VillageSound value) {
    switch (value) {
        case world::VillageSound::churchbell:
            return "churchbell";
        case world::VillageSound::churchbell_2:
            return "churchbell_2";
        case world::VillageSound::house_shot:
            return "house_shot";
    }
    throw std::runtime_error("Unknown village sound");
}

void check_effect(const world::VillageImpact& actual, const ImpactExpectation& expected, const std::string& context) {
    require(actual.points == expected.points && actual.extra_seconds == expected.extra_seconds && actual.spawn_balloon == expected.spawn_balloon,
        context + ": points, time or balloon request differ");
    require(actual.sound_count == expected.sounds.size(), context + ": sound count differs");
    for (std::size_t index = 0; index < actual.sound_count; ++index) {
        require(sound(actual.sounds[index]) == expected.sounds[index], context + ": sound order differs");
    }
    const auto& popup = expected.popup;
    require(actual.popup.has_value() == popup.has_value(), context + ": popup presence differs");
    if (actual.popup) {
        require(*actual.popup == world::VillagePopup{popup->world_x, popup->world_y, popup->points}, context + ": popup origin or points differ");
    }
}

void original_impacts() {
    for (const auto& expected : impacts_cases) {
        auto village = expected.before;
        const auto context = expected.id;
        const auto impact = world::hit_village(village, target(expected.target), expected.frog_hits);
        require(village == expected.after, context + ": original state transition differs");
        check_effect(impact, expected.effect, context);
    }
}

void original_sequences() {
    for (const auto& expected : sequences_cases) {
        world::Village village;
        int score = 0;
        float remaining = 90;
        for (std::size_t index = 0; index < expected.targets.size(); ++index) {
            // Original special processing clamps the preceding shot's negative
            // score before the next frame is composed and its shot resolved.
            if (score < 0) {
                score = 0;
            }
            const auto impact = world::hit_village(village, target(expected.targets[index]), expected.frog_hits[index]);
            score += impact.points;
            remaining += impact.extra_seconds;
            require(
                impact.points == expected.expected_points[index] && score == expected.expected_scores_after_hit[index] && impact.spawn_balloon == expected.balloon_requests[index],
                expected.id + ": ordered award or prerequisite differs");
        }
        require(village == expected.final_state && remaining == expected.expected_remaining, expected.id + ": final state or time differs");
    }
}

void original_draws() {
    for (const auto& expected : draws_cases) {
        const auto draw = world::village_draw(target(expected.target), expected.camera, expected.recoil);
        const auto& source = expected.source;
        const auto& position = expected.destination;
        require(draw.image == expected.image && draw.source.x == source[0] && draw.source.y == source[1] && draw.source.width == source[2] && draw.source.height == source[3]
                    && draw.destination.x == position[0] && draw.destination.y == position[1] && draw.color_key && !draw.mirror_x,
            expected.id + ": original frame, extended camera multiplication or stored recoil pose differs");
    }
}

void invalid_arguments() {
    world::Village village;
    const auto before = village;
    bool rejected = false;
    try {
        static_cast<void>(world::hit_village(village, static_cast<world::VillageTarget>(3), 0));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && village == before, "An invalid impact must fail before changing village state");
    rejected = false;
    try {
        static_cast<void>(world::hit_village(village, world::VillageTarget::house, -1));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && village == before, "An invalid prerequisite counter must fail before changing village state");
    rejected = false;
    try {
        static_cast<void>(world::village_draw(world::VillageTarget::first_church, 1281, 0));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "Village drawing must reject camera values outside the original range");
}

} // namespace

int main() {
    try {
        require(world::Village{} == expected_initial_state, "Village default state must match original round reset");
        original_impacts();
        original_sequences();
        original_draws();
        invalid_arguments();
        std::cout << "Original village impacts, sound order, house chain prerequisites, zero award, reset and camera/recoil poses passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
