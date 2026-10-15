#include "game/specials/multi_hit.hpp"

#include <cmath>
#include <stdexcept>

namespace moorhuhn2::game::specials {
namespace {

// Eligibility is a specific reaction set; several other dying records are excluded.
bool qualifies(int type) {
    switch (type) {
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 50:
        case 51:
        case 60:
        case 61:
            return true;
        default:
            return false;
    }
}

} // namespace

MultiHitEffect update_multi_hit(MultiHitState& state, float delta, std::optional<int> selected_current_type, bool notice_available) {
    if (!std::isfinite(delta) || delta < 0 || state.write_index > state.timers.size()) {
        throw std::invalid_argument("Invalid multiple-hit delta or write index");
    }

    for (const auto timer : state.timers) {
        if (!std::isfinite(timer) || timer > 2) {
            throw std::invalid_argument("Invalid multiple-hit timer");
        }
    }

    auto next = state;
    MultiHitEffect effect;

    if (selected_current_type && qualifies(*selected_current_type)) {
        if (next.write_index < next.timers.size()) {
            next.timers[next.write_index++] = 2;
        } else {
            effect.insertion_dropped = true;
        }
    }

    for (auto& timer : next.timers) {
        if (timer > 0) {
            timer = static_cast<float>(static_cast<long double>(timer) - delta);
        }
    }

    if (next.timers.front() <= 0) {
        effect.count = 1;

        for (std::size_t index = 1; index < next.timers.size(); ++index) {
            if (next.timers[index] > 0) {
                ++effect.count;
                next.timers[index] = 0;
            }
        }

        next.write_index = 0;
        next.timers.front() = 0;

        if (effect.count > 2 && notice_available) {
            effect.notice = MultiHitNotice{effect.count};

            switch (effect.count) {
                case 3:
                    effect.points = 5;
                    break;
                case 4:
                    effect.points = 10;
                    break;
                case 5:
                    effect.points = 15;
                    break;
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                    effect.points = 20;
                    break;
                default:
                    break;
            }

            effect.score_sound = effect.points != 0;
        }
    }

    state = next;

    return effect;
}

} // namespace moorhuhn2::game::specials
