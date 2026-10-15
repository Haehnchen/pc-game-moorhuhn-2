#pragma once
#include "support/require.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace moorhuhn2::test {
template <class State, class Action> void reject_unchanged(State original, Action action, std::string_view message) {
    auto changed = original;
    try {
        action(changed);
    } catch (const std::invalid_argument&) {
        require(changed == original, std::string(message) + ": rejected action changed state");
        return;
    }
    throw std::runtime_error(std::string(message) + ": invalid action accepted");
}
} // namespace moorhuhn2::test
