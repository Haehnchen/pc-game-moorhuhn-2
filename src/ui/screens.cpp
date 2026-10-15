#include "ui/screens.hpp"

#include "contracts/angle.hpp"
#include "contracts/time.hpp"
#include "ui/hotspot.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::ui {
namespace {
constexpr std::array extras_choices{Hotspot{180, 175, 460, 225}, Hotspot{180, 245, 460, 295}, Hotspot{180, 315, 460, 365}};
} // namespace

ExtrasAction ExtrasMenu::update(int direction, bool accept, bool cancel, bool left_release, contracts::ScreenPoint pointer) {
    if (cancel) {
        return ExtrasAction::back;
    }

    if (direction != 0) {
        selected_ = (selected_ + (direction > 0 ? 1 : 2)) % 3;
    }
    int hovered = -1;
    for (int i = 0; i < 3; ++i) {
        if (extras_choices[static_cast<std::size_t>(i)].contains(pointer)) {
            hovered = i;
        }
    }

    if (hovered >= 0 && (pointer != previous_pointer_ || left_release)) {
        selected_ = hovered;
    }

    previous_pointer_ = pointer;
    if (accept || (left_release && hovered >= 0)) {
        return std::array{ExtrasAction::snake, ExtrasAction::making_of, ExtrasAction::back}[static_cast<std::size_t>(selected_)];
    }

    return ExtrasAction::none;
}

std::vector<contracts::SpriteDraw> ExtrasMenu::draws(contracts::ScreenPoint pointer) const {
    std::vector<contracts::SpriteDraw> result;
    text(result, "EXTRAS", 255, 45);
    text(result, "Snake", 265, 175);
    text(result, "Making Of", 225, 245);
    text(result, "Zurueck", 245, 315);
    text(result, ">", 155, 175 + selected_ * 70);
    cursor(result, pointer);

    return result;
}

RoundIntroduction::RoundIntroduction() {
    for (std::size_t i = 0; i < amplitudes_.size(); ++i) {
        amplitudes_[i] = 480.0F + 10.0F * static_cast<float>(i);
        phases_[i] = 0.25F;
    }
}

bool RoundIntroduction::update(float delta) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Round introduction delta must be finite and nonnegative");
    }

    for (std::size_t i = 0; i < amplitudes_.size(); ++i) {
        offsets_[i] = static_cast<float>(std::sin(static_cast<long double>(phases_[i]) * contracts::degrees_per_radian) * amplitudes_[i]);
        phases_[i] = static_cast<float>(static_cast<long double>(phases_[i]) + static_cast<long double>(delta) * 0.02500000037252903L);
        amplitudes_[i] = std::max(0.0F, static_cast<float>(static_cast<long double>(amplitudes_[i]) - static_cast<long double>(delta) * 200.0L));
    }

    return amplitudes_.back() == 0.0F;
}

int glyph(unsigned char character) {
    if (character >= 'A' && character <= 'Z') {
        return character - 'A' + 31;
    }

    if (character >= 'a' && character <= 'z') {
        return character - 'a' + 57;
    }

    if (character >= '0' && character <= '9') {
        return character - '0' + 98;
    }

    switch (character) {
        case '"':
            return 1;
        case '#':
            return 2;
        case '%':
            return 3;
        case '&':
            return 4;
        case '`':
            return 5;
        case '(':
            return 6;
        case ')':
            return 7;
        case '*':
        case '.':
            return 8;
        case '+':
            return 9;
        case '^':
            return 10;
        case '-':
            return 97;
        case '$':
            return 12;
        case '/':
            return 13;
        case ':':
            return 96;
        case ';':
            return 25;
        case '<':
            return 26;
        case '=':
            return 27;
        case '>':
            return 28;
        case '?':
            return 29;
        case '@':
            return 30;
        default:
            return 0;
    }
}

void text(std::vector<contracts::SpriteDraw>& draws, std::string_view value, int x, int y) {
    for (const unsigned char character : value) {
        if (character != ' ') {
            draws.push_back({"gfx/font", {0, glyph(character) * 50, 30, 50}, {x, y}});
        }
        x += 20;
    }
}

void number(std::vector<contracts::SpriteDraw>& draws, int value, int x, int y, int minimum_width) {
    std::array<char, std::numeric_limits<int>::digits10 + 3> buffer;
    const auto converted = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    const std::string_view digits(buffer.data(), converted.ptr);
    text(draws, digits, x + std::max(0, minimum_width - static_cast<int>(digits.size())) * 20, y);
}

void cursor(std::vector<contracts::SpriteDraw>& draws, contracts::ScreenPoint point, int frame) {
    draws.push_back({"gfx/mousecursor", {0, frame * 32, 32, 32}, point});
}

void hud(std::vector<contracts::SpriteDraw>& draws, int minutes, int seconds, int score) {
    std::array<char, 48> buffer{};
    number(draws, score, 530, 0, 5);
    std::snprintf(buffer.data(), buffer.size(), "%2i:%02i", minutes, seconds);
    text(draws, buffer.data(), -10, 0);
}

contracts::SpriteDraw screen(std::string_view image) {
    return {image, {0, 0, 640, 480}, {0, 0}};
}
} // namespace moorhuhn2::ui
