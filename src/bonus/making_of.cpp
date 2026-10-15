#include "bonus/making_of.hpp"

#include "contracts/angle.hpp"
#include "contracts/time.hpp"

#include <cmath>
#include <stdexcept>

namespace moorhuhn2::bonus {

MakingOf::MakingOf(bool witan_logo) : witan_logo_(witan_logo) {}

void MakingOf::update(float delta) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Making-of delta must be finite and nonnegative");
    }

    if (!witan_logo_) {
        return;
    }

    const auto next_x = static_cast<float>(static_cast<long double>(x_phase_) + static_cast<long double>(delta) * 0.008F);
    const auto next_y = static_cast<float>(static_cast<long double>(y_phase_) + static_cast<long double>(delta) * 0.007F);
    // Beyond this domain x87 FSIN/FCOS leave their argument unchanged, making _ftol unsafe.
    if (static_cast<long double>(next_x) * contracts::degrees_per_radian >= 9223372036854775808.0L
        || static_cast<long double>(next_y) * contracts::degrees_per_radian >= 9223372036854775808.0L) {
        throw std::invalid_argument("Making-of phases must fit the original trigonometric domain");
    }
    // The source evaluates both old phases before storing either new binary32 phase.
    logo_position_ = {static_cast<int>(std::sin(static_cast<long double>(x_phase_) * contracts::degrees_per_radian) * 200.0L + 220.0L),
        static_cast<int>(std::cos(static_cast<long double>(y_phase_) * contracts::degrees_per_radian) * 200.0L + 140.0L)};
    x_phase_ = next_x;
    y_phase_ = next_y;
}

int MakingOf::portrait_frame(contracts::ScreenPoint pointer) noexcept {
    return (pointer.x / 213 + pointer.y / 160) / 2;
}

std::vector<contracts::SpriteDraw> MakingOf::draws(contracts::ScreenPoint pointer) const {
    if (witan_logo_) {
        return {{"gfx/WitanLogo", {0, 0, 200, 203}, logo_position_}};
    }

    const int portrait = portrait_frame(pointer);

    return {{"gfx/Making", {0, 0, 640, 480}, {0, 0}}, {"gfx/jennie", {0, portrait * 100, 125, 100}, {515, 327}}, {"gfx/kopf", {0, 0, 46, 29}, {594, 399}}};
}

} // namespace moorhuhn2::bonus
