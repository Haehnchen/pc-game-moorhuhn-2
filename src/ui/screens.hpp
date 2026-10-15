#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace moorhuhn2::ui {

enum class ExtrasAction {
    none,
    snake,
    making_of,
    back
};

class ExtrasMenu {
public:
    ExtrasAction update(int direction, bool accept, bool cancel, bool left_release, contracts::ScreenPoint pointer);
    [[nodiscard]] std::vector<contracts::SpriteDraw> draws(contracts::ScreenPoint pointer) const;

private:
    int selected_{};
    contracts::ScreenPoint previous_pointer_{-1, -1};
};

class RoundIntroduction {
public:
    RoundIntroduction();
    // Original computes the position before reducing the amplitude.
    bool update(float delta);

    [[nodiscard]] std::span<const float, 12> offsets() const {
        return offsets_;
    }

private:
    std::array<float, 12> amplitudes_{};
    std::array<float, 12> phases_{};
    std::array<float, 12> offsets_{};
};

[[nodiscard]] int glyph(unsigned char character);
void text(std::vector<contracts::SpriteDraw>& draws, std::string_view value, int x, int y);
void number(std::vector<contracts::SpriteDraw>& draws, int value, int x, int y, int minimum_width = 0);
void cursor(std::vector<contracts::SpriteDraw>& draws, contracts::ScreenPoint point, int frame = 0);
void hud(std::vector<contracts::SpriteDraw>& draws, int minutes, int seconds, int score);
[[nodiscard]] contracts::SpriteDraw screen(std::string_view image);

} // namespace moorhuhn2::ui
