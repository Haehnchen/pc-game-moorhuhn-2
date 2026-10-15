#include "game/specials/multi_hit.hpp"

#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace moorhuhn2::game::specials;

float f(std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}

struct ExpectedState {
    std::size_t write_index{};
    std::vector<std::pair<std::size_t, std::uint32_t>> timers;
};

struct UpdateCase {
    std::string id;
    ExpectedState before;
    std::uint32_t delta_bits;
    std::optional<int> selected_type;
    bool notice_available;
    ExpectedState after;
    MultiHitEffect effect;
};

struct Sequence {
    ExpectedState initial;
    std::vector<UpdateCase> steps;
};

// Fixture generation only expands literal timer words; no game code computes expectations.
ExpectedState timer_range(
    std::size_t write_index, std::size_t first, std::size_t count, std::uint32_t bits, std::initializer_list<std::pair<std::size_t, std::uint32_t>> overrides = {}) {
    ExpectedState state{write_index, {}};
    for (std::size_t index = first; index < first + count; ++index) {
        state.timers.emplace_back(index, bits);
    }
    for (const auto& [index, word] : overrides) {
        state.timers.at(index - first).second = word;
    }
    return state;
}

const auto updates = [] {
    std::vector<UpdateCase> cases;
    std::vector<std::optional<int>> types{std::nullopt, std::numeric_limits<int>::min(), -1};
    for (int type = 0; type <= 70; ++type) {
        types.push_back(type);
    }
    types.push_back(std::numeric_limits<int>::max());
    constexpr std::array qualifying_types{40, 41, 42, 43, 44, 50, 51, 60, 61};
    for (const auto type : types) {
        const bool qualifies = type && std::ranges::find(qualifying_types, *type) != qualifying_types.end();
        cases.push_back({"qualifier-" + (type ? std::to_string(*type) : "None"), {1, {{0, 0x3f800000U}}}, 0x3e000000U, type, true,
            qualifies ? ExpectedState{2, {{0, 0x3f600000U}, {1, 0x3ff00000U}}} : ExpectedState{1, {{0, 0x3f600000U}}}, {0, std::nullopt, 0, false, false}});
    }
    constexpr std::array points{0, 0, 5, 10, 15, 20, 20, 20, 20, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (std::size_t count = 1; count <= points.size(); ++count) {
        for (const bool available : {true, false}) {
            const auto notice = available && count >= 3 ? std::optional{MultiHitNotice{static_cast<int>(count), f(0x43200000U)}} : std::nullopt;
            cases.push_back({"count-" + std::to_string(count) + "-notice-" + (available ? "True" : "False"), timer_range(count, 0, count, 0x3f800000U, {{0, 0x3e000000U}}),
                0x3e000000U, std::nullopt, available, {0, {}}, {static_cast<int>(count), notice, available ? points[count - 1] : 0, available && points[count - 1] != 0, false}});
        }
    }
    const std::array boundaries{UpdateCase{"idle-first-call-normalizes", {0, {}}, 0x00000000U, std::nullopt, true, {0, {}}, {1, std::nullopt, 0, false, false}},
        UpdateCase{"raw-new-round-reset-retains-index-until-first-call", {19, {}}, 0x00000000U, std::nullopt, true, {0, {}}, {1, std::nullopt, 0, false, false}},
        UpdateCase{"zero-first-positive-spare-tail-is-counted", {2, {{2, 0x3f800000U}, {4, 0x3f800000U}}}, 0x00000000U, std::nullopt, true, {0, {}},
            {3, MultiHitNotice{3, f(0x43200000U)}, 5, true, false}},
        UpdateCase{"zero-and-negative-tail-not-cleared", {4, {{1, 0xbf800000U}, {2, 0x80000000U}, {3, 0x3f800000U}}}, 0x00000000U, std::nullopt, true,
            {0, {{1, 0xbf800000U}, {2, 0x80000000U}}}, {2, std::nullopt, 0, false, false}},
        UpdateCase{"oldest-still-positive", {3, {{0, 0x3f800000U}, {1, 0x3fc00000U}, {2, 0x3fe00000U}}}, 0x3f7fffffU, std::nullopt, true,
            {3, {{0, 0x33800000U}, {1, 0x3f000001U}, {2, 0x3f400001U}}}, {0, std::nullopt, 0, false, false}},
        UpdateCase{"oldest-exactly-zero", {3, {{0, 0x3f800000U}, {1, 0x3fc00000U}, {2, 0x3fe00000U}}}, 0x3f800000U, std::nullopt, true, {0, {}},
            {3, MultiHitNotice{3, f(0x43200000U)}, 5, true, false}},
        UpdateCase{"all-timers-expire-in-large-delta", {3, {{0, 0x3f800000U}, {1, 0x3fc00000U}, {2, 0x3fe00000U}}}, 0x40000000U, std::nullopt, true,
            {0, {{1, 0xbf000000U}, {2, 0xbe800000U}}}, {1, std::nullopt, 0, false, false}},
        UpdateCase{"appended-event-also-ages-immediately", {0, {}}, 0x3e800000U, 50, true, {1, {{0, 0x3fe00000U}}}, {0, std::nullopt, 0, false, false}},
        UpdateCase{"appended-event-delta2-resolves1", {0, {}}, 0x40000000U, 50, true, {0, {}}, {1, std::nullopt, 0, false, false}},
        UpdateCase{"appended-event-delta-above2-resolves1", {0, {}}, 0x40000001U, 50, true, {0, {}}, {1, std::nullopt, 0, false, false}},
        UpdateCase{"appended-event-delta-below2-remains", {0, {}}, 0x3fffffffU, 50, true, {1, {{0, 0x34000000U}}}, {0, std::nullopt, 0, false, false}},
        UpdateCase{"new-hit-is-added-before-oldest-expiry", {2, {{0, 0x3f000000U}, {1, 0x3f800000U}}}, 0x3f000000U, 61, true, {0, {}},
            {3, MultiHitNotice{3, f(0x43200000U)}, 5, true, false}},
        UpdateCase{"binary32-half-ulp-store-ties-even", {1, {{0, 0x40000000U}}}, 0x33800000U, std::nullopt, true, {1, {{0, 0x40000000U}}}, {0, std::nullopt, 0, false, false}},
        UpdateCase{"binary32-next-after-half-ulp-store", {1, {{0, 0x40000000U}}}, 0x33800001U, std::nullopt, true, {1, {{0, 0x3fffffffU}}}, {0, std::nullopt, 0, false, false}},
        UpdateCase{"smallest-positive-timer-exact-expiry", {3, {{0, 0x00000001U}, {1, 0x3f800000U}, {2, 0x3f800000U}}}, 0x00000001U, std::nullopt, true, {0, {}},
            {3, MultiHitNotice{3, f(0x43200000U)}, 5, true, false}},
        UpdateCase{
            "negative-timer-is-frozen", {3, {{0, 0xbe000000U}, {2, 0xbf800000U}}}, 0x3e800000U, std::nullopt, true, {0, {{2, 0xbf800000U}}}, {1, std::nullopt, 0, false, false}},
        UpdateCase{"NATIVE.SAFETY.full32-drops33rd-and-ages", timer_range(32, 0, 32, 0x3f800000U), 0x3e800000U, 50, true, timer_range(32, 0, 32, 0x3f400000U),
            {0, std::nullopt, 0, false, true}},
        UpdateCase{"NATIVE.SAFETY.full32-recovers-at-first-expiry", timer_range(32, 0, 32, 0x3f800000U, {{0, 0x3e000000U}}), 0x3e000000U, 50, true, {0, {}},
            {32, MultiHitNotice{32, f(0x43200000U)}, 0, false, true}},
        UpdateCase{"NATIVE.SAFETY.full32-large-delta-remains-finite", timer_range(32, 0, 32, 0x3f800000U), 0x40000000U, 50, true, timer_range(0, 1, 31, 0xbf800000U),
            {1, std::nullopt, 0, false, true}},
        UpdateCase{"NATIVE.SAFETY.32nd-slot-is-in-bounds", timer_range(31, 0, 31, 0x3f800000U), 0x00000000U, 50, true, timer_range(32, 0, 32, 0x3f800000U, {{31, 0x40000000U}}),
            {0, std::nullopt, 0, false, false}}};
    cases.insert(cases.end(), boundaries.begin(), boundaries.end());
    return cases;
}();

const std::array sequences{
    Sequence{{0, {}}, {{"three-stones-with-intervening-miss-empty-reload-no-fire-1", {0, {}}, 0x3c23d70aU, 50, true, {1, {{0, 0x3ffeb852U}}}, {0, std::nullopt, 0, false, false}},
                          {"three-stones-with-intervening-miss-empty-reload-no-fire-2", {1, {{0, 0x3ffeb852U}}}, 0x3e4ccccdU, 50, true, {2, {{0, 0x3fe51eb8U}, {1, 0x3fe66666U}}},
                              {0, std::nullopt, 0, false, false}},
                          {"three-stones-with-intervening-miss-empty-reload-no-fire-3", {2, {{0, 0x3fe51eb8U}, {1, 0x3fe66666U}}}, 0x3e4ccccdU, 50, true,
                              {3, {{0, 0x3fcb851eU}, {1, 0x3fccccccU}, {2, 0x3fe66666U}}}, {0, std::nullopt, 0, false, false}},
                          {"three-stones-with-intervening-miss-empty-reload-no-fire-4", {3, {{0, 0x3fcb851eU}, {1, 0x3fccccccU}, {2, 0x3fe66666U}}}, 0x3e4ccccdU, std::nullopt,
                              true, {3, {{0, 0x3fb1eb84U}, {1, 0x3fb33332U}, {2, 0x3fccccccU}}}, {0, std::nullopt, 0, false, false}},
                          {"three-stones-with-intervening-miss-empty-reload-no-fire-5", {3, {{0, 0x3fb1eb84U}, {1, 0x3fb33332U}, {2, 0x3fccccccU}}}, 0x3e4ccccdU, std::nullopt,
                              true, {3, {{0, 0x3f9851eaU}, {1, 0x3f999998U}, {2, 0x3fb33332U}}}, {0, std::nullopt, 0, false, false}},
                          {"three-stones-with-intervening-miss-empty-reload-no-fire-6", {3, {{0, 0x3f9851eaU}, {1, 0x3f999998U}, {2, 0x3fb33332U}}}, 0x3e4ccccdU, std::nullopt,
                              true, {3, {{0, 0x3f7d70a1U}, {1, 0x3f7ffffdU}, {2, 0x3f999998U}}}, {0, std::nullopt, 0, false, false}},
                          {"three-stones-with-intervening-miss-empty-reload-no-fire-7", {3, {{0, 0x3f7d70a1U}, {1, 0x3f7ffffdU}, {2, 0x3f999998U}}}, 0x3e4ccccdU, std::nullopt,
                              true, {3, {{0, 0x3f4a3d6eU}, {1, 0x3f4ccccaU}, {2, 0x3f7ffffdU}}}, {0, std::nullopt, 0, false, false}},
                          {"three-stones-with-intervening-miss-empty-reload-no-fire-8", {3, {{0, 0x3f4a3d6eU}, {1, 0x3f4ccccaU}, {2, 0x3f7ffffdU}}}, 0x3f4ccccdU, std::nullopt,
                              true, {0, {{1, 0xb4400000U}}}, {2, std::nullopt, 0, false, false}},
                          {"three-stones-with-intervening-miss-empty-reload-no-fire-9", {0, {{1, 0xb4400000U}}}, 0x3e4ccccdU, std::nullopt, true, {0, {{1, 0xb4400000U}}},
                              {1, std::nullopt, 0, false, false}}}},
    Sequence{{0, {}}, {{"mixed-stone-frog-spider-balloon-chicken-1", {0, {}}, 0x3c23d70aU, 50, true, {1, {{0, 0x3ffeb852U}}}, {0, std::nullopt, 0, false, false}},
                          {"mixed-stone-frog-spider-balloon-chicken-2", {1, {{0, 0x3ffeb852U}}}, 0x3e4ccccdU, 51, true, {2, {{0, 0x3fe51eb8U}, {1, 0x3fe66666U}}},
                              {0, std::nullopt, 0, false, false}},
                          {"mixed-stone-frog-spider-balloon-chicken-3", {2, {{0, 0x3fe51eb8U}, {1, 0x3fe66666U}}}, 0x3e4ccccdU, 60, true,
                              {3, {{0, 0x3fcb851eU}, {1, 0x3fccccccU}, {2, 0x3fe66666U}}}, {0, std::nullopt, 0, false, false}},
                          {"mixed-stone-frog-spider-balloon-chicken-4", {3, {{0, 0x3fcb851eU}, {1, 0x3fccccccU}, {2, 0x3fe66666U}}}, 0x3e4ccccdU, 61, true,
                              {4, {{0, 0x3fb1eb84U}, {1, 0x3fb33332U}, {2, 0x3fccccccU}, {3, 0x3fe66666U}}}, {0, std::nullopt, 0, false, false}},
                          {"mixed-stone-frog-spider-balloon-chicken-5", {4, {{0, 0x3fb1eb84U}, {1, 0x3fb33332U}, {2, 0x3fccccccU}, {3, 0x3fe66666U}}}, 0x3e4ccccdU, 42, true,
                              timer_range(5, 0, 5, 0x3fb33332U, {{0, 0x3f9851eaU}, {1, 0x3f999998U}, {3, 0x3fccccccU}, {4, 0x3fe66666U}}), {0, std::nullopt, 0, false, false}},
                          {"mixed-stone-frog-spider-balloon-chicken-6", timer_range(5, 0, 5, 0x3fb33332U, {{0, 0x3f9851eaU}, {1, 0x3f999998U}, {3, 0x3fccccccU}, {4, 0x3fe66666U}}),
                              0x3fa66666U, std::nullopt, true, {0, {{1, 0xbdcccce0U}}}, {4, MultiHitNotice{4, f(0x43200000U)}, 10, true, false}}}},
    Sequence{{0, {}},
        {{"scene-removal-reuse-return-do-not-register-old-hit-1", {0, {}}, 0x3c23d70aU, 50, true, {1, {{0, 0x3ffeb852U}}}, {0, std::nullopt, 0, false, false}},
            {"scene-removal-reuse-return-do-not-register-old-hit-2", {1, {{0, 0x3ffeb852U}}}, 0x3e4ccccdU, -1, true, {1, {{0, 0x3fe51eb8U}}}, {0, std::nullopt, 0, false, false}},
            {"scene-removal-reuse-return-do-not-register-old-hit-3", {1, {{0, 0x3fe51eb8U}}}, 0x3e4ccccdU, 2, true, {1, {{0, 0x3fcb851eU}}}, {0, std::nullopt, 0, false, false}},
            {"scene-removal-reuse-return-do-not-register-old-hit-4", {1, {{0, 0x3fcb851eU}}}, 0x3e4ccccdU, 20, true, {1, {{0, 0x3fb1eb84U}}}, {0, std::nullopt, 0, false, false}},
            {"scene-removal-reuse-return-do-not-register-old-hit-5", {1, {{0, 0x3fb1eb84U}}}, 0x3e4ccccdU, 52, true, {1, {{0, 0x3f9851eaU}}}, {0, std::nullopt, 0, false, false}},
            {"scene-removal-reuse-return-do-not-register-old-hit-6", {1, {{0, 0x3f9851eaU}}}, 0x3fa66666U, std::nullopt, true, {0, {}}, {1, std::nullopt, 0, false, false}}}},
    Sequence{{0, {}},
        {{"no-fire-clears-selection-no-repeat-corpse-1", {0, {}}, 0x3c23d70aU, 50, true, {1, {{0, 0x3ffeb852U}}}, {0, std::nullopt, 0, false, false}},
            {"no-fire-clears-selection-no-repeat-corpse-2", {1, {{0, 0x3ffeb852U}}}, 0x3e4ccccdU, std::nullopt, true, {1, {{0, 0x3fe51eb8U}}}, {0, std::nullopt, 0, false, false}},
            {"no-fire-clears-selection-no-repeat-corpse-3", {1, {{0, 0x3fe51eb8U}}}, 0x40000000U, std::nullopt, true, {0, {}}, {1, std::nullopt, 0, false, false}}}},
    Sequence{{2, {{0, 0x3f000000U}, {1, 0x3f800000U}}},
        {{"expired-group-new-event-joins-before-resolution-1", {2, {{0, 0x3f000000U}, {1, 0x3f800000U}}}, 0x3f000000U, 50, true, {0, {}},
             {3, MultiHitNotice{3, f(0x43200000U)}, 5, true, false}},
            {"expired-group-new-event-joins-before-resolution-2", {0, {}}, 0x3dcccccdU, 50, true, {1, {{0, 0x3ff33333U}}}, {0, std::nullopt, 0, false, false}}}},
    Sequence{{3, {{0, 0x3e000000U}, {1, 0x3f800000U}, {2, 0x3f800000U}}},
        {{"full-notice-pool-group-is-dropped-no-later-award-1", {3, {{0, 0x3e000000U}, {1, 0x3f800000U}, {2, 0x3f800000U}}}, 0x3e000000U, std::nullopt, false, {0, {}},
             {3, std::nullopt, 0, false, false}},
            {"full-notice-pool-group-is-dropped-no-later-award-2", {0, {}}, 0x3dcccccdU, std::nullopt, true, {0, {}}, {1, std::nullopt, 0, false, false}}}},
    Sequence{{0, {}}, {{"all-same-registration-large-wait-drops-award-1", {0, {}}, 0x00000000U, 50, true, {1, {{0, 0x40000000U}}}, {0, std::nullopt, 0, false, false}},
                          {"all-same-registration-large-wait-drops-award-2", {1, {{0, 0x40000000U}}}, 0x00000000U, 50, true, {2, {{0, 0x40000000U}, {1, 0x40000000U}}},
                              {0, std::nullopt, 0, false, false}},
                          {"all-same-registration-large-wait-drops-award-3", {2, {{0, 0x40000000U}, {1, 0x40000000U}}}, 0x00000000U, 50, true,
                              {3, {{0, 0x40000000U}, {1, 0x40000000U}, {2, 0x40000000U}}}, {0, std::nullopt, 0, false, false}},
                          {"all-same-registration-large-wait-drops-award-4", {3, {{0, 0x40000000U}, {1, 0x40000000U}, {2, 0x40000000U}}}, 0x40000000U, std::nullopt, true, {0, {}},
                              {1, std::nullopt, 0, false, false}}}},
    Sequence{{3, {{0, 0x3e000000U}, {1, 0x3f800000U}, {2, 0x3f800000U}}},
        {{"complete-group-followed-by-fresh-group-1", {3, {{0, 0x3e000000U}, {1, 0x3f800000U}, {2, 0x3f800000U}}}, 0x3e000000U, std::nullopt, true, {0, {}},
             {3, MultiHitNotice{3, f(0x43200000U)}, 5, true, false}},
            {"complete-group-followed-by-fresh-group-2", {0, {}}, 0x3c23d70aU, 40, true, {1, {{0, 0x3ffeb852U}}}, {0, std::nullopt, 0, false, false}},
            {"complete-group-followed-by-fresh-group-3", {1, {{0, 0x3ffeb852U}}}, 0x3e800000U, 50, true, {2, {{0, 0x3fdeb852U}, {1, 0x3fe00000U}}},
                {0, std::nullopt, 0, false, false}},
            {"complete-group-followed-by-fresh-group-4", {2, {{0, 0x3fdeb852U}, {1, 0x3fe00000U}}}, 0x3e800000U, 51, true,
                {3, {{0, 0x3fbeb852U}, {1, 0x3fc00000U}, {2, 0x3fe00000U}}}, {0, std::nullopt, 0, false, false}},
            {"complete-group-followed-by-fresh-group-5", {3, {{0, 0x3fbeb852U}, {1, 0x3fc00000U}, {2, 0x3fe00000U}}}, 0x3fc00000U, std::nullopt, true, {0, {}},
                {2, std::nullopt, 0, false, false}}}}};

using moorhuhn2::test::require;

MultiHitState state(const ExpectedState& expected) {
    MultiHitState value;
    value.write_index = expected.write_index;
    for (const auto& [index, bits] : expected.timers) {
        value.timers.at(index) = f(bits);
    }
    return value;
}

bool same_state(const MultiHitState& actual, const MultiHitState& expected) {
    if (actual.write_index != expected.write_index) {
        return false;
    }
    for (std::size_t index = 0; index < actual.timers.size(); ++index) {
        if (std::bit_cast<std::uint32_t>(actual.timers[index]) != std::bit_cast<std::uint32_t>(expected.timers[index])) {
            return false;
        }
    }
    return true;
}

void check_update(MultiHitState& actual, const UpdateCase& expected) {
    require(same_state(actual, state(expected.before)), "Original tracker sequence input differs");
    const auto result = update_multi_hit(actual, f(expected.delta_bits), expected.selected_type, expected.notice_available);
    require(same_state(actual, state(expected.after)), std::string(expected.id) + ": timer words or write index differs");
    require(result == expected.effect, std::string(expected.id) + ": notice, points, sound or bounded insertion differs");
}

void original_vectors() {
    for (const auto& expected : updates) {
        auto value = state(expected.before);
        check_update(value, expected);
    }
    for (const auto& sequence : sequences) {
        auto value = state(sequence.initial);
        for (const auto& expected : sequence.steps) {
            check_update(value, expected);
        }
    }
    require(MultiHitState{} == state(sequences[0].initial), "New-round tracker begins with zero timers and write index");
}

void rejects_unchanged(MultiHitState value, float delta) {
    const auto before = value;
    bool rejected = false;
    try {
        static_cast<void>(update_multi_hit(value, delta, 50, true));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && same_state(value, before), "Invalid external tracker state or time must be rejected before mutation");
}

void invalid_inputs() {
    rejects_unchanged({}, -0.01F);
    rejects_unchanged({}, std::numeric_limits<float>::quiet_NaN());
    rejects_unchanged({}, std::numeric_limits<float>::infinity());
    MultiHitState value;
    value.write_index = 33;
    rejects_unchanged(value, 0);
    value = {};
    value.timers[17] = 2.01F;
    rejects_unchanged(value, 0);
    value.timers[17] = std::numeric_limits<float>::quiet_NaN();
    rejects_unchanged(value, 0);
    value.timers[17] = -std::numeric_limits<float>::infinity();
    rejects_unchanged(value, 0);
}

} // namespace

int main() {
    try {
        original_vectors();
        invalid_inputs();
        std::cout << "Original multiple-hit types, delayed selection, timers, notice-dependent awards, reset and native capacity recovery passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
