#pragma once

#include "contracts/input.hpp"
#include "platform/display.hpp"
#include "platform/keyboard.hpp"

namespace moorhuhn2::platform {

class InputAdapter {
public:
    explicit InputAdapter(Display& display);
    [[nodiscard]] contracts::InputFrame poll();
    [[nodiscard]] std::uint16_t take_key();
    void clear_keys();
    [[nodiscard]] bool key_held(std::uint16_t key) const;

private:
    Display& display_;
    Keyboard keyboard_;
    bool focused_{};
    std::optional<Coordinate> window_pointer_;
};

} // namespace moorhuhn2::platform
