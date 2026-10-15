#include "world/notice.hpp"

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
    std::string id;
    world::Notice before;
    float delta;
    float application_time;
    world::Notice after;
    std::optional<world::NoticeText> text;
};

struct TextCase {
    std::string id;
    world::Notice state;
    std::optional<world::NoticeText> text;
};

struct PoolStep {
    std::string action;
    int type;
    float y;
    bool success;
    float delta;
    float application_time;
    std::array<world::Notice, 8> after;
};

struct PoolSequence {
    std::array<world::Notice, 8> initial;
    std::vector<PoolStep> steps;
};

const std::array<UpdateCase, 19> updates_cases = {{UpdateCase{"newborn_same_update", world::Notice{true, false, 0.0F, 160.0F, 3}, 0.125F, 0.0F,
                                                       world::Notice{true, true, binary32(0x3e000000U), 155.0F, 3}, world::NoticeText{"TRIPLE HIT +5", 200, 155}},
    UpdateCase{"inclusive_three_seconds", world::Notice{true, false, 0.0F, 160.0F, 4}, 3.0F, 0.0F, world::Notice{true, true, 3.0F, 40.0F, 4},
        world::NoticeText{"UNBELIEVABLE +10", 170, 40}},
    UpdateCase{"expire_but_draw", world::Notice{true, true, 3.0F, 40.0F, 5}, 0.125F, 0.0F, world::Notice{false, true, binary32(0x40480000U), 35.0F, 5},
        world::NoticeText{"MOORHUHN MASTER +15", 140, 35}},
    UpdateCase{"next_inactive_update", world::Notice{false, true, binary32(0x40480000U), 35.0F, 5}, 0.125F, 0.0F, world::Notice{false, false, binary32(0x40480000U), 35.0F, 5},
        std::nullopt},
    UpdateCase{"expiry_before_binary32_store", world::Notice{true, false, 3.0F, 40.0F, 6}, 2.9802322387695312e-08F, 0.0F, world::Notice{false, true, 3.0F, 40.0F, 6},
        world::NoticeText{"I'M NOT WORTHY +20", 150, 40}},
    UpdateCase{"active_below_three_before_binary32_store", world::Notice{true, false, 2.999999761581421F, 60.0F, 101}, binary32(0x343fffffU), 0.0F,
        world::Notice{true, true, 3.0F, binary32(0x426ffffeU), 101}, world::NoticeText{"SPIDER HERO +150", 170, 59}},
    UpdateCase{"blink_last_visible_step", world::Notice{true, false, 0.0F, 160.0F, 3}, 0.0F, 0.25F, world::Notice{true, true, 0.0F, 160.0F, 3},
        world::NoticeText{"TRIPLE HIT +5", 200, 160}},
    UpdateCase{"blink_first_hidden_step", world::Notice{true, true, 0.0F, 160.0F, 3}, 0.0F, 0.28125F, world::Notice{true, false, 0.0F, 160.0F, 3}, std::nullopt},
    UpdateCase{"blink_before_wrap", world::Notice{true, true, 0.0F, 160.0F, 3}, 0.0F, 0.515625F, world::Notice{true, false, 0.0F, 160.0F, 3}, std::nullopt},
    UpdateCase{"blink_last_hidden_step", world::Notice{true, true, 0.0F, 160.0F, 3}, 0.0F, binary32(0x3f080000U), world::Notice{true, false, 0.0F, 160.0F, 3}, std::nullopt},
    UpdateCase{"blink_wrap_visible", world::Notice{true, false, 0.0F, 160.0F, 3}, 0.0F, 0.546875F, world::Notice{true, true, 0.0F, 160.0F, 3},
        world::NoticeText{"TRIPLE HIT +5", 200, 160}},
    UpdateCase{"binary32_sample_below_sixteen", world::Notice{true, false, 0.0F, 160.0F, 3}, 0.0F, 0.2666666507720947F, world::Notice{true, true, 0.0F, 160.0F, 3},
        world::NoticeText{"TRIPLE HIT +5", 200, 160}},
    UpdateCase{"binary32_sample_above_sixteen", world::Notice{true, true, 0.0F, 160.0F, 3}, 0.0F, 0.2666666805744171F, world::Notice{true, false, 0.0F, 160.0F, 3}, std::nullopt},
    UpdateCase{"unknown_thirteen_ages", world::Notice{true, false, 0.0F, 160.0F, 13}, 0.5F, 0.0F, world::Notice{true, true, binary32(0x3f000000U), 140.0F, 13}, std::nullopt},
    UpdateCase{"unknown_thirty_two_expires", world::Notice{true, false, binary32(0x40380000U), 45.0F, 32}, 0.25F, 0.0F,
        world::Notice{false, true, binary32(0x40480000U), 35.0F, 32}, std::nullopt},
    UpdateCase{"hero_preserves_y180", world::Notice{true, false, 0.0F, 180.0F, 101}, 0.125F, 0.0F, world::Notice{true, true, binary32(0x3e000000U), 175.0F, 101},
        world::NoticeText{"SPIDER HERO +150", 170, 175}},
    UpdateCase{"stored_motion_fraction", world::Notice{true, false, binary32(0x3dcccccdU), 156.0F, 12}, 0.10000000149011612F, 0.125F,
        world::Notice{true, true, binary32(0x3e4ccccdU), 152.0F, 12}, world::NoticeText{"I'M NOT WORTHY +20", 150, 152}},
    UpdateCase{"negative_y_truncation", world::Notice{true, false, binary32(0x40200000U), binary32(0x3e800000U), 3}, 0.02500000037252903F, 0.0F,
        world::Notice{true, true, binary32(0x4021999aU), binary32(0xbf400000U), 3}, world::NoticeText{"TRIPLE HIT +5", 200, 0}},
    UpdateCase{"inactive_stale_fields", world::Notice{false, true, binary32(0x3fc00000U), -5.0F, 101}, 2.0F, 0.5F, world::Notice{false, false, binary32(0x3fc00000U), -5.0F, 101},
        std::nullopt}}};

const std::array<TextCase, 24> texts_cases = {{TextCase{"type_-1", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), -1}, std::nullopt},
    TextCase{"type_0", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 0}, std::nullopt},
    TextCase{"type_2", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 2}, std::nullopt},
    TextCase{"type_3", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 3}, world::NoticeText{"TRIPLE HIT +5", 200, 149}},
    TextCase{"type_4", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 4}, world::NoticeText{"UNBELIEVABLE +10", 170, 149}},
    TextCase{"type_5", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 5}, world::NoticeText{"MOORHUHN MASTER +15", 140, 149}},
    TextCase{"type_6", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 6}, world::NoticeText{"I'M NOT WORTHY +20", 150, 149}},
    TextCase{"type_7", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 7}, world::NoticeText{"I'M NOT WORTHY +20", 150, 149}},
    TextCase{"type_8", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 8}, world::NoticeText{"I'M NOT WORTHY +20", 150, 149}},
    TextCase{"type_9", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 9}, world::NoticeText{"I'M NOT WORTHY +20", 150, 149}},
    TextCase{"type_10", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 10}, world::NoticeText{"I'M NOT WORTHY +20", 150, 149}},
    TextCase{"type_11", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 11}, world::NoticeText{"I'M NOT WORTHY +20", 150, 149}},
    TextCase{"type_12", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 12}, world::NoticeText{"I'M NOT WORTHY +20", 150, 149}},
    TextCase{"type_13", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 13}, std::nullopt},
    TextCase{"type_32", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 32}, std::nullopt},
    TextCase{"type_99", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 99}, std::nullopt},
    TextCase{"type_100", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 100}, world::NoticeText{"BALLOON HERO +100", 160, 149}},
    TextCase{"type_101", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 101}, world::NoticeText{"SPIDER HERO +150", 170, 149}},
    TextCase{"type_102", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 102}, world::NoticeText{"ZEPPELIN KILLER", 170, 149}},
    TextCase{"type_103", world::Notice{true, true, binary32(0x3e800000U), binary32(0x4315e000U), 103}, std::nullopt},
    TextCase{"hidden_recognized", world::Notice{true, false, 0.0F, 160.0F, 3}, std::nullopt},
    TextCase{"expiry_visible", world::Notice{false, true, binary32(0x40480000U), binary32(0x420e0000U), 101}, world::NoticeText{"SPIDER HERO +150", 170, 35}},
    TextCase{"negative_fraction", world::Notice{true, true, binary32(0x3e800000U), binary32(0xbf400000U), 6}, world::NoticeText{"I'M NOT WORTHY +20", 150, 0}},
    TextCase{"negative_whole", world::Notice{true, true, binary32(0x3e800000U), binary32(0xc2238000U), 101}, world::NoticeText{"SPIDER HERO +150", 170, -40}}}};

const std::array<PoolSequence, 1> pool_sequences_cases = {
    {PoolSequence{{{world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                      world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                      world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}},
        {PoolStep{"allocate", 101, 180.0F, true, 0.0F, 0.0F,
             {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                 world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                 world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}}},
            PoolStep{"allocate", 3, 160.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{true, false, 0.0F, 160.0F, 3}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}}},
            PoolStep{"allocate", 4, 160.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{true, false, 0.0F, 160.0F, 3}, world::Notice{true, false, 0.0F, 160.0F, 4},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}}},
            PoolStep{"allocate", 5, 160.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{true, false, 0.0F, 160.0F, 3}, world::Notice{true, false, 0.0F, 160.0F, 4},
                    world::Notice{true, false, 0.0F, 160.0F, 5}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}}},
            PoolStep{"allocate", 6, 160.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{true, false, 0.0F, 160.0F, 3}, world::Notice{true, false, 0.0F, 160.0F, 4},
                    world::Notice{true, false, 0.0F, 160.0F, 5}, world::Notice{true, false, 0.0F, 160.0F, 6}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}}},
            PoolStep{"allocate", 11, 160.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{true, false, 0.0F, 160.0F, 3}, world::Notice{true, false, 0.0F, 160.0F, 4},
                    world::Notice{true, false, 0.0F, 160.0F, 5}, world::Notice{true, false, 0.0F, 160.0F, 6}, world::Notice{true, false, 0.0F, 160.0F, 11},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}}},
            PoolStep{"allocate", 12, 160.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{true, false, 0.0F, 160.0F, 3}, world::Notice{true, false, 0.0F, 160.0F, 4},
                    world::Notice{true, false, 0.0F, 160.0F, 5}, world::Notice{true, false, 0.0F, 160.0F, 6}, world::Notice{true, false, 0.0F, 160.0F, 11},
                    world::Notice{true, false, 0.0F, 160.0F, 12}, world::Notice{false, false, 0.0F, 0.0F, 0}}}},
            PoolStep{"allocate", 13, 160.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{true, false, 0.0F, 160.0F, 3}, world::Notice{true, false, 0.0F, 160.0F, 4},
                    world::Notice{true, false, 0.0F, 160.0F, 5}, world::Notice{true, false, 0.0F, 160.0F, 6}, world::Notice{true, false, 0.0F, 160.0F, 11},
                    world::Notice{true, false, 0.0F, 160.0F, 12}, world::Notice{true, false, 0.0F, 160.0F, 13}}}},
            PoolStep{"allocate", 32, 160.0F, false, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{true, false, 0.0F, 160.0F, 3}, world::Notice{true, false, 0.0F, 160.0F, 4},
                    world::Notice{true, false, 0.0F, 160.0F, 5}, world::Notice{true, false, 0.0F, 160.0F, 6}, world::Notice{true, false, 0.0F, 160.0F, 11},
                    world::Notice{true, false, 0.0F, 160.0F, 12}, world::Notice{true, false, 0.0F, 160.0F, 13}}}},
            PoolStep{"update", 0, 0.0F, false, 3.0F, 0.0F,
                {{world::Notice{true, true, 3.0F, 60.0F, 101}, world::Notice{true, true, 3.0F, 40.0F, 3}, world::Notice{true, true, 3.0F, 40.0F, 4},
                    world::Notice{true, true, 3.0F, 40.0F, 5}, world::Notice{true, true, 3.0F, 40.0F, 6}, world::Notice{true, true, 3.0F, 40.0F, 11},
                    world::Notice{true, true, 3.0F, 40.0F, 12}, world::Notice{true, true, 3.0F, 40.0F, 13}}}},
            PoolStep{"allocate", 3, 160.0F, false, 0.0F, 0.0F,
                {{world::Notice{true, true, 3.0F, 60.0F, 101}, world::Notice{true, true, 3.0F, 40.0F, 3}, world::Notice{true, true, 3.0F, 40.0F, 4},
                    world::Notice{true, true, 3.0F, 40.0F, 5}, world::Notice{true, true, 3.0F, 40.0F, 6}, world::Notice{true, true, 3.0F, 40.0F, 11},
                    world::Notice{true, true, 3.0F, 40.0F, 12}, world::Notice{true, true, 3.0F, 40.0F, 13}}}},
            PoolStep{"update", 0, 0.0F, false, 0.125F, 0.0F,
                {{world::Notice{false, true, binary32(0x40480000U), 55.0F, 101}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 3},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 4}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 5},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 6}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 11},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 12}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 13}}}},
            PoolStep{"allocate", 32, 160.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 160.0F, 32}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 3},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 4}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 5},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 6}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 11},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 12}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 13}}}},
            PoolStep{"allocate", 101, 180.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 160.0F, 32}, world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 4},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 5}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 6},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 11}, world::Notice{false, true, binary32(0x40480000U), 35.0F, 12},
                    world::Notice{false, true, binary32(0x40480000U), 35.0F, 13}}}},
            PoolStep{"update", 0, 0.0F, false, 0.125F, 0.28125F,
                {{world::Notice{true, false, binary32(0x3e000000U), 155.0F, 32}, world::Notice{true, false, binary32(0x3e000000U), 175.0F, 101},
                    world::Notice{false, false, binary32(0x40480000U), 35.0F, 4}, world::Notice{false, false, binary32(0x40480000U), 35.0F, 5},
                    world::Notice{false, false, binary32(0x40480000U), 35.0F, 6}, world::Notice{false, false, binary32(0x40480000U), 35.0F, 11},
                    world::Notice{false, false, binary32(0x40480000U), 35.0F, 12}, world::Notice{false, false, binary32(0x40480000U), 35.0F, 13}}}},
            PoolStep{"reset", 0, 0.0F, false, 0.0F, 0.0F,
                {{world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}}},
            PoolStep{"allocate", 101, 180.0F, true, 0.0F, 0.0F,
                {{world::Notice{true, false, 0.0F, 180.0F, 101}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0},
                    world::Notice{false, false, 0.0F, 0.0F, 0}, world::Notice{false, false, 0.0F, 0.0F, 0}}}}}}}};

void check_text(const world::Notice& value, const std::optional<world::NoticeText>& expected, std::string_view id) {
    const auto actual = world::notice_text(value);
    require(actual.has_value() == expected.has_value(), std::string(id) + ": notice text visibility or unsupported type differs");
    if (actual) {
        require(
            actual->text == expected->text && actual->x == expected->x && actual->y == expected->y, std::string(id) + ": literal notice string, fixed x or truncated y differs");
    }
}

void original_updates_and_text() {
    for (const auto& expected : updates_cases) {
        auto value = expected.before;
        world::update_notice(value, expected.delta, expected.application_time);
        const auto& after = expected.after;
        require(value == after && std::bit_cast<std::uint32_t>(value.age) == std::bit_cast<std::uint32_t>(after.age)
                    && std::bit_cast<std::uint32_t>(value.y) == std::bit_cast<std::uint32_t>(after.y),
            expected.id + ": original notice age, expiry, motion or clock blink differs");
        check_text(value, expected.text, expected.id);
    }
    for (const auto& expected : texts_cases) {
        check_text(expected.state, expected.text, expected.id);
    }
}

void original_shared_pool() {
    for (const auto& sequence : pool_sequences_cases) {
        world::NoticePool pool;
        for (std::size_t slot = 0; slot < pool.size(); ++slot) {
            pool[slot] = sequence.initial.at(slot);
        }
        for (const auto& step : sequence.steps) {
            const auto action = step.action;
            if (action == "allocate") {
                const auto before = pool;
                const bool actual = world::allocate_notice(pool, step.type, step.y);
                require(actual == step.success, "Original shared notice allocation success differs");
                if (!actual) {
                    require(pool == before, "Full notice pool must preserve every existing entry");
                }
            } else if (action == "update") {
                for (auto& value : pool) {
                    world::update_notice(value, step.delta, step.application_time);
                }
            } else if (action == "reset") {
                pool = {};
            } else {
                throw std::runtime_error("Unknown original notice pool action");
            }
            for (std::size_t slot = 0; slot < pool.size(); ++slot) {
                const auto expected = step.after.at(slot);
                require(pool[slot] == expected && std::bit_cast<std::uint32_t>(pool[slot].age) == std::bit_cast<std::uint32_t>(expected.age)
                            && std::bit_cast<std::uint32_t>(pool[slot].y) == std::bit_cast<std::uint32_t>(expected.y),
                    "Original shared-pool first vacancy, capacity, expiry, reuse or new-round reset differs");
            }
        }
    }
}

void rejected_inputs() {
    world::Notice value{true, true, 0, 160, 3};
    const auto reject_update = [&](float delta, float sample) {
        const auto before = value;
        bool rejected = false;
        try {
            world::update_notice(value, delta, sample);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && value == before, "Invalid notice update must fail before changing fields");
    };
    reject_update(-1, 0);
    reject_update(0, -1);
    reject_update(std::numeric_limits<float>::infinity(), 0);
    reject_update(0, std::numeric_limits<float>::infinity());
    reject_update(0, std::numeric_limits<float>::max());
    reject_update(std::numeric_limits<float>::max(), 0);
    value.age = -1;
    reject_update(0, 0);
    value = {true, true, 0, 160, 3};
    world::NoticePool pool{};
    const auto before = pool;
    bool rejected = false;
    try {
        static_cast<void>(world::allocate_notice(pool, 3, std::numeric_limits<float>::infinity()));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && pool == before, "Nonfinite notice allocation must preserve the shared pool");
    for (const float y : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()}) {
        value.y = y;
        rejected = false;
        try {
            static_cast<void>(world::notice_text(value));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Visible notice must reject nonfinite or out-of-range integer conversion");
    }
}

} // namespace

int main() {
    try {
        original_updates_and_text();
        original_shared_pool();
        rejected_inputs();
        std::cout << "Original generic notice lifecycle, shared pool, fixed text, expiry drawing and reset passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
