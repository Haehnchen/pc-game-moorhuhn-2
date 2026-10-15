#include "ui/house88_results.hpp"

#include "contracts/time.hpp"

#include <stdexcept>
#include <string_view>

namespace moorhuhn2::ui {

House88Results::House88Results(int actual_score, ResultsRandom& random) {
    if (actual_score >= 1500) {
        for (int index = 0; index < 64; ++index) {
            static_cast<void>(random.next());
        }
    }

    for (auto& chicken : chickens_) {
        chicken.x = static_cast<float>(random.next() % 600);
        chicken.y = static_cast<float>(-64 - static_cast<int>(random.next() % 128));
        chicken.speed = static_cast<float>(50 + random.next() % 50);
        chicken.variant = static_cast<int>(random.next() % 2);
    }
}

void House88Results::update(float delta, ResultsRandom& random) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Results delta must be finite and nonnegative");
    }

    for (const auto& chicken : chickens_) {
        if (static_cast<long double>(chicken.animation_time) + static_cast<long double>(delta) * 10 >= 2147483648.0L) {
            throw std::invalid_argument("Results animation must fit a signed 32-bit frame conversion");
        }
    }

    for (auto& chicken : chickens_) {
        chicken.y = static_cast<float>(static_cast<long double>(chicken.y) + static_cast<long double>(chicken.speed) * delta);
        const auto animation = static_cast<long double>(chicken.animation_time) + static_cast<long double>(delta) * 10;
        chicken.animation_time = static_cast<float>(animation);
        // FST keeps the extended sum for _ftol; recycling does not clear time.
        chicken.frame = static_cast<int>(animation) % 10;
        if (chicken.y > 480) {
            chicken.y = static_cast<float>(-64 - static_cast<int>(random.next() % 128));
            chicken.x = static_cast<float>(random.next() % 600);
            chicken.frame = 0;
            chicken.speed = static_cast<float>(50 + random.next() % 50);
            chicken.variant = static_cast<int>(random.next() % 2);
        }
    }
}

std::array<contracts::SpriteDraw, 16> House88Results::draws() const {
    constexpr std::array<std::string_view, 2> images{"gfx/DChickleft2", "gfx/DChickright2"};
    std::array<contracts::SpriteDraw, 16> result;
    for (std::size_t index = 0; index < chickens_.size(); ++index) {
        const auto& chicken = chickens_[index];
        result[index] = {images[static_cast<std::size_t>(chicken.variant)], {0, chicken.frame * 62, 83, 62}, {static_cast<int>(chicken.x), static_cast<int>(chicken.y)}};
    }

    return result;
}

} // namespace moorhuhn2::ui
