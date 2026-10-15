#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace moorhuhn2::test {
inline void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

template <typename Operation> void require_throws(Operation&& operation, std::string_view message) {
    bool thrown{};
    try {
        operation();
    } catch (const std::exception&) {
        thrown = true;
    }

    require(thrown, message);
}
} // namespace moorhuhn2::test
