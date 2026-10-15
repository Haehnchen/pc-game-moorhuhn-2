#include "ui/results_transition.hpp"

#include "contracts/time.hpp"
#include "ui/tiles.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::ui {
namespace {

std::array<ResultsTile, 768> grid(float velocity_y) {
    std::array<ResultsTile, 768> result{};
    for (std::size_t index = 0; index < result.size(); ++index) {
        result[index] = {static_cast<float>(index % 32 * 20), static_cast<float>(index / 32 * 20), velocity_y, 0, static_cast<int>(index)};
    }

    return result;
}

void validate_delta(float delta) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Results transition delta must be finite and nonnegative");
    }
}

float binary32(long double value) {
    if (!std::isfinite(value) || value < -std::numeric_limits<float>::max() || value > std::numeric_limits<float>::max()) {
        throw std::invalid_argument("Results transition update must fit binary32 state");
    }

    return static_cast<float>(value);
}

int coordinate(float value) {
    if (!std::isfinite(value) || static_cast<long double>(value) < std::numeric_limits<int>::min() || static_cast<long double>(value) > std::numeric_limits<int>::max()) {
        throw std::out_of_range("Results transition draw coordinates must fit signed integers");
    }

    return static_cast<int>(value);
}

} // namespace

ResultsTransition::ResultsTransition() : tiles_(grid(0.0F)) {}

void ResultsTransition::update_entry(float delta) {
    validate_delta(delta);
    if (exiting_) {
        throw std::logic_error("Results entry cannot update after exit begins");
    }

    const long double elapsed = delta;
    const float offset = binary32(row_offset_ - row_speed_ * elapsed * 100.0F);
    const long double speed = row_speed_ - elapsed * 5.0F;
    row_offset_ = offset <= 0 ? 0.0F : offset;
    row_speed_ = speed <= 1 ? 1.0F : binary32(speed);
}

void ResultsTransition::begin_exit() {
    if (exiting_) {
        return;
    }

    tiles_ = grid(1.0F);
    reveal_time_ = 0;
    next_column_ = 0;
    exiting_ = true;
}

bool ResultsTransition::update_exit(float delta, bool skip) {
    validate_delta(delta);
    if (!exiting_) {
        throw std::logic_error("Results exit must begin before its first update");
    }

    if (finished_) {
        return true;
    }

    const long double elapsed = delta;
    const float offset = binary32(row_offset_ - row_speed_ * elapsed * 100.0F);
    const long double speed = row_speed_ + elapsed * 10.0F;
    const float next_speed = speed >= 30 ? 30.0F : binary32(speed);
    const long double time = elapsed * 15.0F + reveal_time_;
    float next_time = binary32(time);
    auto next_tiles = tiles_;
    auto next_column = next_column_;
    // FST stores binary32 but leaves the extended sum for this strict test.
    if (time > 1.0F && next_column < 32) {
        next_time = binary32(static_cast<long double>(next_time) - 1.0F);
        for (std::size_t row = 0; row < 24; ++row) {
            next_tiles[row * 32 + next_column].state = 1;
        }
        ++next_column;
    }

    for (auto& tile : next_tiles) {
        if (tile.state > 0) {
            // Movement uses the old velocity; acceleration is stored afterward.
            tile.y = binary32(elapsed * tile.velocity_y * 100.0F + tile.y);
            tile.velocity_y = binary32(elapsed * 20.0F + tile.velocity_y);
        }
    }

    row_offset_ = offset;
    row_speed_ = next_speed;
    reveal_time_ = next_time;
    next_column_ = next_column;
    tiles_ = next_tiles;
    finished_ = skip || tiles_[31].y > 480.0F;

    return finished_;
}

std::vector<contracts::SpriteDraw> ResultsTransition::background_draws() const {
    std::vector<contracts::SpriteDraw> result;
    result.reserve(exiting_ ? 1536 : 768);
    if (exiting_) {
        // Round initialization resets the title underlay to this fixed grid.
        for (std::size_t reverse = tiles_.size(); reverse != 0; --reverse) {
            const auto index = reverse - 1;
            const int x = static_cast<int>(index % 32 * 20);
            const int y = static_cast<int>(index / 32 * 20);
            result.push_back(tile_draw("gfx/mainscreen", static_cast<int>(index), {x, y}));
        }
    }

    for (std::size_t reverse = tiles_.size(); reverse != 0; --reverse) {
        const auto& tile = tiles_[reverse - 1];
        result.push_back(tile_draw("gfx/scorescreen", tile.frame, {coordinate(tile.x), coordinate(tile.y)}));
    }

    return result;
}

} // namespace moorhuhn2::ui
