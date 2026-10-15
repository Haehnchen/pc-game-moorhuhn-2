#pragma once

#include "game/round.hpp"
#include "world/presentation.hpp"

namespace moorhuhn2::render {

// SDL-free frame composition shared by the application and pixel-hit tests.
// Scratch buffers persist across frames; neither composition method changes the round.
class RoundScene final {
public:
    void append(std::vector<contracts::SpriteDraw>& draws, const game::Round& round, const world::Presentation& landscape, contracts::ScreenPoint pointer);

private:
    std::vector<contracts::DepthDraw> scene_, layer_;
    std::vector<contracts::SpriteDraw> text_;
};

[[nodiscard]] std::vector<contracts::SpriteDraw> round_scene(const game::Round& round, const world::Presentation& landscape);

} // namespace moorhuhn2::render
