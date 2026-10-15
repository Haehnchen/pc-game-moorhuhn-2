#include "platform/clock.hpp"

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

struct SampleExpectation {
    std::uint32_t current_bits{};
    std::uint32_t delta_bits{};
    std::uint32_t total_bits{};
};

struct SampleStep {
    std::string operation{};
    std::uint32_t sample_bits{};
    SampleExpectation expected{};
};

struct SampleCase {
    std::string id{};
    SampleExpectation initial{};
    std::vector<SampleStep> steps{};
};

struct CounterExpectation {
    std::uint32_t reciprocal_bits{};
    std::uint32_t origin_bits{};
    std::uint32_t elapsed_bits{};
    bool paused{};
};

struct CounterStep {
    std::string operation{};
    std::int64_t counter{};
    CounterExpectation expected{};
    std::uint32_t returned_bits{};
};

struct CounterCase {
    std::string id{};
    std::int64_t frequency{};
    std::int64_t initial_counter{};
    CounterExpectation initial{};
    std::vector<CounterStep> steps{};
};

struct FocusStep {
    std::string operation{};
    std::int64_t counter{};
    CounterExpectation expected_counter{};
    SampleExpectation expected_application{};
};

struct FocusCase {
    std::string id{};
    std::int64_t frequency{};
    std::int64_t initial_counter{};
    CounterExpectation initial_counter_state{};
    SampleExpectation initial_application_state{};
    std::vector<FocusStep> steps{};
};

const std::array<SampleCase, 4> sample_cases = {
    {{"cold-and-zero-delta", {0x00000000U, 0x00000000U, 0x00000000U},
         {{"sample", 0x00000000U, {0x00000000U, 0x00000000U, 0x00000000U}}, {"sample", 0x3e000000U, {0x3e000000U, 0x3e000000U, 0x3e000000U}},
             {"sample", 0x3e000000U, {0x3e000000U, 0x00000000U, 0x3e000000U}}}},
        {"retained-delta-total", {0x00000000U, 0x00000000U, 0x00000000U},
            {{"reset", 0x3dcccccdU, {0x3dcccccdU, 0x00000000U, 0x00000000U}}, {"sample", 0x3e4ccccdU, {0x3e4ccccdU, 0x3dcccccdU, 0x3dcccccdU}},
                {"sample", 0x3f800000U, {0x3f800000U, 0x3f4ccccdU, 0x3f666666U}}}},
        {"baseline-preserves-total", {0x00000000U, 0x00000000U, 0x00000000U},
            {{"reset", 0x3f400000U, {0x3f400000U, 0x00000000U, 0x00000000U}}, {"sample", 0x3f800000U, {0x3f800000U, 0x3e800000U, 0x3e800000U}},
                {"reset", 0x40000000U, {0x40000000U, 0x3e800000U, 0x3e800000U}}, {"sample", 0x40100000U, {0x40100000U, 0x3e800000U, 0x3f000000U}},
                {"sample", 0x40100000U, {0x40100000U, 0x00000000U, 0x3f000000U}}}},
        {"negative-elapsed-and-delta", {0x00000000U, 0x00000000U, 0x00000000U},
            {{"sample", 0xbd800000U, {0xbd800000U, 0xbd800000U, 0xbd800000U}}, {"sample", 0x00000000U, {0x00000000U, 0x3d800000U, 0x00000000U}},
                {"reset", 0x3f800000U, {0x3f800000U, 0x3d800000U, 0x00000000U}}, {"sample", 0x3f400000U, {0x3f400000U, 0xbe800000U, 0xbe800000U}}}}}};

const std::array<CounterCase, 6> counter_cases = {{{"cold-counter", 8, 0, {0x3e000000U, 0x00000000U, 0x00000000U, false},
                                                       {{"sample", 0, {0x3e000000U, 0x00000000U, 0x00000000U, false}, 0x00000000U},
                                                           {"sample", 1, {0x3e000000U, 0x00000000U, 0x3e000000U, false}, 0x3e000000U},
                                                           {"sample", 8, {0x3e000000U, 0x00000000U, 0x3f800000U, false}, 0x3f800000U}}},
    {"retained-reset-reciprocal", 10, 10, {0x3dcccccdU, 0x3f800000U, 0x00000000U, false},
        {{"sample", 10, {0x3dcccccdU, 0x3f800000U, 0x32800000U, false}, 0x32800000U}, {"sample", 11, {0x3dcccccdU, 0x3f800000U, 0x3dcccccfU, false}, 0x3dcccccfU},
            {"pause", 12, {0x3dcccccdU, 0x3f800000U, 0x3e4cccceU, true}, 0}, {"sample", 100, {0x3dcccccdU, 0x3f800000U, 0x3e4cccceU, true}, 0x3e4cccceU},
            {"pause", 101, {0x3dcccccdU, 0x3f800000U, 0x3e4cccceU, true}, 0}, {"resume", 112, {0x3dcccccdU, 0x41300000U, 0x3e4cccceU, false}, 0},
            {"sample", 112, {0x3dcccccdU, 0x41300000U, 0x3e4cccd8U, false}, 0x3e4cccd8U}, {"sample", 113, {0x3dcccccdU, 0x41300000U, 0x3e99999fU, false}, 0x3e99999fU}}},
    {"negative-same-counter-elapsed", 3000000, 3000000000000, {0x34b2f4fcU, 0x49742400U, 0x00000000U, false},
        {{"sample", 3000000000000, {0x34b2f4fcU, 0x49742400U, 0xbb257666U, false}, 0xbb257666U},
            {"sample", 3000000000001, {0x34b2f4fcU, 0x49742400U, 0xbb2570ceU, false}, 0xbb2570ceU}, {"pause", 3000000001000, {0x34b2f4fcU, 0x49742400U, 0xbb0f9dfeU, true}, 0},
            {"sample", 3000003000000, {0x34b2f4fcU, 0x49742400U, 0xbb0f9dfeU, true}, 0xbb0f9dfeU}, {"resume", 3000003000000, {0x34b2f4fcU, 0x49742410U, 0xbb0f9dfeU, false}, 0},
            {"sample", 3000003000001, {0x34b2f4fcU, 0x49742410U, 0xbb2570d9U, false}, 0xbb2570d9U}}},
    {"negative-counter-domain", 10, -10, {0x3dcccccdU, 0xbf800000U, 0x00000000U, false},
        {{"sample", -10, {0x3dcccccdU, 0xbf800000U, 0xb2800000U, false}, 0xb2800000U}, {"sample", -11, {0x3dcccccdU, 0xbf800000U, 0xbdcccccfU, false}, 0xbdcccccfU},
            {"pause", 0, {0x3dcccccdU, 0xbf800000U, 0x3f800000U, true}, 0}, {"resume", 10, {0x3dcccccdU, 0x32800000U, 0x3f800000U, false}, 0},
            {"sample", 11, {0x3dcccccdU, 0x32800000U, 0x3f8ccccdU, false}, 0x3f8ccccdU}}},
    {"exact-counter-domain-boundary", 1, 9007199254740992, {0x3f800000U, 0x5a000000U, 0x00000000U, false},
        {{"sample", 9007199254740992, {0x3f800000U, 0x5a000000U, 0x00000000U, false}, 0x00000000U},
            {"sample", 9007199254740991, {0x3f800000U, 0x5a000000U, 0xbf800000U, false}, 0xbf800000U},
            {"sample", -9007199254740992, {0x3f800000U, 0x5a000000U, 0xda800000U, false}, 0xda800000U}}},
    {"maximum-frequency", 9007199254740992, 0, {0x25000000U, 0x00000000U, 0x00000000U, false},
        {{"sample", 1, {0x25000000U, 0x00000000U, 0x25000000U, false}, 0x25000000U}, {"sample", 9007199254740992, {0x25000000U, 0x00000000U, 0x3f800000U, false}, 0x3f800000U}}}}};

const FocusCase focus_case = {"sample-before-focus-pause", 8, 0, {0x3e000000U, 0x00000000U, 0x00000000U, false}, {0x00000000U, 0x00000000U, 0x00000000U},
    {{"baseline", 56, {0x3e000000U, 0x00000000U, 0x40e00000U, false}, {0x40e00000U, 0x00000000U, 0x00000000U}},
        {"accepted", 80, {0x3e000000U, 0x00000000U, 0x41200000U, false}, {0x41200000U, 0x40400000U, 0x40400000U}},
        {"loss", 82, {0x3e000000U, 0x00000000U, 0x41240000U, false}, {0x41240000U, 0x3e800000U, 0x40500000U}},
        {"pause", 83, {0x3e000000U, 0x00000000U, 0x41260000U, true}, {0x41240000U, 0x3e800000U, 0x40500000U}},
        {"inactive", 120, {0x3e000000U, 0x00000000U, 0x41260000U, true}, {0x41240000U, 0x3e800000U, 0x40500000U}},
        {"paused_get", 159, {0x3e000000U, 0x00000000U, 0x41260000U, true}, {0x41240000U, 0x3e800000U, 0x40500000U}},
        {"resume", 160, {0x3e000000U, 0x411a0000U, 0x41260000U, false}, {0x41240000U, 0x3e800000U, 0x40500000U}},
        {"accepted", 161, {0x3e000000U, 0x411a0000U, 0x41280000U, false}, {0x41280000U, 0x3e800000U, 0x40600000U}}}};

using moorhuhn2::contracts::FrameTime;
using moorhuhn2::platform::CounterClock;
using moorhuhn2::platform::CounterClockState;
using moorhuhn2::platform::SampleClock;

void check(bool condition, std::string_view id, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(id) + ": " + std::string(message));
    }
}

float floating(std::uint32_t value) {
    return std::bit_cast<float>(value);
}

void check_float(float actual, std::uint32_t expected, std::string_view id, std::string_view field) {
    check(std::bit_cast<std::uint32_t>(actual) == expected, id, field);
}

void check_frame(const FrameTime& state, const SampleExpectation& expected, std::string_view id) {
    check_float(state.current, expected.current_bits, id, "Stored current sample bits");
    check_float(state.delta, expected.delta_bits, id, "Stored subtraction bits");
    check_float(state.total, expected.total_bits, id, "Total retains the subtraction before delta's binary32 store");
}

void check_counter(const CounterClock& clock, const CounterExpectation& expected, std::string_view id) {
    const auto& state = clock.state();
    check_float(state.reciprocal, expected.reciprocal_bits, id, "Stored reciprocal bits");
    check_float(state.origin, expected.origin_bits, id, "Origin from retained reset reciprocal or ordered ContinueTime operations");
    check_float(state.elapsed, expected.elapsed_bits, id, "Stored/reloaded signed elapsed bits");
    check(state.paused == expected.paused, id, "Pause state");
}

void sample_sequences(std::span<const SampleCase> cases) {
    for (const auto& data : cases) {
        const auto id = data.id;
        SampleClock clock;
        check_frame(clock.state(), data.initial, id);

        for (const auto& step : data.steps) {
            const auto operation = step.operation;

            if (operation == "reset") {
                clock.reset_previous(floating(step.sample_bits));
            } else if (operation == "sample") {
                check_frame(clock.sample(floating(step.sample_bits)), step.expected, id);
            } else {
                throw std::runtime_error("Unknown application clock fixture operation " + operation);
            }

            check_frame(clock.state(), step.expected, id);
        }
    }
}

void counter_sequences(std::span<const CounterCase> cases) {
    for (const auto& data : cases) {
        const auto id = data.id;
        CounterClock clock(data.frequency, data.initial_counter);
        check(clock.frequency() == data.frequency, id, "Fixed counter frequency");
        check_counter(clock, data.initial, id);

        for (const auto& step : data.steps) {
            const auto operation = step.operation;
            const auto counter = step.counter;

            if (operation == "sample") {
                check_float(clock.sample(counter), step.returned_bits, id, "Returned GetTime value is the stored elapsed");
            } else if (operation == "pause") {
                clock.pause(counter);
            } else if (operation == "resume") {
                clock.resume(counter);
            } else {
                throw std::runtime_error("Unknown counter clock fixture operation " + operation);
            }

            check_counter(clock, step.expected, id);
        }
    }
}

void focus_sequence(const FocusCase& data) {
    const auto id = data.id;
    CounterClock counter(data.frequency, data.initial_counter);
    SampleClock application;
    check_counter(counter, data.initial_counter_state, id);
    check_frame(application.state(), data.initial_application_state, id);

    for (const auto& step : data.steps) {
        const auto operation = step.operation;
        const auto value = step.counter;

        if (operation == "baseline") {
            application.reset_previous(counter.sample(value));
        } else if (operation == "accepted" || operation == "loss") {
            application.sample(counter.sample(value));
        } else if (operation == "pause") {
            counter.pause(value);
        } else if (operation == "resume") {
            counter.resume(value);
        } else if (operation == "paused_get") {
            counter.sample(value);
        } else if (operation != "inactive") {
            throw std::runtime_error("Unknown focus clock fixture operation " + operation);
        }

        check_counter(counter, step.expected_counter, id);
        check_frame(application.state(), step.expected_application, id);
    }
}

template <typename Exception, typename Function> void check_throws(Function function, std::string_view id) {
    bool caught = false;
    try {
        function();
    } catch (const Exception&) {
        caught = true;
    }

    check(caught, id, "Required explicit domain rejection");
}

bool same_counter(const CounterClockState& left, const CounterClockState& right) {
    return std::bit_cast<std::uint32_t>(left.reciprocal) == std::bit_cast<std::uint32_t>(right.reciprocal)
           && std::bit_cast<std::uint32_t>(left.origin) == std::bit_cast<std::uint32_t>(right.origin)
           && std::bit_cast<std::uint32_t>(left.elapsed) == std::bit_cast<std::uint32_t>(right.elapsed) && left.paused == right.paused;
}

void invalid_domains() {
    constexpr std::int64_t outside = (std::int64_t{1} << 53) + 1;
    check_throws<std::invalid_argument>(
        [] {
            CounterClock clock(0, 0);
        },
        "zero-frequency");
    check_throws<std::invalid_argument>(
        [] {
            CounterClock clock(-1, 0);
        },
        "negative-frequency");
    check_throws<std::out_of_range>(
        [] {
            CounterClock clock(outside, 0);
        },
        "inexact-frequency");
    check_throws<std::out_of_range>(
        [] {
            CounterClock clock(1, outside);
        },
        "inexact-initial-counter");
    check_throws<std::out_of_range>(
        [] {
            CounterClock clock(1, -outside);
        },
        "inexact-negative-initial-counter");
    CounterClock counter(8, 0);
    counter.pause(1);
    const auto before_counter = counter.state();
    check_throws<std::out_of_range>(
        [&] {
            counter.sample(outside);
        },
        "inexact-paused-counter");
    check_throws<std::out_of_range>(
        [&] {
            counter.pause(-outside);
        },
        "inexact-pause-counter");
    check_throws<std::out_of_range>(
        [&] {
            counter.resume(outside);
        },
        "inexact-resume-counter");
    check(same_counter(counter.state(), before_counter), "counter-rejection", "Failed counter inputs preserve all state");

    SampleClock application;
    application.reset_previous(0.75F);
    application.sample(1.0F);
    const auto before_frame = application.state();
    check_throws<std::invalid_argument>(
        [&] {
            application.sample(std::numeric_limits<float>::infinity());
        },
        "infinite-sample");
    check_throws<std::invalid_argument>(
        [&] {
            application.sample(std::numeric_limits<float>::quiet_NaN());
        },
        "nan-sample");
    check_throws<std::invalid_argument>(
        [&] {
            application.reset_previous(-std::numeric_limits<float>::infinity());
        },
        "infinite-baseline");
    check_throws<std::invalid_argument>(
        [&] {
            application.reset_previous(std::numeric_limits<float>::quiet_NaN());
        },
        "nan-baseline");
    check(application.state() == before_frame, "application-rejection", "Failed samples/baselines preserve all state");

    SampleClock overflow;
    overflow.reset_previous(-std::numeric_limits<float>::max());
    const auto before_overflow = overflow.state();
    check_throws<std::out_of_range>(
        [&] {
            overflow.sample(std::numeric_limits<float>::max());
        },
        "delta-overflow");
    check(overflow.state() == before_overflow, "delta-overflow", "Failed delta store preserves all state");
    overflow.reset_previous(0);
    overflow.sample(std::numeric_limits<float>::max());
    overflow.reset_previous(0);
    const auto before_total = overflow.state();
    check_throws<std::out_of_range>(
        [&] {
            overflow.sample(std::numeric_limits<float>::max());
        },
        "total-overflow");
    check(overflow.state() == before_total, "total-overflow", "Failed total store preserves all state");
}

} // namespace

int main() {
    try {
        sample_sequences(sample_cases);
        counter_sequences(counter_cases);
        focus_sequence(focus_case);
        invalid_domains();
        std::cout << "Original-derived sample/counter bit patterns, retained totals, baselines, focus pause/resume and invalid domains passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
