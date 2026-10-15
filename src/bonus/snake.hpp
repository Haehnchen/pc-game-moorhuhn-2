#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace moorhuhn2::bonus {

inline constexpr int snake_columns = 32;
inline constexpr int snake_rows = 25;
inline constexpr int snake_cells = snake_columns * snake_rows;

enum class SnakeDirection : std::uint8_t {
    up,
    right,
    down,
    left
};

struct SnakeInput {
    bool space{};
    bool cancel{};
    // Original queries held keys in this order; left wins simultaneous holds.
    bool up{};
    bool right{};
    bool down{};
    bool left{};
};

struct SnakeState {
    // Board coordinates, not screen pixels. Cells use original -1/0/1/8/9 values.
    std::array<std::int32_t, snake_cells> board{};
    std::array<int, snake_cells> body{};
    int length{3};
    SnakeDirection direction{SnakeDirection::left};
    int head_x{16}, head_y{12};
    std::uint32_t score{};
    float speed{1.0F}, accumulator{};
    bool playing{}, reset_pending{}, returned{};

    bool operator==(const SnakeState&) const = default;
};

struct SnakeUpdate {
    bool exit_to_title{};
    bool moved{};
    std::array<std::string_view, 2> sounds{};
    std::size_t sound_count{};
};

class Snake {
public:
    Snake();
    // One sampled original update. Entry and resets do not consume or reseed RNG.
    SnakeUpdate update(float delta, SnakeInput input, std::uint32_t& random_state);

    [[nodiscard]] const SnakeState& state() const noexcept {
        return state_;
    }

    [[nodiscard]] std::vector<contracts::SpriteDraw> draws() const;

private:
    void reset_board();
    void move(std::uint32_t& random_state, SnakeUpdate& result);
    bool place_food(std::uint32_t& random_state);
    void collide(SnakeUpdate& result);

    SnakeState state_{};
};

} // namespace moorhuhn2::bonus
