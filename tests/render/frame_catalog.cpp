#include "render/frame_catalog.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

using namespace moorhuhn2;

namespace {

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function> void rejects(Function function) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }

    throw std::runtime_error("Invalid frame request was accepted");
}

void frame_cells() {
    const std::array grids{render::FrameGrid{"gfx/M2BG01", 30, 1, 64, 480}, render::FrameGrid{"gfx/Chickleft1", 1, 20, 165, 124}};
    const render::FrameCatalog frames(grids);
    const auto wrapped = frames.frame_source("gfx/M2BG01", 32);
    check(wrapped.x == 128 && wrapped.width == 64, "Original frame modulo wraps scenery");
    check(frames.frame_source("gfx/Chickleft1", 19).y == 2356, "Stored chicken frame19 remains distinct from active0..18");
    rejects([&] {
        static_cast<void>(frames.frame_source("gfx/Chickleft1", -1));
    });
}

} // namespace

int main() {
    try {
        frame_cells();
        std::cout << "Frame grid storage and wrapping checks passed\n";
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';

        return 1;
    }
}
