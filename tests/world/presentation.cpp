#include "world/presentation.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    using moorhuhn2::world::Presentation;

    try {
        // Literal original camera products truncate before a binary32 store.
        const bool matches = Presentation::camera_offset(640, 3) == 895 && Presentation::camera_offset(640, 0) == 1151 && Presentation::camera_offset(1280, 3) == 1791
                             && Presentation::camera_offset(1280, 0) == 2303 && Presentation::camera_offset(624.1666259765625F, 4) == 748;

        if (!matches) {
            throw std::runtime_error("Original retained camera offsets differ");
        }

        try {
            static_cast<void>(Presentation::camera_offset(1281, 3));
        } catch (const std::invalid_argument&) {
            return 0;
        }

        throw std::runtime_error("Out-of-range camera was accepted");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
