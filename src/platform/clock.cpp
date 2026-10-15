#include "platform/clock.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::platform {
namespace {

static_assert(std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::digits == 24);
static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53);

constexpr std::int64_t largest_exact_integer = std::int64_t{1} << 53;

double exact_counter(std::int64_t counter) {
    if (counter < -largest_exact_integer || counter > largest_exact_integer) {
        throw std::out_of_range("Clock counter exceeds the exact binary64 integer domain");
    }
    return static_cast<double>(counter);
}

// Each source x87 operation uses the EXE's 53-bit precision setting.
// Volatile stores also prevent a compiler from contracting/reassociating them.
double multiply(double left, double right) {
    volatile double result = left * right;
    return result;
}

double subtract(double left, double right) {
    volatile double result = left - right;
    return result;
}

double add(double left, double right) {
    volatile double result = left + right;
    return result;
}

} // namespace

contracts::FrameTime SampleClock::sample(float current) {
    if (!std::isfinite(current)) {
        throw std::invalid_argument("Clock sample must be finite");
    }
    const double retained_delta = subtract(static_cast<double>(current), static_cast<double>(state_.current));
    const float delta = static_cast<float>(retained_delta);
    // FST delta retains the unrounded subtraction for the total's FADD.
    const float total = static_cast<float>(add(retained_delta, static_cast<double>(state_.total)));
    if (!std::isfinite(delta) || !std::isfinite(total)) {
        throw std::out_of_range("Clock sample exceeds the finite binary32 state domain");
    }
    state_ = {current, delta, total};
    return state_;
}

void SampleClock::reset_previous(float current) {
    if (!std::isfinite(current)) {
        throw std::invalid_argument("Clock baseline must be finite");
    }
    state_.current = current;
}

const contracts::FrameTime& SampleClock::state() const {
    return state_;
}

CounterClock::CounterClock(std::int64_t frequency, std::int64_t initial_counter) : frequency_(frequency) {
    if (frequency <= 0) {
        throw std::invalid_argument("Clock frequency must be positive");
    }
    const double divisor = exact_counter(frequency);
    const double counter = exact_counter(initial_counter);
    volatile double reciprocal = 1.0 / divisor;
    state_.reciprocal = static_cast<float>(reciprocal);
    // ResetTime retains the reciprocal before its binary32 store for Q0*1/F.
    state_.origin = static_cast<float>(multiply(counter, reciprocal));
}

float CounterClock::sample(std::int64_t counter) {
    const double value = exact_counter(counter);
    if (!state_.paused) {
        const double scaled = multiply(value, static_cast<double>(state_.reciprocal));
        state_.elapsed = static_cast<float>(subtract(scaled, static_cast<double>(state_.origin)));
    }
    return state_.elapsed;
}

void CounterClock::pause(std::int64_t counter) {
    sample(counter);
    state_.paused = true;
}

void CounterClock::resume(std::int64_t counter) {
    const double value = exact_counter(counter);
    const double origin = static_cast<double>(state_.origin);
    const double scaled = multiply(value, static_cast<double>(state_.reciprocal));
    const double since_origin = subtract(scaled, origin);
    const double adjustment = subtract(since_origin, static_cast<double>(state_.elapsed));
    // ContinueTime performs all four operations; cancellation changes rounding.
    state_.origin = static_cast<float>(add(adjustment, origin));
    state_.paused = false;
}

const CounterClockState& CounterClock::state() const {
    return state_;
}

std::int64_t CounterClock::frequency() const {
    return frequency_;
}

} // namespace moorhuhn2::platform
