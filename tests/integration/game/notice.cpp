#include "world/notice.hpp"

#include "support/require.hpp"
#include "ui/screens.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace moorhuhn2;

using moorhuhn2::test::require;

struct Glyph {
    int cell;
    contracts::ScreenPoint destination;
};

struct NoticeCase {
    int type;
    float y;
    std::vector<Glyph> glyphs;
};

const std::array notice_cases{
    NoticeCase{3, 160.0F,
        {{50, {200, 160}}, {48, {220, 160}}, {39, {240, 160}}, {46, {260, 160}}, {42, {280, 160}}, {35, {300, 160}}, {38, {340, 160}}, {39, {360, 160}}, {50, {380, 160}},
            {9, {420, 160}}, {103, {440, 160}}}},
    NoticeCase{4, 160.0F,
        {{51, {170, 160}}, {44, {190, 160}}, {32, {210, 160}}, {35, {230, 160}}, {42, {250, 160}}, {39, {270, 160}}, {35, {290, 160}}, {52, {310, 160}}, {31, {330, 160}},
            {32, {350, 160}}, {42, {370, 160}}, {35, {390, 160}}, {9, {430, 160}}, {99, {450, 160}}, {98, {470, 160}}}},
    NoticeCase{5, 160.0F,
        {{43, {140, 160}}, {45, {160, 160}}, {45, {180, 160}}, {48, {200, 160}}, {38, {220, 160}}, {51, {240, 160}}, {38, {260, 160}}, {44, {280, 160}}, {43, {320, 160}},
            {31, {340, 160}}, {49, {360, 160}}, {50, {380, 160}}, {35, {400, 160}}, {48, {420, 160}}, {9, {460, 160}}, {99, {480, 160}}, {103, {500, 160}}}},
    NoticeCase{6, 160.0F,
        {{39, {150, 160}}, {0, {170, 160}}, {43, {190, 160}}, {44, {230, 160}}, {45, {250, 160}}, {50, {270, 160}}, {53, {310, 160}}, {45, {330, 160}}, {48, {350, 160}},
            {50, {370, 160}}, {38, {390, 160}}, {55, {410, 160}}, {9, {450, 160}}, {100, {470, 160}}, {98, {490, 160}}}},
    NoticeCase{100, 180.0F,
        {{32, {160, 180}}, {31, {180, 180}}, {42, {200, 180}}, {42, {220, 180}}, {45, {240, 180}}, {45, {260, 180}}, {44, {280, 180}}, {38, {320, 180}}, {35, {340, 180}},
            {48, {360, 180}}, {45, {380, 180}}, {9, {420, 180}}, {99, {440, 180}}, {98, {460, 180}}, {98, {480, 180}}}},
    NoticeCase{101, 180.0F,
        {{49, {170, 180}}, {46, {190, 180}}, {39, {210, 180}}, {34, {230, 180}}, {35, {250, 180}}, {48, {270, 180}}, {38, {310, 180}}, {35, {330, 180}}, {48, {350, 180}},
            {45, {370, 180}}, {9, {410, 180}}, {99, {430, 180}}, {103, {450, 180}}, {98, {470, 180}}}},
    NoticeCase{102, 180.0F,
        {{56, {170, 180}}, {35, {190, 180}}, {46, {210, 180}}, {46, {230, 180}}, {35, {250, 180}}, {42, {270, 180}}, {39, {290, 180}}, {44, {310, 180}}, {41, {350, 180}},
            {39, {370, 180}}, {42, {390, 180}}, {42, {410, 180}}, {35, {430, 180}}, {48, {450, 180}}}},
};

void original_glyph_commands() {
    for (const auto& expected : notice_cases) {
        const world::Notice value{true, true, 0, expected.y, expected.type};
        const auto text = world::notice_text(value);
        require(text.has_value(), "Source pixel word must have a recognized notice descriptor");
        std::vector<contracts::SpriteDraw> commands;
        ui::text(commands, text->text, text->x, text->y);
        require(commands.size() == expected.glyphs.size(), "Original notice must skip spaces while advancing their positions");
        for (std::size_t i = 0; i < commands.size(); ++i) {
            const auto& command = commands[i];
            const auto& glyph = expected.glyphs[i];
            require(command.image == "gfx/font" && command.source.x == 0 && command.source.y == glyph.cell * 50 && command.source.width == 30 && command.source.height == 50
                        && command.destination == glyph.destination && command.color_key && !command.mirror_x,
                "Original notice glyph cell, painter order, spacing, key or destination differs");
        }
    }
}

} // namespace

int main() {
    try {
        original_glyph_commands();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
