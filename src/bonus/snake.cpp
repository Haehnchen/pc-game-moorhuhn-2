#include "bonus/snake.hpp"

#include "contracts/random.hpp"
#include "contracts/time.hpp"
#include "ui/screens.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string>

namespace moorhuhn2::bonus {

Snake::Snake() {
    reset_board();
}

void Snake::reset_board() {
    for (int row = 0; row < snake_rows; ++row) {
        for (int column = 0; column < snake_columns; ++column) {
            state_.board[row * snake_columns + column] = row == 0 || row == snake_rows - 1 || column == 0 || column == snake_columns - 1 ? -1 : 0;
        }
    }

    state_.body.fill(0);
    state_.body[0] = 400;
    state_.body[1] = 401;
    state_.body[2] = 402;
    state_.board[400] = 9;
    state_.board[401] = 8;
    state_.board[402] = 8;
    state_.board[330] = 1;
    state_.length = 3;
    state_.head_x = 16;
    state_.head_y = 12;
    state_.direction = SnakeDirection::left;
    state_.speed = 1.0F;
    state_.accumulator = 0;
    state_.playing = false;
    state_.reset_pending = false;
}

bool Snake::place_food(std::uint32_t& random_state) {
    // The normal path has just cleared the old tail. Reject a full board instead
    // of inheriting an endless original rejection loop if that invariant fails.
    if (std::ranges::find(state_.board, 0) == state_.board.end()) {
        return false;
    }

    while (true) {
        const int column = static_cast<int>(contracts::next_random(random_state) % snake_columns);
        const int row = static_cast<int>(contracts::next_random(random_state) % snake_rows);
        const int cell = row * snake_columns + column;
        if (state_.board[cell] == 0) {
            state_.board[cell] = 1;

            return true;
        }
    }
}

void Snake::collide(SnakeUpdate& result) {
    state_.playing = false;
    state_.reset_pending = true;
    result.sounds[result.sound_count++] = "sfx/sfx_airplane_crash";
}

void Snake::move(std::uint32_t& random_state, SnakeUpdate& result) {
    result.moved = true;
    result.sounds[result.sound_count++] = "sfx/sfx_typename_click";
    ++state_.score;
    const int old_tail = state_.body[state_.length - 1];
    state_.board[old_tail] = 0;
    for (int index = state_.length - 1; index > 0; --index) {
        state_.body[index] = state_.body[index - 1];
    }

    switch (state_.direction) {
        case SnakeDirection::up:
            --state_.head_y;
            break;
        case SnakeDirection::right:
            ++state_.head_x;
            break;
        case SnakeDirection::down:
            ++state_.head_y;
            break;
        case SnakeDirection::left:
            --state_.head_x;
            break;
    }
    // Ordinary moves encounter a border cell before they could leave the array.
    if (state_.head_x < 0 || state_.head_x >= snake_columns || state_.head_y < 0 || state_.head_y >= snake_rows) {
        collide(result);
        return;
    }

    const int head = state_.head_y * snake_columns + state_.head_x;
    state_.body[0] = head;
    switch (state_.board[head]) {
        case 1:
            state_.score += 50U;
            if (state_.length == snake_cells) {
                collide(result);
                break;
            }

            result.sounds[result.sound_count++] = "sfx/sfx_50_point_score";
            state_.body[state_.length++] = old_tail;
            if (!place_food(random_state)) {
                // No source-reachable normal state can exhaust the board here.
                // The score sound already occupies the second one-shot slot.
                state_.playing = false;
                state_.reset_pending = true;
                break;
            }

            state_.speed = static_cast<float>(static_cast<long double>(state_.speed) + 0.25F);
            break;
        case -1:
        case 8:
        case 9:
            collide(result);
            break;
        default:
            break;
    }
    // Original deliberately omits the final body slot. On a growth update its
    // old-tail cell stays clear, or becomes food if rejection sampling chose it.
    for (int index = 0; index < state_.length - 1; ++index) {
        state_.board[state_.body[index]] = 8;
    }

    state_.board[head] = 9;
}

SnakeUpdate Snake::update(float delta, SnakeInput input, std::uint32_t& random_state) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Snake delta must be finite and nonnegative");
    }
    SnakeUpdate result;
    if (state_.returned) {
        result.exit_to_title = true;

        return result;
    }

    if (state_.reset_pending) {
        reset_board();
    }

    if (input.space) {
        state_.playing = true;
        state_.score = 0;
    }

    if (state_.playing) {
        if (input.up) {
            state_.direction = SnakeDirection::up;
        }

        if (input.right) {
            state_.direction = SnakeDirection::right;
        }

        if (input.down) {
            state_.direction = SnakeDirection::down;
        }

        if (input.left) {
            state_.direction = SnakeDirection::left;
        }

        const long double accumulated = static_cast<long double>(state_.speed) * delta + state_.accumulator;
        // FST rounds the stored timer, while FCOMP uses its retained x87 value.
        if (accumulated >= 1.0F) {
            state_.accumulator = 0;
            move(random_state, result);
        } else {
            state_.accumulator = static_cast<float>(accumulated);
        }
    }

    if (input.cancel) {
        state_.returned = true;
        result.exit_to_title = true;
    }

    return result;
}

std::vector<contracts::SpriteDraw> Snake::draws() const {
    std::vector<contracts::SpriteDraw> result;
    result.reserve(snake_cells + 48);
    result.push_back({"gfx/Making", {0, 0, 640, 480}, {0, 0}});
    result.push_back({"gfx/jennie", {0, 200, 125, 100}, {515, 327}});
    result.push_back({"gfx/kopf", {0, 0, 46, 29}, {594, 399}});
    for (int frame = 0; frame < 20; ++frame) {
        result.push_back({"gfx/loadingbar", {0, frame * 10, 267, 10}, {187, 140 + frame * 10}});
    }

    for (int row = 0; row < snake_rows; ++row) {
        for (int column = 0; column < snake_columns; ++column) {
            int frame = -1;
            switch (state_.board[row * snake_columns + column]) {
                case -1:
                    frame = 0;
                    break;
                case 9:
                    frame = 1;
                    break;
                case 8:
                    frame = 2;
                    break;
                case 1:
                    frame = 3;
                    break;
                default:
                    break;
            }

            if (frame >= 0) {
                result.push_back({"gfx/snake", {0, frame * 8, 8, 8}, {192 + column * 8, 140 + row * 8}});
            }
        }
    }
    ui::text(result, std::to_string(std::bit_cast<std::int32_t>(state_.score)), 300, 100);
    if (!state_.playing) {
        ui::text(result, "Press SPACE to start", 120, 360);
    }

    return result;
}

} // namespace moorhuhn2::bonus
