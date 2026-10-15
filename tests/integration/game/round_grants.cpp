#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "game/round.hpp"
#include "support.hpp"
#include "support/numeric.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::require;

using moorhuhn2::test::binary32;

struct LeafSlot {
    std::size_t slot;
    world::TreeFragment leaf;
};

struct SourceBoundary {
    game::test::SourceRandom random;
    std::uint64_t particle_draws;
    std::vector<std::size_t> balloon_slots;
    std::optional<std::uint32_t> condition_word;
    std::vector<std::uint32_t> initializer_words;
};

struct LeafUpdate {
    std::uint32_t delta_bits;
    game::test::SourceRandom random;
    world::TreeFragment leaf;
};

struct LeafCase {
    std::string id;
    std::uint32_t seed;
    int leaves_hit;
    int branches_hit;
    std::size_t scene_capacity;
    std::vector<std::size_t> free_slots;
    std::uint32_t camera_bits;
    int pointer_x;
    std::uint32_t delta_bits;
    bool exit_requested;
    bool cancel;
    bool earlier_stones;
    SourceBoundary boundary;
    std::optional<LeafSlot> granted;
    std::vector<LeafUpdate> followup;
};

struct RecurringPhase {
    int leaves_hit;
    int branches_hit;
    SourceBoundary boundary;
    std::vector<LeafSlot> leaves;
};

struct RecurrenceCase {
    std::uint32_t seed;
    std::uint32_t camera_bits;
    int pointer_x;
    std::vector<std::size_t> free_slots;
    std::vector<RecurringPhase> phases;
};

struct HitCounters {
    game::test::SourceRandom random;
    int leaves_hit;
    int score;
    int hits;
    int misses;
    int ammo_index;
    int recoil;
};

struct InputCase {
    std::uint32_t seed;
    std::uint32_t camera_bits;
    std::size_t existing_slot;
    world::TreeFragment existing_leaf;
    game::test::SourceRandom first_advance;
    HitCounters after_hit;
    int reaction_type;
    SourceBoundary next_boundary;
    world::TreeFragment next_leaf;
    HitCounters after_miss;
};

struct SpiderCase {
    std::string id;
    std::uint32_t seed;
    int score;
    int spider_hits;
    bool latched;
    int leaves_hit;
    bool multi_hit;
    bool full_notices;
    game::test::SourceRandom random;
    int expected_score;
    bool expected_latch;
    bool hero;
    std::vector<int> notice_types;
};

const std::array<LeafCase, 19> leaf_examples = {{LeafCase{"below_threshold9", 549U, 9, 0, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
                                                     SourceBoundary{game::test::SourceRandom{919734132U, 33ULL}, 33ULL, {}, std::nullopt, {}}, std::nullopt, {}},
    LeafCase{"negative_leaf_counter", 549U, -1, 0, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{919734132U, 33ULL}, 33ULL, {}, std::nullopt, {}}, std::nullopt, {}},
    LeafCase{"minimum_signed_leaf_counter", 549U, -2147483648, 0, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{919734132U, 33ULL}, 33ULL, {}, std::nullopt, {}}, std::nullopt, {}},
    LeafCase{"threshold10_residue7_depth2", 549U, 10, 0, 128U, {34U, 80U, 95U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{572791469U, 40ULL}, 33ULL, {}, 15367U, {18624U, 15031U, 5337U, 19729U, 3199U, 8740U}},
        LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44b58667U), -147.0F, -1024.0F, -1024.0F, 29.0F, 55.0F, 0.0F, 4.0F, 19, 7, 1}},
        {LeafUpdate{0x00000000U, game::test::SourceRandom{1838936141U, 72ULL},
             world::TreeFragment{27, 2, binary32(0x44b58667U), -147.0F, binary32(0x43d6199cU), -147.0F, 29.0F, 55.0F, 0.0F, 4.0F, 19, 0, 1}},
            LeafUpdate{0x3e000000U, game::test::SourceRandom{2518585596U, 105ULL},
                world::TreeFragment{
                    27, 2, binary32(0x44b5fa67U), binary32(0xc30c2000U), binary32(0x43d7e99cU), binary32(0xc30c2000U), 29.0F, 55.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}}}},
    LeafCase{"above_threshold11_depth3", 638U, 11, 0, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{3295432415U, 39ULL}, 32ULL, {}, 8711U, {10607U, 23738U, 28230U, 24459U, 24940U, 17516U}},
        LeafSlot{34U, world::TreeFragment{27, 3, binary32(0x449de599U), -90.0F, -1024.0F, -1024.0F, 45.0F, 46.0F, 0.0F, 4.0F, 19, 6, 1}},
        {LeafUpdate{0x00000000U, game::test::SourceRandom{719018367U, 71ULL},
             world::TreeFragment{27, 3, binary32(0x449de599U), -90.0F, binary32(0x43b79664U), -90.0F, 45.0F, 46.0F, 0.0F, 4.0F, 19, 0, 1}},
            LeafUpdate{0x3e000000U, game::test::SourceRandom{1823634463U, 103ULL},
                world::TreeFragment{
                    27, 3, binary32(0x449e9999U), binary32(0xc2a88000U), binary32(0x43ba6664U), binary32(0xc2a88000U), 45.0F, 46.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}}}},
    LeafCase{"maximum_signed_leaf_counter", 549U, 2147483647, 0, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{572791469U, 40ULL}, 33ULL, {}, 15367U, {18624U, 15031U, 5337U, 19729U, 3199U, 8740U}},
        LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44b58667U), -147.0F, -1024.0F, -1024.0F, 29.0F, 55.0F, 0.0F, 4.0F, 19, 7, 1}},
        {LeafUpdate{0x00000000U, game::test::SourceRandom{1838936141U, 72ULL},
             world::TreeFragment{27, 2, binary32(0x44b58667U), -147.0F, binary32(0x43d6199cU), -147.0F, 29.0F, 55.0F, 0.0F, 4.0F, 19, 0, 1}},
            LeafUpdate{0x3e000000U, game::test::SourceRandom{2518585596U, 105ULL},
                world::TreeFragment{
                    27, 2, binary32(0x44b5fa67U), binary32(0xc30c2000U), binary32(0x43d7e99cU), binary32(0xc30c2000U), 29.0F, 55.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}}}},
    LeafCase{"branch1_suppresses_draw", 549U, 10, 1, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{919734132U, 33ULL}, 33ULL, {}, std::nullopt, {}}, std::nullopt, {}},
    LeafCase{"negative_branch_suppresses_draw", 549U, 10, -1, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{919734132U, 33ULL}, 33ULL, {}, std::nullopt, {}}, std::nullopt, {}},
    LeafCase{"four_branches_suppress_draw", 549U, 10, 4, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{919734132U, 33ULL}, 33ULL, {}, std::nullopt, {}}, std::nullopt, {}},
    LeafCase{"residue8_free_pool", 815U, 10, 0, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{3255359830U, 33ULL}, 32ULL, {}, 16904U, {}}, std::nullopt, {}},
    LeafCase{"residue8_full_pool", 815U, 10, 0, 128U, {}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{3255359830U, 33ULL}, 32ULL, {}, 16904U, {}}, std::nullopt, {}},
    LeafCase{"residue7_full_pool", 549U, 10, 0, 128U, {}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{1007114343U, 34ULL}, 33ULL, {}, 15367U, {}}, std::nullopt, {}},
    LeafCase{"excluded_slot128_is_not_free", 549U, 10, 0, 128U, {128U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{1007114343U, 34ULL}, 33ULL, {}, 15367U, {}}, std::nullopt, {}},
    LeafCase{"capacity256_uses_first_hole200", 638U, 10, 0, 256U, {200U, 220U}, 0x44200800U, 321, 0x00000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{3295432415U, 39ULL}, 32ULL, {}, 8711U, {10607U, 23738U, 28230U, 24459U, 24940U, 17516U}},
        LeafSlot{200U, world::TreeFragment{27, 3, binary32(0x449de599U), -90.0F, -1024.0F, -1024.0F, 45.0F, 46.0F, 0.0F, 4.0F, 19, 6, 1}},
        {LeafUpdate{0x00000000U, game::test::SourceRandom{719018367U, 71ULL},
             world::TreeFragment{27, 3, binary32(0x449de599U), -90.0F, binary32(0x43b79664U), -90.0F, 45.0F, 46.0F, 0.0F, 4.0F, 19, 0, 1}},
            LeafUpdate{0x3e000000U, game::test::SourceRandom{1823634463U, 103ULL},
                world::TreeFragment{
                    27, 3, binary32(0x449e9999U), binary32(0xc2a88000U), binary32(0x43ba6664U), binary32(0xc2a88000U), 45.0F, 46.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}}}},
    LeafCase{"exit_already_requested", 549U, 10, 0, 128U, {34U}, 0x44200800U, 321, 0x00000000U, true, false, false,
        SourceBoundary{game::test::SourceRandom{572791469U, 40ULL}, 33ULL, {}, 15367U, {18624U, 15031U, 5337U, 19729U, 3199U, 8740U}},
        LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44b58667U), -147.0F, -1024.0F, -1024.0F, 29.0F, 55.0F, 0.0F, 4.0F, 19, 7, 1}},
        {LeafUpdate{0x00000000U, game::test::SourceRandom{1838936141U, 72ULL},
             world::TreeFragment{27, 2, binary32(0x44b58667U), -147.0F, binary32(0x43d6199cU), -147.0F, 29.0F, 55.0F, 0.0F, 4.0F, 19, 0, 1}},
            LeafUpdate{0x3e000000U, game::test::SourceRandom{2518585596U, 105ULL},
                world::TreeFragment{
                    27, 2, binary32(0x44b5fa67U), binary32(0xc30c2000U), binary32(0x43d7e99cU), binary32(0xc30c2000U), 29.0F, 55.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}}}},
    LeafCase{"cancel_still_completes_grant", 638U, 10, 0, 128U, {34U}, 0x44200800U, 321, 0x00000000U, false, true, false,
        SourceBoundary{game::test::SourceRandom{3295432415U, 39ULL}, 32ULL, {}, 8711U, {10607U, 23738U, 28230U, 24459U, 24940U, 17516U}},
        LeafSlot{34U, world::TreeFragment{27, 3, binary32(0x449de599U), -90.0F, -1024.0F, -1024.0F, 45.0F, 46.0F, 0.0F, 4.0F, 19, 6, 1}},
        {LeafUpdate{0x00000000U, game::test::SourceRandom{719018367U, 71ULL},
             world::TreeFragment{27, 3, binary32(0x449de599U), -90.0F, binary32(0x43b79664U), -90.0F, 45.0F, 46.0F, 0.0F, 4.0F, 19, 0, 1}},
            LeafUpdate{0x3e000000U, game::test::SourceRandom{1823634463U, 103ULL},
                world::TreeFragment{
                    27, 3, binary32(0x449e9999U), binary32(0xc2a88000U), binary32(0x43ba6664U), binary32(0xc2a88000U), 45.0F, 46.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}}}},
    LeafCase{"nonzero_delta_birth_stays_unupdated", 549U, 10, 0, 128U, {34U}, 0x44200800U, 321, 0x3e000000U, false, false, false,
        SourceBoundary{game::test::SourceRandom{572791469U, 40ULL}, 33ULL, {}, 15367U, {18624U, 15031U, 5337U, 19729U, 3199U, 8740U}},
        LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44b58667U), -147.0F, -1024.0F, -1024.0F, 29.0F, 55.0F, 0.0F, 4.0F, 19, 7, 1}},
        {LeafUpdate{0x00000000U, game::test::SourceRandom{1838936141U, 72ULL},
             world::TreeFragment{27, 2, binary32(0x44b58667U), -147.0F, binary32(0x43d6199cU), -147.0F, 29.0F, 55.0F, 0.0F, 4.0F, 19, 0, 1}},
            LeafUpdate{0x3e000000U, game::test::SourceRandom{2518585596U, 105ULL},
                world::TreeFragment{
                    27, 2, binary32(0x44b5fa67U), binary32(0xc30c2000U), binary32(0x43d7e99cU), binary32(0xc30c2000U), 29.0F, 55.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}}}},
    LeafCase{"earlier_stones16_then_leaf", 17U, 10, 0, 128U, {34U, 35U, 36U, 37U, 38U, 39U, 40U, 41U, 42U, 43U, 44U, 45U, 46U, 47U, 48U, 49U, 50U}, 0x44200800U, 321, 0x00000000U,
        false, false, true,
        SourceBoundary{game::test::SourceRandom{1400554793U, 120ULL}, 33ULL, {34U, 35U, 36U, 37U, 38U, 39U, 40U, 41U, 42U, 43U, 44U, 45U, 46U, 47U, 48U, 49U}, 19463U,
            {6383U, 28511U, 5635U, 16365U, 21916U, 21370U}},
        LeafSlot{50U, world::TreeFragment{27, 3, binary32(0x44928599U), -95.0F, -1024.0F, -1024.0F, 21.0F, 50.0F, 0.0F, 4.0F, 19, 6, 1}},
        {LeafUpdate{0x00000000U, game::test::SourceRandom{982507721U, 152ULL},
             world::TreeFragment{27, 3, binary32(0x44928599U), -95.0F, binary32(0x438a1664U), -95.0F, 21.0F, 50.0F, 0.0F, 4.0F, 19, 0, 1}},
            LeafUpdate{0x3e000000U, game::test::SourceRandom{4103265672U, 185ULL},
                world::TreeFragment{
                    27, 3, binary32(0x4492d999U), binary32(0xc2b18000U), binary32(0x438b6664U), binary32(0xc2b18000U), 21.0F, 50.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}}}},
    LeafCase{"earlier_stones_fill_last_holes", 495U, 10, 0, 128U, {34U, 40U}, 0x44200800U, 321, 0x00000000U, false, false, true,
        SourceBoundary{game::test::SourceRandom{1476872968U, 43ULL}, 32ULL, {34U, 40U}, 22535U, {}}, std::nullopt, {}}}};

const RecurrenceCase expected_recurrence = RecurrenceCase{925U, 0x44200800U, 321, {34U, 37U, 80U},
    {RecurringPhase{10, 0, SourceBoundary{game::test::SourceRandom{3522018954U, 39ULL}, 32ULL, {}, 9222U, {19190U, 462U, 8573U, 24635U, 19287U, 20973U}},
         {LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44a06667U), -83.0F, -1024.0F, -1024.0F, 37.0F, 48.0F, 0.0F, 4.0F, 19, 11, 1}}}},
        RecurringPhase{10, 0, SourceBoundary{game::test::SourceRandom{13199963U, 78ULL}, 32ULL, {}, 12801U, {16470U, 31054U, 21360U, 25372U, 4685U, 201U}},
            {LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44a06667U), -83.0F, binary32(0x4381999cU), -83.0F, 37.0F, 48.0F, 0.0F, 4.0F, 19, 0, 1}},
                LeafSlot{37U, world::TreeFragment{27, 2, binary32(0x44906667U), -120.0F, -1024.0F, -1024.0F, 15.0F, 36.0F, 0.0F, 4.0F, 19, 7, 1}}}},
        RecurringPhase{10, 1, SourceBoundary{game::test::SourceRandom{3723246331U, 110ULL}, 32ULL, {}, std::nullopt, {}},
            {LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44a06667U), -83.0F, binary32(0x4381999cU), -83.0F, 37.0F, 48.0F, 0.0F, 4.0F, 19, 0, 1}},
                LeafSlot{37U, world::TreeFragment{27, 2, binary32(0x44906667U), -120.0F, binary32(0x43033338U), -120.0F, 15.0F, 36.0F, 0.0F, 4.0F, 19, 0, 1}}}},
        RecurringPhase{9, 0, SourceBoundary{game::test::SourceRandom{11632365U, 144ULL}, 34ULL, {}, std::nullopt, {}},
            {LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44a06667U), -83.0F, binary32(0x4381999cU), -83.0F, 37.0F, 48.0F, 0.0F, 4.0F, 19, 0, 1}},
                LeafSlot{37U, world::TreeFragment{27, 2, binary32(0x44906667U), -120.0F, binary32(0x43033338U), -120.0F, 15.0F, 36.0F, 0.0F, 4.0F, 19, 0, 1}}}},
        RecurringPhase{10, 0, SourceBoundary{game::test::SourceRandom{3265752943U, 178ULL}, 33ULL, {}, 17063U, {}},
            {LeafSlot{34U, world::TreeFragment{27, 2, binary32(0x44a06667U), -83.0F, binary32(0x4381999cU), -83.0F, 37.0F, 48.0F, 0.0F, 4.0F, 19, 0, 1}},
                LeafSlot{37U, world::TreeFragment{27, 2, binary32(0x44906667U), -120.0F, binary32(0x43033338U), -120.0F, 15.0F, 36.0F, 0.0F, 4.0F, 19, 0, 1}}}}}};

const InputCase expected_input_boundary = InputCase{402U, 0x44200800U, 34U, world::TreeFragment{27, 2, 1324.0F, 200.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 4.0F, 19, 0, 1},
    game::test::SourceRandom{2211396495U, 35ULL}, HitCounters{game::test::SourceRandom{2211396495U, 35ULL}, 10, 74, 8, 3, 12, 8}, 67,
    SourceBoundary{game::test::SourceRandom{509858135U, 75ULL}, 33ULL, {}, 27143U, {22866U, 6375U, 11192U, 20681U, 4732U, 7779U}},
    world::TreeFragment{27, 2, binary32(0x44b38667U), -152.0F, -1024.0F, -1024.0F, 62.0F, 54.0F, 0.0F, 4.0F, 19, 9, 1},
    HitCounters{game::test::SourceRandom{509858135U, 75ULL}, 10, 74, 8, 4, 13, 8}};

const std::array<SpiderCase, 13> spider_examples = {
    {SpiderCase{"hero_zero", 1U, 0, 26, false, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 100, true, true, {101}},
        SpiderCase{"hero_negative50_before_clamp", 1U, -50, 26, false, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 50, true, true, {101}},
        SpiderCase{"hero_negative101_before_clamp", 1U, -101, 26, false, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 0, true, true, {101}},
        SpiderCase{"hero_maximum_nonwrapping", 1U, 2147483547, 26, false, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 2147483647, true, true, {101}},
        SpiderCase{"hero_first_wrapping_to_int_min", 549U, 2147483548, 26, false, 10, false, false, game::test::SourceRandom{1007114343U, 34ULL}, 0, true, true, {101}},
        SpiderCase{"hero_from_int_max", 1U, 2147483647, 26, false, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 0, true, true, {101}},
        SpiderCase{"hero_from_int_min", 1U, -2147483648, 26, false, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 0, true, true, {101}},
        SpiderCase{"hero_wrapping_then_multi_hit", 1U, 2147483548, 26, false, 0, true, false, game::test::SourceRandom{1423890337U, 32ULL}, 0, true, true, {101, 3}},
        SpiderCase{"hero_then_multi_hit_notice_order", 1U, 37, 26, false, 0, true, false, game::test::SourceRandom{1423890337U, 32ULL}, 142, true, true, {101, 3}},
        SpiderCase{"hero_full_notice_pool", 1U, 2147483548, 26, false, 0, false, true, game::test::SourceRandom{1423890337U, 32ULL}, 0, true, true, {}},
        SpiderCase{"hero_latched_does_not_repeat", 1U, 2147483548, 26, true, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 2147483548, true, false, {}},
        SpiderCase{"hero_below26", 1U, 2147483548, 25, false, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 2147483548, false, false, {}},
        SpiderCase{"hero_above26", 1U, 2147483548, 27, false, 0, false, false, game::test::SourceRandom{1423890337U, 32ULL}, 2147483548, false, false, {}}}};

game::RoundState& fixture(game::Round& round) {
    auto& state = const_cast<game::RoundState&>(round.state());
    // Retain the original 32 permanent leaves and normal update traversal; make
    // every other RNG consumer inert and explicitly control the shared scene holes.
    state.frog.type = -1;
    state.hidden_chicken.type = -1;
    state.spider.type = -1;
    state.spawn_accumulator = -10000;
    state.camera = 640.125F;
    state.remaining = 73;
    state.random_state = 1;
    state.random_draws = 0;
    state.score = 73;
    state.hits = 7;
    state.misses = 3;
    state.ammo_index = 11;
    state.reload_start_index = 6;
    state.special_ammo = true;
    state.branch_ammo_granted = true;
    state.stone_bonus_granted = true;
    state.balloon_hero = true;
    state.spider_hero = true;
    state.scarecrow_complete = true;
    state.hidden_chicken_enabled = true;
    for (auto& chicken : state.chickens) {
        chicken.type = -2;
    }
    // Filling transient and notice pools also proves this scene request has no
    // prerequisite or fallback involving either pool.
    for (auto& particle : state.particles) {
        particle = {.active = true, .type = 2, .depth = 2, .world_x = 100, .world_y = 100, .screen_x = -924, .screen_y = 100, .points = 1};
    }
    for (auto& notice : state.notices) {
        notice = {true, true, 0, 200, 3};
    }
    return state;
}

void holes(game::RoundState& state, std::span<const std::size_t> values) {
    for (const auto& value : values) {
        state.chickens[value].type = -1;
    }
}

void advance(game::Round& round, float delta = 0, game::RoundInput input = {}) {
    game::test::advance(round, round.state().previous_sample + delta, input);
}

void tick(game::Round& round, float delta = 0, game::RoundInput input = {}) {
    advance(round, delta, input);
    round.resolve_actions({});
}

void random_state(const game::RoundState& state, const game::test::SourceRandom& expected, std::string_view id) {
    require(state.random_state == expected.state && state.random_draws == expected.draws, std::string(id) + ": literal ordered shared CRT state");
}

void fragment(const world::TreeFragment& actual, const world::TreeFragment& expected, std::string_view id) {
    require(actual.type == expected.type && actual.depth == expected.depth && actual.frame_count == expected.frame_count && actual.frame == expected.frame
                && actual.points == expected.points,
        std::string(id) + ": type27 integer fields follow the source initializer/update");
    constexpr std::array fields{std::pair{"world_x", &world::TreeFragment::world_x}, std::pair{"world_y", &world::TreeFragment::world_y},
        std::pair{"screen_x", &world::TreeFragment::screen_x}, std::pair{"screen_y", &world::TreeFragment::screen_y}, std::pair{"velocity_x", &world::TreeFragment::velocity_x},
        std::pair{"velocity_y", &world::TreeFragment::velocity_y}, std::pair{"animation_time", &world::TreeFragment::animation_time},
        std::pair{"animation_rate", &world::TreeFragment::animation_rate}};
    for (const auto& [name, member] : fields) {
        require(std::bit_cast<std::uint32_t>(actual.*member) == std::bit_cast<std::uint32_t>(expected.*member), std::string(id) + ": literal binary32 leaf field " + name);
    }
}

void source_boundary(const game::RoundState& before, const SourceBoundary& expected, std::string_view id) {
    auto random = game::test::snapshot(before);
    const auto draws = random.draws;
    random.leaves();
    require(random.draws - draws == expected.particle_draws, std::string(id) + ": all 32 conditional particle draws precede grants");
    random.discard(5 * expected.balloon_slots.size());
    if (expected.condition_word.has_value()) {
        require(random.next() == *expected.condition_word, std::string(id) + ": grant condition uses the next original CRT word");
    }
    for (const auto& value : expected.initializer_words) {
        require(random.next() == value, std::string(id) + ": each successful request consumes its six initializer words in order");
    }
    require(random.state == expected.random.state && random.draws == expected.random.draws, std::string(id) + ": source fixture and independent scalar stream agree");
}

void leaf_cases(std::span<const LeafCase> rows) {
    for (const auto& row : rows) {
        const auto id = row.id;
        const auto& expected = row.boundary;
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.random_state = row.seed;
        state.leaves_hit = row.leaves_hit;
        state.branches_hit = row.branches_hit;
        state.scene_capacity = row.scene_capacity;
        state.camera = binary32(row.camera_bits);
        state.exit_requested = row.exit_requested;
        if (row.earlier_stones) {
            state.stones_hit = 8;
            state.stone_bonus_granted = false;
        }
        holes(state, row.free_slots);
        const auto before = state;
        const float delta = binary32(row.delta_bits);
        source_boundary(before, expected, id);
        advance(round, delta, {.pointer = {row.pointer_x, 240}, .cancel = row.cancel});
        random_state(state, expected.random, id);
        const std::size_t leaf_count = row.granted ? 1 : 0;
        require(std::ranges::count(state.tree_fragments, 27, &world::TreeFragment::type) == static_cast<std::ptrdiff_t>(leaf_count),
            id + ": the signed predicates and residue boundary request at most one leaf");
        if (leaf_count != 0) {
            const auto slot = row.granted->slot;
            require(state.chickens[slot].type == -4, id + ": leaf reserves the first free active scene slot");
            fragment(state.tree_fragments[slot], row.granted->leaf, id);
            const auto draws = round.target_draws(state.tree_fragments[slot].depth);
            const auto draw = std::ranges::find(draws, static_cast<int>(slot), &contracts::DepthDraw::scene_slot);
            require(draw != draws.end() && draw->sprite.image == "gfx/Leaf" && draw->sprite.destination.x == -1024 && draw->sprite.destination.y == -1024,
                id + ": current composition retains the initializer sentinel, including a nonzero birth delta");
        }
        const auto& balloons = expected.balloon_slots;
        require(std::ranges::count(state.balloons, 21, &world::Balloon::type) == static_cast<std::ptrdiff_t>(balloons.size()),
            id + ": earlier stone requests take their first free holes before this request");
        for (const auto& value : balloons) {
            const auto slot = value;
            require(state.chickens[slot].type == -3 && state.balloons[slot].screen_x == -1024 && state.balloons[slot].screen_y == -1024,
                id + ": earlier grant occupies the hole without a premature scene update");
        }
        const auto reserved = [&](std::size_t slot) {
            return (row.granted && row.granted->slot == slot) || std::ranges::any_of(balloons, [slot](const auto& value) {
                return value == slot;
            });
        };
        for (std::size_t slot = 0; slot < state.chickens.size(); ++slot) {
            if (!reserved(slot)) {
                require(state.chickens[slot].type == before.chickens[slot].type, id + ": failed scans and unused holes preserve their scene markers");
            }
        }
        require(state.score == before.score && state.leaves_hit == before.leaves_hit && state.branches_hit == before.branches_hit && state.hits == before.hits
                    && state.misses == before.misses && state.ammo == before.ammo && state.ammo_index == before.ammo_index && state.reload_start_index == before.reload_start_index
                    && state.special_ammo == before.special_ammo && state.remaining == 73.0F - delta && state.balloon_hero == before.balloon_hero
                    && state.spider_hero == before.spider_hero && state.hidden_chicken_enabled == before.hidden_chicken_enabled,
            id + ": recurring request has no score, hit, ammunition, time or other-latch side effects");
        require(
            state.multi_hit.timers == before.multi_hit.timers && state.multi_hit.write_index == before.multi_hit.write_index, id + ": later tracker preserves its pending fields");
        require(state.exit_requested == (row.exit_requested || row.cancel), id + ": exit does not abort the grant dispatch");
        for (const auto& particle : state.particles) {
            require(
                particle.active && particle.type == 2 && particle.points == 1 && particle.age == delta, id + ": occupied transient particles only receive their ordinary update");
        }
        require(std::ranges::all_of(state.notices,
                    [delta](const auto& notice) {
                        return notice.active && notice.type == 3 && notice.age == delta;
                    }),
            id + ": recurring request does not allocate or replace a notice");
        std::size_t event_index = 0;
        if (row.cancel) {
            require(round.events().size() > event_index && round.events()[event_index++].kind == game::RoundEventKind::game_over, id + ": cancellation event precedes grants");
        }
        if (row.earlier_stones) {
            require(round.events().size() > event_index && round.events()[event_index++].kind == game::RoundEventKind::special_ammo, id + ": only earlier stone grant emits magic");
        }
        require(round.events().size() == event_index, id + ": recurring leaf has no spawn, score, impact or sound event");
        round.resolve_actions({});
        random_state(state, expected.random, id);
        require(round.events().size() == event_index, id + ": no-input resolution preserves grant state and events");
        if (leaf_count != 0) {
            const auto slot = row.granted->slot;
            state.leaves_hit = 9;
            for (const auto& next : row.followup) {
                tick(round, binary32(next.delta_bits), {.pointer = {row.pointer_x, 240}});
                random_state(state, next.random, id);
                fragment(state.tree_fragments[slot], next.leaf, id);
                require(round.events().empty(), id + ": newborn's next scene update consumes no initializer or sound draws");
            }
        }
    }
}

void recurring(const RecurrenceCase& row) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.random_state = row.seed;
    state.camera = binary32(row.camera_bits);
    holes(state, row.free_slots);
    std::size_t phase_index = 0;
    for (const auto& phase : row.phases) {
        const auto id = "recurrence phase " + std::to_string(phase_index++);
        state.leaves_hit = phase.leaves_hit;
        state.branches_hit = phase.branches_hit;
        const auto& expected = phase.boundary;
        source_boundary(state, expected, id);
        tick(round, 0, {.pointer = {row.pointer_x, 240}});
        random_state(state, expected.random, id);
        for (const auto& value : phase.leaves) {
            fragment(state.tree_fragments[value.slot], value.leaf, id);
        }
        require(std::ranges::count(state.tree_fragments, 27, &world::TreeFragment::type) == static_cast<std::ptrdiff_t>(phase.leaves.size()) && state.score == 73
                    && round.events().empty(),
            id + ": success never latches; branch and threshold changes only gate that call");
    }
    game::Round fresh(1, 0);
    require(fresh.state().leaves_hit == 0 && fresh.state().branches_hit == 0
                && std::ranges::none_of(fresh.state().tree_fragments,
                    [](const auto& value) {
                        return value.type >= 0;
                    }),
        "Fresh round resets counters and fragments; recurring grant requires no additional latch reset");
    auto& fresh_state = fixture(fresh);
    const auto random = game::test::after_leaves(game::test::snapshot(fresh_state));
    tick(fresh);
    require(fresh_state.random_state == random.state && fresh_state.random_draws == random.draws && fresh.events().empty(),
        "Fresh below-threshold round consumes only the accepted particle stream");
}

void input_boundary(const InputCase& row) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.random_state = row.seed;
    state.camera = binary32(row.camera_bits);
    state.leaves_hit = 9;
    const auto existing_slot = row.existing_slot;
    const auto& initial = row.existing_leaf;
    state.chickens[existing_slot].type = -4;
    state.chickens[37].type = -1;
    state.tree_fragments[existing_slot] = initial;
    advance(round, 0, {.pointer = {300, 200}, .fire_down = true});
    random_state(state, row.first_advance, "input boundary before tenth hit");
    require(state.leaves_hit == 9 && state.chickens[37].type == -1 && round.events().empty(), "Pre-input grant dispatch cannot use the current shot's tenth leaf hit");
    // Accepted tree tests cover actual source pixels. This scoped matcher permits
    // the already bounded leaf candidate while retaining normal scene selection,
    // impact, counter, ammunition and input ordering.
    const auto leaf_pixel = [](const contracts::SpriteDraw& draw, contracts::ScreenPoint) {
        return draw.image == "gfx/Leaf";
    };
    round.resolve_actions(leaf_pixel);
    const auto counters = [&](const HitCounters& expected, std::string_view id) {
        random_state(state, expected.random, id);
        require(state.leaves_hit == expected.leaves_hit && state.score == expected.score && state.hits == expected.hits && state.misses == expected.misses
                    && state.ammo_index == expected.ammo_index && state.recoil == expected.recoil,
            std::string(id) + ": original-derived input and score counters");
    };
    counters(row.after_hit, "tenth hit resolution");
    require(state.selected_slot == static_cast<int>(existing_slot) && state.tree_fragments[existing_slot].type == row.reaction_type && state.chickens[37].type == -1
                && round.events().size() == 2 && round.events()[0].kind == game::RoundEventKind::shot && round.events()[1].kind == game::RoundEventKind::leaf_hit,
        "Tenth hit becomes reaction67 and emits only gunshot/leaf hit; it cannot run an extra grants pass");
    const auto& next = row.next_boundary;
    source_boundary(state, next, "update after tenth hit");
    advance(round, 0, {.pointer = {321, 240}, .fire_down = true});
    random_state(state, next.random, "update after tenth hit");
    fragment(state.tree_fragments[37], row.next_leaf, "update after tenth hit");
    require(state.selected_slot == static_cast<int>(existing_slot) && state.multi_hit.write_index == 0 && round.events().empty(),
        "Previous reaction67 is excluded by the later multiple-hit lookup while the leaf grant retains its newborn sentinel");
    round.resolve_actions(leaf_pixel);
    counters(row.after_miss, "newborn on-screen selection");
    require(state.selected_slot == -1 && state.tree_fragments[37].type == 27 && round.events().size() == 1 && round.events()[0].kind == game::RoundEventKind::shot,
        "An on-screen pointer cannot select the newborn leaf before its next scene update");
}

void spider_cases(std::span<const SpiderCase> rows) {
    for (const auto& row : rows) {
        const auto id = row.id;
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.random_state = row.seed;
        state.score = row.score;
        state.spider_hits = row.spider_hits;
        state.spider_hero = row.latched;
        state.leaves_hit = row.leaves_hit;
        if (!row.full_notices) {
            state.notices = {};
        }
        if (row.multi_hit) {
            state.multi_hit.timers[1] = 1;
            state.multi_hit.timers[2] = 1;
            state.multi_hit.write_index = 3;
        }
        const auto ammo = state.ammo;
        const auto particles = state.particles;
        tick(round);
        random_state(state, row.random, id);
        require(state.score == row.expected_score && state.spider_hero == row.expected_latch && state.spider_hits == row.spider_hits,
            id + ": source 32-bit hero addition is applied before the shared signed-negative clamp");
        require(state.hits == 7 && state.misses == 3 && state.ammo_index == 11 && state.reload_start_index == 6 && state.ammo == ammo && state.particles == particles
                    && std::ranges::none_of(state.balloons,
                        [](const auto& value) {
                            return value.type >= 0;
                        }),
            id + ": full scene does not suppress hero score/sounds and consumes no balloon initializer draws");
        std::size_t event_index = 0;
        if (row.hero) {
            require(round.events().size() >= 2 && round.events()[event_index++].kind == game::RoundEventKind::spider_balloons,
                id + ": hero emits magic first even when all 20 allocation attempts fail");
            const auto& hero = round.events()[event_index++];
            require(hero.kind == game::RoundEventKind::spider_hero && hero.points == 100, id + ": source hero points are 100 before signed score wrapping");
        }
        if (row.multi_hit && !row.full_notices) {
            require(round.events().size() > event_index && round.events()[event_index].kind == game::RoundEventKind::multi_hit_bonus && round.events()[event_index++].points == 5,
                id + ": later multiple-hit sound/addition follows the hero's wrapping addition");
        }
        require(round.events().size() == event_index, id + ": no additional award or numeric popup event");
        if (!row.full_notices) {
            const auto& notices = row.notice_types;
            for (std::size_t index = 0; index < notices.size(); ++index) {
                require(state.notices[index].active && state.notices[index].type == notices[index] && state.notices[index].age == 0
                            && state.notices[index].y == (notices[index] == 101 ? 180 : 160),
                    id + ": spider notice101 reserves the first hole before any multiple-hit notice");
            }
            require(std::ranges::count(state.notices, true, &world::Notice::active) == static_cast<std::ptrdiff_t>(notices.size()),
                id + ": failed or latched hero never allocates another notice");
        } else {
            require(std::ranges::all_of(state.notices,
                        [](const auto& value) {
                            return value.active && value.type == 3;
                        }),
                id + ": full notice pool preserves existing records while hero still wraps/clamps score");
        }
    }
}
} // namespace

int main() {
    try {
        leaf_cases(leaf_examples);
        recurring(expected_recurrence);
        input_boundary(expected_input_boundary);
        spider_cases(spider_examples);
        std::cout << "Source-derived recurring leaf grant predicates/allocation/RNG/order, newborn updates and spider hero wrap passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
