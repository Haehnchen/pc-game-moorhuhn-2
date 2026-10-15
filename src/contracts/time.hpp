#pragma once

#include <cmath>

namespace moorhuhn2::contracts {

// One accepted engine sample. Total time survives stage-entry baselines;
// delta excludes updates that were sampled but skipped before gameplay.
struct FrameTime {
    float current{};
    float delta{};
    float total{};

    bool operator==(const FrameTime&) const = default;
};

[[nodiscard]] inline bool valid_delta(float delta) {
    return std::isfinite(delta) && delta >= 0;
}

} // namespace moorhuhn2::contracts
