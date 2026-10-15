#include "ui/outro.hpp"

#include "contracts/random.hpp"
#include "contracts/time.hpp"
#include "ui/tiles.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::ui {
namespace {

int coordinate(float value) {
    if (!std::isfinite(value) || static_cast<long double>(value) < std::numeric_limits<int>::min() || static_cast<long double>(value) > std::numeric_limits<int>::max()) {
        throw std::out_of_range("Outro draw coordinates must fit signed integers");
    }

    return static_cast<int>(value);
}

} // namespace

Outro::Outro(const OutroTiles& tiles, std::uint32_t& random_state) : tiles_(tiles) {
    for (auto& tile : tiles_) {
        if (tile.state != 2) {
            tile.state = 0;
            tile.velocity_x = static_cast<float>(static_cast<long double>(static_cast<int>(contracts::next_random(random_state) % 20) - 10) * 0.1F);
            tile.velocity_y = static_cast<float>(static_cast<long double>(static_cast<int>(contracts::next_random(random_state) % 20) - 18) * 0.1F);
        }
    }
}

bool Outro::update(float delta, bool skip) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Outro delta must be finite and nonnegative");
    }

    if (finished_) {
        return true;
    }

    finished_ = skip;
    const long double elapsed = delta;
    if (holding_) {
        const long double time = phase_time_ + elapsed;
        phase_time_ = static_cast<float>(time);
        finished_ = finished_ || time > 5.0F;
    } else {
        const long double time = elapsed * 100.0F + phase_time_;
        phase_time_ = static_cast<float>(time);
        // FST stores binary32, but the strict test retains the x87 sum.
        if (time > 1.0F) {
            for (int count = 0; count < 8; ++count) {
                tiles_[next_tile_].state = 1;
                ++next_tile_;
                if (next_tile_ == tiles_.size()) {
                    next_tile_ = tiles_.size() - 1;
                    holding_ = true;
                }
            }

            phase_time_ = 0;
        }
    }

    for (auto& tile : tiles_) {
        if (tile.state > 0) {
            const long double acceleration = elapsed * 0.25F;
            tile.velocity_x = static_cast<float>(tile.velocity_x > 0 ? tile.velocity_x + acceleration : tile.velocity_x - acceleration);
            tile.velocity_y = static_cast<float>(elapsed + tile.velocity_y);
            // Movement reloads both new binary32 velocities before multiplication.
            tile.x = static_cast<float>(elapsed * tile.velocity_x * 100.0F + tile.x);
            tile.y = static_cast<float>(elapsed * tile.velocity_y * 100.0F + tile.y);
        }
    }

    const long double logo_time = logo_time_ + elapsed;
    const long double smooth = logo_time > 1 ? 1 : (3.0F - (logo_time + logo_time)) * logo_time * logo_time;
    logo_x_ = static_cast<float>(smooth * 100.0F + 585.0F);
    logo_time_ = logo_time >= 1 ? 1.0F : static_cast<float>(logo_time);

    return finished_;
}

std::array<contracts::SpriteDraw, 769> Outro::draws() const {
    std::array<contracts::SpriteDraw, 769> result;
    for (std::size_t index = 0; index < tiles_.size(); ++index) {
        const auto& tile = tiles_[index];
        const auto frame = tile.frame % 768;
        result[index] = tile_draw("gfx/mainscreen", frame, {coordinate(tile.x), coordinate(tile.y)});
    }

    result.back() = {"gfx/Witansmall", {0, 0, 80, 81}, {coordinate(logo_x_), 426}};
    return result;
}

} // namespace moorhuhn2::ui
