#include "ui/round_exit.hpp"

#include "contracts/time.hpp"
#include "ui/tiles.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::ui {
namespace {

float binary32(long double value) {
    if (!std::isfinite(value) || value < -std::numeric_limits<float>::max() || value > std::numeric_limits<float>::max()) {
        throw std::invalid_argument("Round exit update must fit binary32 state");
    }

    return static_cast<float>(value);
}

} // namespace

RoundExit::RoundExit() {
    for (std::size_t index = 0; index < tiles_.size(); ++index) {
        const float target = static_cast<float>(index % 32 * 20);
        const float x = target + (index / 32 % 2 == 0 ? -640.0F : 640.0F);
        tiles_[index] = {x, static_cast<float>(index / 32 * 20), 20.0F, target, 0, static_cast<int>(index)};
    }
}

bool RoundExit::update(float delta, bool skip) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Round exit delta must be finite and nonnegative");
    }

    if (finished_) {
        return true;
    }

    const long double elapsed = delta;
    const long double time = elapsed * 60.0F + reveal_time_;
    float next_time = binary32(time);
    auto next_tiles = tiles_;
    auto next_tile = next_tile_;
    // FST retains the extended sum; subtraction reloads the binary32 store.
    if (time > 1.0F) {
        next_time = binary32(static_cast<long double>(next_time) - 1.0F);
        for (int count = 0; count < 32; ++count) {
            if (next_tile >= next_tiles.size()) {
                next_tile = next_tiles.size() - 1;
            }

            next_tiles[next_tile].state = 1;
            ++next_tile;
        }
    }

    for (std::size_t index = 0; index < next_tiles.size(); ++index) {
        auto& tile = next_tiles[index];
        if (tile.state != 0) {
            const bool from_left = index / 32 % 2 == 0;
            const long double distance = tile.speed * elapsed * 100.0F;
            tile.x = binary32(from_left ? tile.x + distance : tile.x - distance);
            // Movement uses the old speed; the minimum compares the retained result.
            const long double speed = tile.speed - elapsed * 5.0F;
            tile.speed = speed < 3.0F ? 3.0F : binary32(speed);
            if (from_left ? tile.x >= tile.target_x : tile.x <= tile.target_x) {
                tile.x = tile.target_x;
                tile.state = 0;
            }
        }
    }

    reveal_time_ = next_time;
    next_tile_ = next_tile;
    tiles_ = next_tiles;
    finished_ = skip || tiles_.back().x == tiles_.back().target_x;

    return finished_;
}

std::array<contracts::SpriteDraw, 768> RoundExit::draws() const {
    std::array<contracts::SpriteDraw, 768> result;
    for (std::size_t index = 0; index < tiles_.size(); ++index) {
        const auto& tile = tiles_[index];
        result[index] = tile_draw("gfx/scorescreen", tile.frame, {static_cast<int>(tile.x), static_cast<int>(tile.y)});
    }

    return result;
}

} // namespace moorhuhn2::ui
