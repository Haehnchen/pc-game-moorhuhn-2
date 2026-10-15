#include "ui/screens.hpp"

#include "support/require.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using moorhuhn2::test::require;

} // namespace

int main() {
    try {
        using namespace moorhuhn2;
        ui::ExtrasMenu extras;
        require(extras.update(0, false, false, false, {320, 195}) == ui::ExtrasAction::none, "Hover must not activate Snake");
        require(extras.update(1, true, false, false, {320, 195}) == ui::ExtrasAction::making_of, "Keyboard selection survives a stationary mouse");
        require(extras.update(1, true, false, false, {320, 195}) == ui::ExtrasAction::back, "Down and Enter select return after Making-of");
        require(extras.update(1, true, false, false, {320, 195}) == ui::ExtrasAction::snake, "Menu wraps downwards from return to Snake");
        require(extras.update(-1, true, false, false, {320, 195}) == ui::ExtrasAction::back, "Menu wraps upwards from Snake to return");
        require(extras.update(-1, true, false, false, {320, 195}) == ui::ExtrasAction::making_of, "Up and Enter select Making-of from return");
        require(extras.update(-1, true, false, false, {320, 195}) == ui::ExtrasAction::snake, "Up and Enter select Snake");
        require(extras.update(0, false, false, true, {320, 195}) == ui::ExtrasAction::snake, "Snake keeps its original clickable row");
        require(extras.update(0, false, false, true, {320, 260}) == ui::ExtrasAction::making_of, "Making-of is clickable");
        require(extras.update(0, false, false, true, {320, 335}) == ui::ExtrasAction::back, "Return is clickable below Making-of");
        require(extras.update(0, false, false, true, {20, 20}) == ui::ExtrasAction::none, "Outside click must not activate current selection");
        require(extras.update(0, false, true, false, {320, 260}) == ui::ExtrasAction::back, "Escape returns to title");
        ui::RoundIntroduction introduction;
        require(!introduction.update(2.94F), "Introduction final amplitude 590 requires 2.95 seconds");
        require(introduction.update(0.02F), "Introduction completion is final amplitude reaching zero");
        for (const float delta : {-1.0F, std::numeric_limits<float>::quiet_NaN()}) {
            ui::RoundIntroduction invalid;
            bool rejected = false;
            try {
                static_cast<void>(invalid.update(delta));
            } catch (const std::invalid_argument&) {
                rejected = true;
            }

            require(rejected && invalid.offsets()[0] == 0, "Invalid introduction deltas are rejected before changing offsets");
        }
        require(ui::glyph('A') == 31 && ui::glyph('z') == 82 && ui::glyph('0') == 98 && ui::glyph('9') == 107 && ui::glyph(':') == 96, "Original font lookup cells");
        std::vector<contracts::SpriteDraw> hud;
        ui::hud(hud, 1, 30, 25);
        require(hud[0].destination.x == 590 && hud[0].source.y == 5000, "Score 25 right aligned at x 590, digit 2 cell 100");
        require(hud[2].destination.x == 10 && hud[3].source.y == 4800, "Timer blank spacing and colon cell 96");

        hud.clear();
        ui::number(hud, std::numeric_limits<int>::min(), 0, 10, 5);
        require(hud.size() == 11 && hud.front().source.y == 4850 && hud.back().source.y == 5300 && hud.back().destination.x == 200,
            "Minimum signed score retains its minus sign and all ten digits");
        hud.clear();
        ui::number(hud, -25, 0, 10, 5);
        require(hud.size() == 3 && hud.front().destination.x == 40 && hud.back().destination.x == 80, "Negative score padding includes its sign");

        std::cout << "Menu, introduction, font and HUD checks passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
