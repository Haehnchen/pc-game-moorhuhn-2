#include "bonus/computer.hpp"

#include "contracts/random.hpp"
#include "contracts/time.hpp"

#include <stdexcept>

namespace moorhuhn2::bonus {
namespace {

ComputerChicken new_chicken(std::uint32_t& random_state) {
    ComputerChicken chicken;
    chicken.direction = static_cast<int>(contracts::next_random(random_state) % 2);
    const int offset = static_cast<int>(contracts::next_random(random_state) % 256);
    chicken.x = static_cast<float>(chicken.direction == 0 ? -128 - offset : 640 + offset);
    chicken.y = static_cast<float>(contracts::next_random(random_state) % 400);
    chicken.animation_time = static_cast<float>(contracts::next_random(random_state) % 19);
    chicken.speed = static_cast<float>(contracts::next_random(random_state) % 100 + 50);

    return chicken;
}

} // namespace

Computer::Computer(std::uint32_t& random_state) {
    for (auto& chicken : chickens_) {
        chicken = new_chicken(random_state);
        // Entry consumes the phase draw, then overwrites it. Recycling retains it.
        chicken.animation_time = 0;
    }
}

void Computer::update(float delta, std::uint32_t& random_state) {
    if (!contracts::valid_delta(delta) || static_cast<long double>(delta) * 3.0F * 5.0F + 19.0F >= 2147483648.0L) {
        throw std::invalid_argument("Computer delta must be finite, nonnegative and fit the original frame conversion");
    }

    for (auto& chicken : chickens_) {
        const long double movement = static_cast<long double>(delta) * chicken.speed;
        const long double x = chicken.direction == 0 ? chicken.x + movement : chicken.x - movement;
        chicken.x = static_cast<float>(x);
        // FST stores binary32, but the strict boundary compares the retained x87 result.
        if ((chicken.direction == 0 && x > 640.0F) || (chicken.direction == 1 && x < -128.0F)) {
            chicken = new_chicken(random_state);
        }

        const long double animation = 3.0L * delta * 5.0F + chicken.animation_time;
        chicken.animation_time = static_cast<float>(animation);
        // _ftol also consumes the retained sum, before the binary32 store is reloaded.
        chicken.frame = static_cast<int>(animation);
        if (chicken.frame >= 19) {
            chicken.frame = 0;
            chicken.animation_time = 0;
        }
    }
}

std::array<contracts::SpriteDraw, 9> Computer::draws() const {
    std::array<contracts::SpriteDraw, 9> result;
    result[0] = {"gfx/Computer", {0, 0, 640, 480}, {0, 0}};
    for (std::size_t index = 0; index < chickens_.size(); ++index) {
        const auto& chicken = chickens_[index];
        result[index + 1] = {
            chicken.direction == 0 ? "gfx/Chickright1" : "gfx/Chickleft1", {0, chicken.frame * 124, 165, 124}, {static_cast<int>(chicken.x), static_cast<int>(chicken.y)}};
    }

    return result;
}

} // namespace moorhuhn2::bonus
