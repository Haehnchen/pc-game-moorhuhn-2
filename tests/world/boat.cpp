#include "world/boat.hpp"

#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

struct ImpactCase {
    std::string id;
    world::Boat before;
    bool special_ammo;
    bool became_sinking;
    world::Boat after;
};

struct HitStep {
    int ammo_kind;
    int special_hits;
    bool sinking;
    int score_delta;
};

struct AnimationCase {
    std::string id;
    world::Boat before;
    float delta_seconds;
    world::Boat after;
    std::uint32_t animation_bits;
    std::array<int, 4> draw_source;
};

struct AnimationStep {
    float delta_seconds;
    float animation_time;
    int frame;
};

const std::array<ImpactCase, 7> selected_hit_cases = {{ImpactCase{"ordinary_at_counter_two", world::Boat{2, false, 0.0F, 0}, false, false, world::Boat{2, false, 0.0F, 0}},
    ImpactCase{"first_special", world::Boat{0, false, 0.0F, 0}, true, false, world::Boat{1, false, 0.0F, 0}},
    ImpactCase{"second_special", world::Boat{1, false, 0.0F, 0}, true, false, world::Boat{2, false, 0.0F, 0}},
    ImpactCase{"third_special", world::Boat{2, false, 0.0F, 0}, true, true, world::Boat{3, true, 0.0F, 0}},
    ImpactCase{"third_does_not_reset_animation", world::Boat{2, false, 0.75F, 7}, true, true, world::Boat{3, true, 0.75F, 7}},
    ImpactCase{"synthetic_fourth_does_not_award", world::Boat{3, false, 0.0F, 0}, true, false, world::Boat{4, false, 0.0F, 0}},
    ImpactCase{"sinking_rejects_further_selection", world::Boat{3, true, 10.0F, 10}, true, false, world::Boat{3, true, 10.0F, 10}}}};

const std::array<HitStep, 5> repeated_ordinary_special_sequence_cases = {
    {HitStep{0, 0, false, 0}, HitStep{1, 1, false, 0}, HitStep{0, 1, false, 0}, HitStep{1, 2, false, 0}, HitStep{2, 3, true, 50}}};

const world::Boat expected_reset = world::Boat{0, false, 0.0F, 0};

const std::array<AnimationCase, 12> sinking_updates_cases = {
    {AnimationCase{"zero", world::Boat{3, true, 0.0F, 0}, 0.0F, world::Boat{3, true, 0.0F, 0}, 0x00000000U, {{0, 0, 144, 79}}},
        AnimationCase{"one_tenth", world::Boat{3, true, 0.0F, 0}, 0.10000000149011612F, world::Boat{3, true, 1.0F, 1}, 0x3f800000U, {{0, 79, 144, 79}}},
        AnimationCase{"one_eighth", world::Boat{3, true, 0.0F, 0}, 0.125F, world::Boat{3, true, 1.25F, 1}, 0x3fa00000U, {{0, 79, 144, 79}}},
        AnimationCase{"seven_store_frame_six", world::Boat{3, true, 0.0F, 0}, 0.699999988079071F, world::Boat{3, true, 7.0F, 6}, 0x40e00000U, {{0, 474, 144, 79}}},
        AnimationCase{"nine_store_frame_eight", world::Boat{3, true, 0.0F, 0}, 0.8999999761581421F, world::Boat{3, true, 9.0F, 8}, 0x41100000U, {{0, 632, 144, 79}}},
        AnimationCase{"read_rounded_seven", world::Boat{3, true, 7.0F, 0}, 0.0F, world::Boat{3, true, 7.0F, 7}, 0x40e00000U, {{0, 553, 144, 79}}},
        AnimationCase{"exact_final_frame", world::Boat{3, true, 0.0F, 0}, 1.0F, world::Boat{3, true, 10.0F, 10}, 0x41200000U, {{0, 790, 144, 79}}},
        AnimationCase{"above_clamp", world::Boat{3, true, 0.0F, 0}, 2.0F, world::Boat{3, true, 20.0F, 10}, 0x41a00000U, {{0, 790, 144, 79}}},
        AnimationCase{"continue_after_clamp", world::Boat{3, true, 10.0F, 0}, 1.0F, world::Boat{3, true, 20.0F, 10}, 0x41a00000U, {{0, 790, 144, 79}}},
        AnimationCase{"retained_for_thirty_seconds", world::Boat{3, true, 20.0F, 0}, 30.0F, world::Boat{3, true, 320.0F, 10}, 0x43a00000U, {{0, 790, 144, 79}}},
        AnimationCase{
            "fractional_pre_store_crossing", world::Boat{3, true, 6.999999523162842F, 0}, 2.9802322387695312e-08F, world::Boat{3, true, 7.0F, 6}, 0x40e00000U, {{0, 474, 144, 79}}},
        AnimationCase{"live_update_does_not_animate", world::Boat{0, false, 0.0F, 0}, 1.0F, world::Boat{0, false, 0.0F, 0}, 0x00000000U, {{0, 0, 144, 79}}}}};

const std::array<AnimationStep, 19> animation_sequence_cases = {
    {AnimationStep{0.0625F, 0.625F, 0}, AnimationStep{0.0625F, 1.25F, 1}, AnimationStep{0.0625F, 1.875F, 1}, AnimationStep{0.0625F, 2.5F, 2}, AnimationStep{0.0625F, 3.125F, 3},
        AnimationStep{0.0625F, 3.75F, 3}, AnimationStep{0.0625F, 4.375F, 4}, AnimationStep{0.0625F, 5.0F, 5}, AnimationStep{0.0625F, 5.625F, 5}, AnimationStep{0.0625F, 6.25F, 6},
        AnimationStep{0.0625F, 6.875F, 6}, AnimationStep{0.0625F, 7.5F, 7}, AnimationStep{0.0625F, 8.125F, 8}, AnimationStep{0.0625F, 8.75F, 8}, AnimationStep{0.0625F, 9.375F, 9},
        AnimationStep{0.0625F, 10.0F, 10}, AnimationStep{0.0625F, 10.625F, 10}, AnimationStep{0.0625F, 11.25F, 10}, AnimationStep{30.0F, 311.25F, 10}}};

void check_impacts() {
    for (const auto& example : selected_hit_cases) {
        auto boat = example.before;
        const bool became_sinking = world::hit_boat(boat, example.special_ammo);
        const auto context = example.id;
        require(became_sinking == example.became_sinking, context + ": third-hit transition differs");
        require(boat == example.after, context + ": impact state differs");
    }
    world::Boat boat;
    for (const auto& step : repeated_ordinary_special_sequence_cases) {
        const bool became_sinking = world::hit_boat(boat, step.ammo_kind > 0);
        require(boat.special_hits == step.special_hits && boat.sinking == step.sinking, "Ordinary shots must preserve the accumulated special-hit count");
        require(became_sinking == (step.score_delta == 50), "Only the third special hit can authorize the shared score/popup event");
    }
    const auto sinking = boat;
    require(!world::hit_boat(boat, true) && boat == sinking, "A sinking boat must not trigger a repeated score event");
    require(!world::hit_boat(boat, false) && boat == sinking, "A sinking boat must preserve state after ordinary input");
    boat = {};
    require(boat == expected_reset, "New round must recreate the live zero-counter boat");
    require(!world::hit_boat(boat, true) && boat.special_hits == 1 && !boat.sinking, "New-round first special hit must begin a new three-hit sequence");
}

void check_animation() {
    for (const auto& example : sinking_updates_cases) {
        auto boat = example.before;
        world::update_boat(boat, example.delta_seconds);
        const auto context = example.id;
        require(boat == example.after, context + ": original-derived sinking state differs");
        const auto expected_bits = example.animation_bits;
        require(std::bit_cast<std::uint32_t>(boat.animation_time) == expected_bits, context + ": binary32 animation store differs");
        const auto draw = world::boat_draw(boat, 320, 0);
        const auto& source = example.draw_source;
        require(draw.image == "gfx/boat" && draw.source.x == source[0] && draw.source.y == source[1] && draw.source.width == source[2] && draw.source.height == source[3],
            context + ": sinking sprite cell differs");
        require(draw.destination.x == 179 && draw.destination.y == 264 && draw.color_key, context + ": sinking must retain world origin and source key");
        require(world::boat_draw(boat, 320, 8).destination.y == 270, context + ": sinking must retain the depth3 recoil transform");
        require(world::boat_collision(320, 0).source.y == 0, context + ": static collision art must remain cell zero");
    }

    world::Boat boat{.special_hits = 3, .sinking = true};
    for (const auto& step : animation_sequence_cases) {
        world::update_boat(boat, step.delta_seconds);
        require(boat.animation_time == step.animation_time && boat.frame == step.frame && boat.sinking && boat.special_hits == 3,
            "Exact update sequence must advance all eleven frames, clamp and retain the growing accumulator");
    }
    const auto final = boat;
    world::update_boat(boat, 0);
    require(boat == final, "The retained final frame must survive a zero-delta update");
    for (int frame = 0; frame <= 10; ++frame) {
        boat.frame = frame;
        const auto draw = world::boat_draw(boat, 0, 0);
        require(draw.source.y == frame * 79 && draw.source.width == 144 && draw.source.height == 79, "Each sinking frame must address its original vertical sheet cell");
    }
    world::Boat live;
    world::update_boat(live, 30);
    require(live == world::Boat{}, "Passing time must not animate or remove the live boat");
}

} // namespace

int main() {
    try {
        check_impacts();
        check_animation();
        std::cout << "Original boat ordinary/three-special/repeat/reset, exact sinking stores, all frames and retained finale passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
