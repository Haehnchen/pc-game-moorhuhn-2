#pragma once

#include <array>
#include <cstddef>
#include <optional>

namespace moorhuhn2::game::specials {

struct MultiHitState {
    std::array<float, 32> timers{};
    std::size_t write_index{};

    bool operator==(const MultiHitState&) const = default;
};

struct MultiHitNotice {
    int kind{};
    float y{160};

    bool operator==(const MultiHitNotice&) const = default;
};

struct MultiHitEffect {
    int count{};
    std::optional<MultiHitNotice> notice;
    int points{};
    bool score_sound{};
    // Original inserts past 32 overwrite unrelated memory. Native drops them
    // until the first timer resolves, while continuing to age the stored group.
    bool insertion_dropped{};

    bool operator==(const MultiHitEffect&) const = default;
};

// Run after scene/particle updates and earlier specials, before composition or
// current input. Supply the previous selected slot's current type, not its type
// at impact. Empty/missed/no-fire/reload actions leave no selection for next time.
// The caller reserves the first free shared notice slot before passing true;
// exhausted notice capacity suppresses the points and sound as in the original.
[[nodiscard]] MultiHitEffect update_multi_hit(MultiHitState& state, float delta, std::optional<int> selected_current_type, bool notice_available);

} // namespace moorhuhn2::game::specials
