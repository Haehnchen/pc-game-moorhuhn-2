#include "game/round.hpp"
#include "platform/clock.hpp"
#include "support.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using moorhuhn2::game::test::fixture;
using moorhuhn2::game::test::require;
using namespace moorhuhn2;

std::uint32_t word(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

void install_notice(game::Round& round) {
    // Inject one active notice directly; everything after this uses the public update order.
    fixture(round).notices[0] = {.active = true, .visible = false, .age = 0, .y = 300, .type = 100};
}

void step(game::Round& round, contracts::FrameTime time, game::RoundInput input = {}) {
    round.advance(time, input);
    round.resolve_actions({});
}

void retained_total_reaches_round() {
    // EXE 40eb50–40ebac: FST delta preserves the subtraction operand for the following FADD.
    platform::SampleClock clock;
    clock.reset_previous(.1F);
    const auto first = clock.sample(.2F);
    require(word(first.delta) == 0x3dcccccdU && word(first.total) == 0x3dcccccdU, "First source sample stores the binary32 delta and total");
    const auto time = clock.sample(1);
    require(word(time.current) == 0x3f800000U && word(time.delta) == 0x3f4ccccdU && word(time.total) == 0x3f666666U,
        "Retained subtraction produces source total3f666666 rather than reloaded-delta total3f666667");

    game::Round round(1, .2F);
    install_notice(round);
    step(round, time);
    require(word(round.state().previous_sample) == 0x3f800000U && word(round.state().delta) == 0x3f4ccccdU && word(round.state().accumulated_time) == 0x3f666666U,
        "Round retains the independently supplied current sample, stored delta and total");
    require(round.state().notices[0].active && !round.state().notices[0].visible, "Stored total produces notice tick53, mask21, hidden");
}

void baseline_and_notice_clock() {
    // Stage entry changes +88 only; 40fdca–40fde0 uses stored +dc *60, truncation and &31.
    platform::SampleClock clock;
    clock.reset_previous(.75F);
    game::Round round(1, .75F), current_clock(1, .75F);
    install_notice(round);
    install_notice(current_clock);

    const auto first = clock.sample(1);
    require(first == contracts::FrameTime{1, .25F, .25F}, "Stage baseline yields current1, delta.25 and accumulated.25");
    step(round, first);
    step(current_clock, {first.current, first.delta, first.current});
    require(round.state().remaining == 89.75F && round.state().notices[0].age == .25F && round.state().notices[0].y == 290 && round.state().notices[0].visible
                && world::notice_text(round.state().notices[0]).has_value(),
        "Accumulated notice tick15 remains visible through normal Round composition");
    require(
        !current_clock.state().notices[0].visible && !world::notice_text(current_clock.state().notices[0]).has_value(), "Using current1 instead produces tick60, mask28, hidden");

    clock.reset_previous(2);
    require(clock.state() == contracts::FrameTime{2, .25F, .25F}, "Baseline reset preserves stored delta and total");
    round.reset_sample(2);
    current_clock.reset_sample(2);
    const auto second = clock.sample(2.25F);
    require(second == contracts::FrameTime{2.25F, .25F, .5F}, "Next stage sample adds only .25 to the retained total");
    step(round, second);
    step(current_clock, {second.current, second.delta, second.current});
    require(round.state().remaining == 89.5F && round.state().notices[0].age == .5F && round.state().notices[0].y == 280 && !round.state().notices[0].visible,
        "Retained accumulated.5 selects tick30, hidden after entry reset");
    require(current_clock.state().notices[0].visible, "Using current2.25 instead selects tick135, mask7, visible");
}

void skipped_dispatch_uses_current_frame() {
    // Focus-loss and boss-screen frames keep sampling without dispatching hunting.
    // The returning frame receives its own delta and current mouse flags together.
    platform::SampleClock clock;
    clock.reset_previous(7);
    require(clock.sample(10) == contracts::FrameTime{10, 3, 3}, "Establish source baseline current10 and accumulated3");
    game::Round round(1, 10);
    install_notice(round);

    require(clock.sample(10.25F) == contracts::FrameTime{10.25F, .25F, 3.25F}, "Skipped application update still accumulates its sampled .25");
    require(clock.sample(10.5F) == contracts::FrameTime{10.5F, .25F, 3.5F}, "Further skipped samples retain the accumulated total");

    const auto returned = clock.sample(10.625F);
    require(returned == contracts::FrameTime{10.625F, .125F, 3.625F}, "Return receives only the current .125 frame and retained total3.625");
    step(round, returned, {.fire_down = true, .held_left = true});
    require(round.state().remaining == 89.875F && round.state().camera == 615 && round.state().notices[0].age == .125F && round.state().notices[0].y == 295
                && round.state().ammo_index == 9 && round.state().misses == 1 && round.state().updates == 1 && round.state().previous_sample == 10.625F
                && round.state().delta == .125F && round.state().accumulated_time == 3.625F,
        "Return applies current movement, timer and fire together without .625 of catch-up");
    require(std::ranges::count(round.events(), game::RoundEventKind::shot, &game::RoundEvent::kind) == 1, "The same returning frame resolves fire once");
}

void invalid_frame_is_atomic() {
    const float infinity = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const std::array invalid_frames{contracts::FrameTime{-1, .25F, .5F}, contracts::FrameTime{nan, .25F, .5F}, contracts::FrameTime{infinity, .25F, .5F},
        contracts::FrameTime{2147483648.0F, .25F, .5F}, contracts::FrameTime{1.25F, -1, .5F}, contracts::FrameTime{1.25F, nan, .5F}, contracts::FrameTime{1.25F, infinity, .5F},
        contracts::FrameTime{1.25F, 2147483648.0F, .5F}, contracts::FrameTime{1.25F, .25F, -1}, contracts::FrameTime{1.25F, .25F, nan}, contracts::FrameTime{1.25F, .25F, infinity},
        contracts::FrameTime{1.25F, .25F, 35791396.0F}, contracts::FrameTime{.75F, .25F, .5F}};

    game::Round round(1, .75F);
    install_notice(round);
    step(round, {1, .25F, .25F}, {.fire_down = true});
    const auto before = round.state();
    const std::vector events(round.events().begin(), round.events().end());
    for (const auto invalid : invalid_frames) {
        bool rejected = false;
        try {
            round.advance(invalid, {.fire_down = true, .reload_down = true});
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Negative/nonfinite/overflowing clock fields and backwards current samples must be rejected");
        require(round.state() == before && std::ranges::equal(round.events(), events), "Invalid frame rejection preserves state, RNG and prior events atomically");
    }
    step(round, {1.25F, .25F, .5F});
    require(round.state().remaining == 89.5F && round.state().updates == 2, "An invalid clock frame cannot leave pending actions that block the next accepted frame");
}
} // namespace

int main() {
    try {
        retained_total_reaches_round();
        baseline_and_notice_clock();
        skipped_dispatch_uses_current_frame();
        invalid_frame_is_atomic();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Source retained clock, accumulated notice blink, focus/boss gaps and atomic frame validation passed\n";
}
