#include "ui/title.hpp"

#include "support/require.hpp"
#include "ui/screens.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

struct TileExpectation {
    std::size_t index{};
    std::uint32_t x_bits{};
    std::uint32_t y_bits{};
    std::uint32_t phase_bits{};
    std::uint32_t amplitude_bits{};
};

struct TitleExpectation {
    bool introducing{};
    int logo_x{};
    bool cursor_visible{};
    std::vector<TileExpectation> records{};
};

struct StepCase {
    std::uint32_t delta_bits{};
    bool space{};
    bool reenter{};
    std::string action{};
    TitleExpectation expected{};
};

struct SequenceCase {
    TitleExpectation initial{};
    std::vector<StepCase> steps{};
};

const std::array<SequenceCase, 4> sequences = {
    {{{true, 0, false,
          {{0, 0x00000000U, 0x00000000U, 0x40000000U, 0x44200000U}, {31, 0x00000000U, 0x00000000U, 0x40000000U, 0x44200000U},
              {32, 0x00000000U, 0x00000000U, 0x40000000U, 0x44228000U}, {736, 0x00000000U, 0x00000000U, 0x40000000U, 0x44598000U},
              {767, 0x00000000U, 0x00000000U, 0x40000000U, 0x44598000U}}},
         {{0x00000000U, false, false, "none",
              {true, 0, false,
                  {{0, 0x441f87f5U, 0x00000000U, 0x40000000U, 0x44200000U}, {31, 0x449d43faU, 0x00000000U, 0x40000000U, 0x44200000U},
                      {32, 0x44220615U, 0x41a00000U, 0x40000000U, 0x44228000U}, {736, 0x4458dcd1U, 0x43e60000U, 0x40000000U, 0x44598000U},
                      {767, 0x44b9ee68U, 0x43e60000U, 0x40000000U, 0x44598000U}}}},
             {0x3e000000U, false, false, "none",
                 {true, 0, false,
                     {{0, 0x441f87f5U, 0x00000000U, 0x40001cacU, 0x441a6000U}, {31, 0x449d43faU, 0x00000000U, 0x40001cacU, 0x441a6000U},
                         {32, 0x44220615U, 0x41a00000U, 0x40001cacU, 0x441ce000U}, {736, 0x4458dcd1U, 0x43e60000U, 0x40001cacU, 0x4453e000U},
                         {767, 0x44b9ee68U, 0x43e60000U, 0x40001cacU, 0x4453e000U}}}},
             {0x3f000000U, false, false, "none",
                 {true, 0, false,
                     {{0, 0x441a54eaU, 0x00000000U, 0x40008f5cU, 0x4403e000U}, {31, 0x449aaa75U, 0x00000000U, 0x40008f5cU, 0x4403e000U},
                         {32, 0x441cd4bcU, 0x41a00000U, 0x40008f5cU, 0x44066000U}, {736, 0x4453d0c8U, 0x43e60000U, 0x40008f5cU, 0x443d6000U},
                         {767, 0x44b76864U, 0x43e60000U, 0x40008f5cU, 0x443d6000U}}}},
             {0x00000000U, false, true, "none",
                 {true, 0, false,
                     {{0, 0x441f87f5U, 0x00000000U, 0x40000000U, 0x44200000U}, {31, 0x449d43faU, 0x00000000U, 0x40000000U, 0x44200000U},
                         {32, 0x44220615U, 0x41a00000U, 0x40000000U, 0x44228000U}, {736, 0x4458dcd1U, 0x43e60000U, 0x40000000U, 0x44598000U},
                         {767, 0x44b9ee68U, 0x43e60000U, 0x40000000U, 0x44598000U}}}}}},
        {{true, 0, false,
             {{0, 0x00000000U, 0x00000000U, 0x40000000U, 0x44200000U}, {31, 0x00000000U, 0x00000000U, 0x40000000U, 0x44200000U},
                 {32, 0x00000000U, 0x00000000U, 0x40000000U, 0x44228000U}, {736, 0x00000000U, 0x00000000U, 0x40000000U, 0x44598000U},
                 {767, 0x00000000U, 0x00000000U, 0x40000000U, 0x44598000U}}},
            {{0x3e000000U, true, false, "none",
                 {false, 680, true,
                     {{0, 0x00000000U, 0x00000000U, 0x40001cacU, 0x441a6000U}, {31, 0x441b0000U, 0x00000000U, 0x40001cacU, 0x441a6000U},
                         {32, 0x00000000U, 0x41a00000U, 0x40001cacU, 0x441ce000U}, {736, 0x00000000U, 0x43e60000U, 0x40001cacU, 0x4453e000U},
                         {767, 0x441b0000U, 0x43e60000U, 0x40001cacU, 0x4453e000U}}}},
                {0x3f000000U, false, false, "none",
                    {false, 616, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40001cacU, 0x441a6000U}, {31, 0x441b0000U, 0x00000000U, 0x40001cacU, 0x441a6000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40001cacU, 0x441ce000U}, {736, 0x00000000U, 0x43e60000U, 0x40001cacU, 0x4453e000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40001cacU, 0x4453e000U}}}},
                {0x00000000U, false, true, "none",
                    {false, 685, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40001cacU, 0x441a6000U}, {31, 0x441b0000U, 0x00000000U, 0x40001cacU, 0x441a6000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40001cacU, 0x441ce000U}, {736, 0x00000000U, 0x43e60000U, 0x40001cacU, 0x4453e000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40001cacU, 0x4453e000U}}}},
                {0x3f800000U, false, false, "none",
                    {false, 585, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40001cacU, 0x441a6000U}, {31, 0x441b0000U, 0x00000000U, 0x40001cacU, 0x441a6000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40001cacU, 0x441ce000U}, {736, 0x00000000U, 0x43e60000U, 0x40001cacU, 0x4453e000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40001cacU, 0x4453e000U}}}},
                {0x00000000U, true, false, "start",
                    {false, 585, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40001cacU, 0x441a6000U}, {31, 0x441b0000U, 0x00000000U, 0x40001cacU, 0x441a6000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40001cacU, 0x441ce000U}, {736, 0x00000000U, 0x43e60000U, 0x40001cacU, 0x4453e000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40001cacU, 0x4453e000U}}}}}},
        {{true, 0, false,
             {{0, 0x00000000U, 0x00000000U, 0x40000000U, 0x44200000U}, {31, 0x00000000U, 0x00000000U, 0x40000000U, 0x44200000U},
                 {32, 0x00000000U, 0x00000000U, 0x40000000U, 0x44228000U}, {736, 0x00000000U, 0x00000000U, 0x40000000U, 0x44598000U},
                 {767, 0x00000000U, 0x00000000U, 0x40000000U, 0x44598000U}}},
            {{0x3f800000U, false, false, "none",
                 {true, 0, false,
                     {{0, 0x441f87f5U, 0x00000000U, 0x4000e560U, 0x43e60000U}, {31, 0x449d43faU, 0x00000000U, 0x4000e560U, 0x43e60000U},
                         {32, 0x44220615U, 0x41a00000U, 0x4000e560U, 0x43eb0000U}, {736, 0x4458dcd1U, 0x43e60000U, 0x4000e560U, 0x442c8000U},
                         {767, 0x44b9ee68U, 0x43e60000U, 0x4000e560U, 0x442c8000U}}}},
                {0x3f800000U, false, false, "none",
                    {true, 0, false,
                        {{0, 0x43ac11d8U, 0x00000000U, 0x4001cac0U, 0x438c0000U}, {31, 0x447108ecU, 0x00000000U, 0x4001cac0U, 0x438c0000U},
                            {32, 0x43afcf72U, 0x41a00000U, 0x4001cac0U, 0x43910000U}, {736, 0x44010d62U, 0x43e60000U, 0x4001cac0U, 0x43ff0000U},
                            {767, 0x448e06b1U, 0x43e60000U, 0x4001cac0U, 0x43ff0000U}}}},
                {0x3f800000U, false, false, "none",
                    {true, 0, false,
                        {{0, 0x41410808U, 0x00000000U, 0x4002b020U, 0x42c80000U}, {31, 0x441e0420U, 0x00000000U, 0x4002b020U, 0x42c80000U},
                            {32, 0x4147ece4U, 0x41a00000U, 0x4002b020U, 0x42dc0000U}, {736, 0x41afcbe3U, 0x43e60000U, 0x4002b020U, 0x43a50000U},
                            {767, 0x44207e5fU, 0x43e60000U, 0x4002b020U, 0x43a50000U}}}},
                {0x3f800000U, false, false, "none",
                    {true, 0, false,
                        {{0, 0xc289a503U, 0x00000000U, 0x40039580U, 0x00000000U}, {31, 0x4409cb60U, 0x00000000U, 0x40039580U, 0x00000000U},
                            {32, 0xc29768b6U, 0x41a00000U, 0x40039580U, 0x00000000U}, {736, 0xc3631d11U, 0x43e60000U, 0x40039580U, 0x43160000U},
                            {767, 0x43c47177U, 0x43e60000U, 0x40039580U, 0x43160000U}}}},
                {0x3f555555U, false, false, "none",
                    {true, 0, false,
                        {{0, 0x00000000U, 0x00000000U, 0x400454a6U, 0x00000000U}, {31, 0x441b0000U, 0x00000000U, 0x400454a6U, 0x00000000U},
                            {32, 0x00000000U, 0x41a00000U, 0x400454a6U, 0x00000000U}, {736, 0xc315fe39U, 0x43e60000U, 0x400454a6U, 0x36700000U},
                            {767, 0x43eb00e4U, 0x43e60000U, 0x400454a6U, 0x36700000U}}}},
                {0x3e000000U, false, false, "none",
                    {false, 680, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40047152U, 0x00000000U}, {31, 0x441b0000U, 0x00000000U, 0x40047152U, 0x00000000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40047152U, 0x00000000U}, {736, 0xb63dc4fdU, 0x43e60000U, 0x40047152U, 0x00000000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40047152U, 0x00000000U}}}},
                {0x00000000U, false, false, "none",
                    {false, 680, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40047152U, 0x00000000U}, {31, 0x441b0000U, 0x00000000U, 0x40047152U, 0x00000000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40047152U, 0x00000000U}, {736, 0xb63dc4fdU, 0x43e60000U, 0x40047152U, 0x00000000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40047152U, 0x00000000U}}}},
                {0x00000000U, true, false, "start",
                    {false, 680, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40047152U, 0x00000000U}, {31, 0x441b0000U, 0x00000000U, 0x40047152U, 0x00000000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40047152U, 0x00000000U}, {736, 0xb63dc4fdU, 0x43e60000U, 0x40047152U, 0x00000000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40047152U, 0x00000000U}}}}}},
        {{true, 0, false,
             {{0, 0x00000000U, 0x00000000U, 0x40000000U, 0x44200000U}, {31, 0x00000000U, 0x00000000U, 0x40000000U, 0x44200000U},
                 {32, 0x00000000U, 0x00000000U, 0x40000000U, 0x44228000U}, {736, 0x00000000U, 0x00000000U, 0x40000000U, 0x44598000U},
                 {767, 0x00000000U, 0x00000000U, 0x40000000U, 0x44598000U}}},
            {{0x3f800000U, false, false, "none",
                 {true, 0, false,
                     {{0, 0x441f87f5U, 0x00000000U, 0x4000e560U, 0x43e60000U}, {31, 0x449d43faU, 0x00000000U, 0x4000e560U, 0x43e60000U},
                         {32, 0x44220615U, 0x41a00000U, 0x4000e560U, 0x43eb0000U}, {736, 0x4458dcd1U, 0x43e60000U, 0x4000e560U, 0x442c8000U},
                         {767, 0x44b9ee68U, 0x43e60000U, 0x4000e560U, 0x442c8000U}}}},
                {0x3f800000U, false, false, "none",
                    {true, 0, false,
                        {{0, 0x43ac11d8U, 0x00000000U, 0x4001cac0U, 0x438c0000U}, {31, 0x447108ecU, 0x00000000U, 0x4001cac0U, 0x438c0000U},
                            {32, 0x43afcf72U, 0x41a00000U, 0x4001cac0U, 0x43910000U}, {736, 0x44010d62U, 0x43e60000U, 0x4001cac0U, 0x43ff0000U},
                            {767, 0x448e06b1U, 0x43e60000U, 0x4001cac0U, 0x43ff0000U}}}},
                {0x3f800000U, false, false, "none",
                    {true, 0, false,
                        {{0, 0x41410808U, 0x00000000U, 0x4002b020U, 0x42c80000U}, {31, 0x441e0420U, 0x00000000U, 0x4002b020U, 0x42c80000U},
                            {32, 0x4147ece4U, 0x41a00000U, 0x4002b020U, 0x42dc0000U}, {736, 0x41afcbe3U, 0x43e60000U, 0x4002b020U, 0x43a50000U},
                            {767, 0x44207e5fU, 0x43e60000U, 0x4002b020U, 0x43a50000U}}}},
                {0x3f800000U, false, false, "none",
                    {true, 0, false,
                        {{0, 0xc289a503U, 0x00000000U, 0x40039580U, 0x00000000U}, {31, 0x4409cb60U, 0x00000000U, 0x40039580U, 0x00000000U},
                            {32, 0xc29768b6U, 0x41a00000U, 0x40039580U, 0x00000000U}, {736, 0xc3631d11U, 0x43e60000U, 0x40039580U, 0x43160000U},
                            {767, 0x43c47177U, 0x43e60000U, 0x40039580U, 0x43160000U}}}},
                {0x3f555555U, false, false, "none",
                    {true, 0, false,
                        {{0, 0x00000000U, 0x00000000U, 0x400454a6U, 0x00000000U}, {31, 0x441b0000U, 0x00000000U, 0x400454a6U, 0x00000000U},
                            {32, 0x00000000U, 0x41a00000U, 0x400454a6U, 0x00000000U}, {736, 0xc315fe39U, 0x43e60000U, 0x400454a6U, 0x36700000U},
                            {767, 0x43eb00e4U, 0x43e60000U, 0x400454a6U, 0x36700000U}}}},
                {0x3e000000U, true, false, "none",
                    {false, 680, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40047152U, 0x00000000U}, {31, 0x441b0000U, 0x00000000U, 0x40047152U, 0x00000000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40047152U, 0x00000000U}, {736, 0x00000000U, 0x43e60000U, 0x40047152U, 0x00000000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40047152U, 0x00000000U}}}},
                {0x00000000U, true, false, "start",
                    {false, 680, true,
                        {{0, 0x00000000U, 0x00000000U, 0x40047152U, 0x00000000U}, {31, 0x441b0000U, 0x00000000U, 0x40047152U, 0x00000000U},
                            {32, 0x00000000U, 0x41a00000U, 0x40047152U, 0x00000000U}, {736, 0x00000000U, 0x43e60000U, 0x40047152U, 0x00000000U},
                            {767, 0x441b0000U, 0x43e60000U, 0x40047152U, 0x00000000U}}}}}}}};

using moorhuhn2::contracts::ScreenPoint;
using moorhuhn2::contracts::SpriteDraw;
using moorhuhn2::ui::Title;
using moorhuhn2::ui::TitleAction;
using moorhuhn2::ui::TitleTile;

using moorhuhn2::test::require;

const SpriteDraw& find(const std::vector<SpriteDraw>& draws, std::string_view image) {
    const auto found = std::ranges::find(draws, image, &SpriteDraw::image);
    require(found != draws.end(), "Expected title draw is absent");

    return *found;
}

bool contains(const std::vector<SpriteDraw>& draws, std::string_view image) {
    return std::ranges::find(draws, image, &SpriteDraw::image) != draws.end();
}

bool same_records(std::span<const TitleTile> actual, std::span<const TitleTile> expected) {
    const auto fields = [](const auto& tile) {
        return std::tie(tile.x, tile.y, tile.target_x, tile.target_y, tile.motion_x, tile.motion_y, tile.state, tile.frame, tile.amplitude, tile.initial_amplitude);
    };
    return std::ranges::equal(actual, expected, {}, fields, fields);
}

void check(const Title& title, const TitleExpectation& expected) {
    require(title.introducing() == expected.introducing, "Introduction flag differs from original outcomes");
    const auto tiles = title.records();
    for (const auto& record : expected.records) {
        const auto index = record.index;
        const auto& tile = tiles[index];
        require(std::bit_cast<std::uint32_t>(tile.x) == record.x_bits, "Source sine/old amplitude X differs");
        require(std::bit_cast<std::uint32_t>(tile.y) == record.y_bits, "Source target Y differs");
        require(std::bit_cast<std::uint32_t>(tile.motion_x) == record.phase_bits, "Source binary32 phase store differs");
        require(std::bit_cast<std::uint32_t>(tile.amplitude) == record.amplitude_bits, "Source binary32 amplitude store differs");
        require(tile.frame == static_cast<std::int32_t>(index) && tile.state == 0 && tile.target_x == static_cast<float>(index % 32 * 20)
                    && tile.target_y == static_cast<float>(index / 32 * 20) && tile.motion_y == 0 && tile.initial_amplitude == 640,
            "Original record fields must survive ordinary animation");
    }

    const auto draws = title.draws({320, 240});
    const bool introducing = title.introducing();
    require(contains(draws, "gfx/loading") == introducing, "Only ordinary introduction draws loading art");
    require(contains(draws, "gfx/mousecursor") == expected.cursor_visible, "Original cursor is hidden during introduction");
    require(contains(draws, "gfx/Witansmall") == !introducing, "Branding visibility differs");
    const auto first = introducing ? 1U : 0U;
    require(draws[first].image == "gfx/mainscreen" && draws[first].source.x == 620 && draws[first].source.y == 460, "Reverse record painter order must start at tile767");
    require(
        draws[first + 767].image == "gfx/mainscreen" && draws[first + 767].source.x == 0 && draws[first + 767].source.y == 0, "Reverse record painter order must finish at tile0");
    if (!introducing) {
        require(draws[768].image == "gfx/Witansmall" && draws[768].destination == ScreenPoint{expected.logo_x, 426}, "Branding must use the same update's retained x87 tween");
        require(draws[769].image == "gfx/font" && draws[769].destination == ScreenPoint{480, 375}, "Authorized Extras must remain visible");
        require(draws.back().image == "gfx/mousecursor", "Cursor follows title/brand/authorized Extras");
    }

    const auto outro = title.outro_tiles();
    for (std::size_t index = 0; index < tiles.size(); ++index) {
        require(outro[index].x == tiles[index].x && outro[index].y == tiles[index].y && outro[index].velocity_x == tiles[index].motion_x
                    && outro[index].velocity_y == tiles[index].motion_y && outro[index].state == tiles[index].state && outro[index].frame == tiles[index].frame,
            "Outro handoff must retain record poses, motion, states and frames");
    }
}

void check_vectors() {
    for (const auto& sequence : sequences) {
        Title title;
        check(title, sequence.initial);
        for (const auto& step : sequence.steps) {
            if (step.reenter) {
                title.reenter();
            }

            const auto delta = std::bit_cast<float>(step.delta_bits);
            const auto action = title.update(delta, step.space, false, false, {320, 240});
            require(action == (step.action == "start" ? TitleAction::start : TitleAction::none), "Concurrent natural completion activation remains in the settle branch");
            check(title, step.expected);
        }
    }
}

void check_input_order() {
    Title title;
    require(title.draws({40, 405}).size() == 769, "Before first update introduction draws loading plus 768 tile records, without a cursor");
    require(
        title.update(0.125F, false, true, true, {40, 405}) == TitleAction::extra && title.introducing(), "Computer release overrides Escape and consumes release without settling");
    const std::vector<TitleTile> interrupted(title.records().begin(), title.records().end());
    title.reenter();
    require(!same_records(title.records(), interrupted) && title.records()[767].x == 0 && title.records()[767].amplitude == 870,
        "Unfinished reentry resets records rather than continuing the old animation");
    require(title.update(0.125F, false, true, true, {320, 240}) == TitleAction::quit && !title.introducing(),
        "Intro activation snaps but cannot replace Escape with a start in the same update");
    require(find(title.draws({320, 240}), "gfx/Witansmall").destination.x == 680, "Forced completion updates branding in the same frame");
    require(title.update(0, false, true, true, {320, 240}) == TitleAction::start, "Settled release overrides Escape");
    require(title.update(0, true, false, true, {285, 450}) == TitleAction::start, "Space still starts after a hotspot consumes its simultaneous mouse release");
    require(title.update(0, false, false, true, {285, 450}) == TitleAction::disclaimer, "Settled normal info hotspot remains reachable");
    require(title.update(0, false, false, true, {500, 390}) == TitleAction::extras_menu, "Authorized Extras consumes release");
    require(find(title.draws({500, 390}), "gfx/mousecursor").source.y == 32, "Authorized Extras retains its active cursor");
    require(title.update(0, false, false, true, {284, 450}) == TitleAction::start, "Outside info rectangle starts normally");
    require(title.update(0, false, false, true, {28, 391}) == TitleAction::extra, "Computer includes its lower boundary");
    require(title.update(0, false, false, true, {54, 418}) == TitleAction::extra, "Computer includes its upper boundary");
    require(title.update(0, false, false, true, {55, 418}) == TitleAction::start, "Computer excludes the next pixel");
    require(title.update(0, false, false, true, {620, 425}) == TitleAction::start, "Outside Extras starts normally");
}

void check_invalid_delta() {
    Title title;
    title.update(0.25F, true, false, false, {320, 240});
    const std::vector<TitleTile> previous(title.records().begin(), title.records().end());
    const auto logo_x = find(title.draws({320, 240}), "gfx/Witansmall").destination.x;
    for (const auto value : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected = false;
        try {
            title.update(value, true, false, false, {320, 240});
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && !title.introducing() && same_records(title.records(), previous) && find(title.draws({320, 240}), "gfx/Witansmall").destination.x == logo_x,
            "Invalid delta must fail before mutating tile records or branding");
    }
}

void check_hunting_introduction() {
    Title title;
    title.update(1.0F, true, false, false, {320, 240});
    require(title.logo_x() == 585.0F, "Settled title brand starts at the original anchor");
    const std::vector<TitleTile> retained(title.records().begin(), title.records().end());
    // Source 0x40bc61--0x40bcca: retained tween + delta, then cubic and FSTP.
    for (const auto& [delta, expected] : std::array{std::pair{0.125F, 589.296875F}, std::pair{0.375F, 635.0F}, std::pair{0.5F, 685.0F}, std::pair{0.125F, 685.0F}}) {
        title.update_introduction(delta);
        require(title.logo_x() == expected && same_records(title.records(), retained), "Introduction moves branding outward without changing retained tiles");
    }

    const auto draws = title.introduction_draws();
    require(draws.size() == 769 && draws.front().source.x == 0 && draws.front().source.y == 0 && draws.front().source.width == 20 && draws.front().source.height == 20
                && draws[767].source.x == 620 && draws[767].source.y == 460 && draws[767].source.width == 20 && draws[767].source.height == 20,
        "Source introduction paints tiles forward from 0 to 767");
    require(draws.back().image == "gfx/Witansmall" && draws.back().destination == ScreenPoint{685, 426} && !contains(draws, "gfx/mousecursor") && !contains(draws, "gfx/font")
                && !contains(draws, "gfx/loading"),
        "Introduction base is retained tiles then branding without title input overlays");
    Title early;
    early.update(0.5F, true, false, false, {320, 240});
    early.update_introduction(0.125F);
    require(early.logo_x() == 653.359375F, "Early round entry continues the existing half-finished tween");
    for (const auto delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected{};
        try {
            early.update_introduction(delta);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && early.logo_x() == 653.359375F, "Invalid introduction delta must not mutate branding");
    }
}
} // namespace

int main() {
    try {
        check_vectors();
        check_input_order();
        check_invalid_delta();
        check_hunting_introduction();
        std::cout << "Title source outcomes passed: intro/settle/reentry, draw order, input/branding and outro handoff\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
