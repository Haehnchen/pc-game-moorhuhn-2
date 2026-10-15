#pragma once

#include "contracts/display.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace moorhuhn2::contracts {

// These are physical input controls; their game meaning belongs to the consumer.
enum class InputControl : std::uint8_t {
    primary,
    secondary
};

enum class InputEdge : std::uint8_t {
    pressed,
    released
};

struct InputAction {
    InputControl control{};
    InputEdge edge{};

    bool operator==(const InputAction&) const = default;
};

struct InputFrame {
    std::optional<ScreenPoint> pointer;
    // Includes black margins at the nearest playfield edge for cursor motion and scrolling.
    // Button actions still require the unclamped pointer to be inside the playfield.
    std::optional<ScreenPoint> clamped_pointer;
    // Mouse button edges in platform arrival order; keys are read through the keyboard queue.
    std::vector<InputAction> actions;
    bool quit_requested{};
    bool focused{};

    bool operator==(const InputFrame&) const = default;
};

} // namespace moorhuhn2::contracts
