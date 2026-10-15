#pragma once

#include "contracts/time.hpp"

#include <cstdint>

namespace moorhuhn2::platform {

// EXE +88/+d8/+dc: the current baseline, stored delta and accumulated
// total have independent lifetimes.
class SampleClock {
public:
    contracts::FrameTime sample(float current);
    void reset_previous(float current);
    const contracts::FrameTime& state() const;

private:
    contracts::FrameTime state_{};
};

struct CounterClockState {
    float reciprocal{};
    float origin{};
    float elapsed{};
    bool paused{};
};

// WTN counter arithmetic with source binary32 stores and 53-bit operations.
// Integer inputs are restricted to [-2^53, 2^53] (positive frequency) so
// conversion to double preserves the original exact signed-64-bit FILD value.
// Native callers subtract a captured counter origin before passing counters.
class CounterClock {
public:
    CounterClock(std::int64_t frequency, std::int64_t initial_counter);

    float sample(std::int64_t counter);
    void pause(std::int64_t counter);
    void resume(std::int64_t counter);
    const CounterClockState& state() const;
    std::int64_t frequency() const;

private:
    std::int64_t frequency_;
    CounterClockState state_{};
};

} // namespace moorhuhn2::platform
