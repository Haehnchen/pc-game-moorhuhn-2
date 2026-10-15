#include "ui/outro.hpp"

#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

using moorhuhn2::ui::Outro;
using moorhuhn2::ui::OutroTile;
using moorhuhn2::ui::OutroTiles;

using moorhuhn2::test::require;

std::uint32_t bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

OutroTiles grid() {
    OutroTiles result;
    for (std::size_t index = 0; index < result.size(); ++index) {
        result[index] = {static_cast<float>((index % 32) * 20), static_cast<float>((index / 32) * 20), 0, 0, 0, static_cast<int>(index)};
    }

    return result;
}

OutroTiles snapshot(const Outro& outro) {
    OutroTiles result;
    std::ranges::copy(outro.records(), result.begin());

    return result;
}

std::size_t active(const Outro& outro) {
    return static_cast<std::size_t>(std::ranges::count_if(outro.records(), [](const OutroTile& tile) {
        return tile.state > 0;
    }));
}

void entry_and_random() {
    auto tiles = grid();
    for (std::size_t index = 0; index < tiles.size(); ++index) {
        tiles[index].x += 0.75F;
        tiles[index].y -= 0.25F;
        tiles[index].frame = static_cast<int>(767 - index);
        tiles[index].state = index % 2 == 0 ? -1 : 1;
        tiles[index].velocity_x = 7.5F;
        tiles[index].velocity_y = -0.25F;
    }

    tiles[0].state = 2;
    tiles[3].state = 2;
    tiles[767].state = 2;
    auto random_state = std::uint32_t{1};
    const Outro outro(tiles, random_state);
    require(random_state == 0xc2c729dbU, "State2 records must skip both draws in the retained row-major CRT stream");
    for (std::size_t index = 0; index < tiles.size(); ++index) {
        const auto& actual = outro.records()[index];
        require(actual.x == tiles[index].x && actual.y == tiles[index].y && actual.frame == tiles[index].frame, "Entry must retain float title poses and frames");
        if (tiles[index].state == 2) {
            require(actual == tiles[index], "State2 entry records must retain both velocities and their state");
        } else {
            require(actual.state == 0, "Every nonstate2 record must be reset to state0");
        }
    }

    require(bits(outro.records()[1].velocity_x) == 0xbf666667U && bits(outro.records()[1].velocity_y) == 0xbf8ccccdU && bits(outro.records()[2].velocity_x) == 0x3ecccccdU
                && bits(outro.records()[2].velocity_y) == 0xbfe66667U && bits(outro.records()[4].velocity_x) == 0xbdcccccdU && bits(outro.records()[4].velocity_y) == 0xbfb33333U,
        "Entry velocities must consume x then y and multiply the binary32 0.1 constant");

    auto protected_tiles = grid();
    for (auto& tile : protected_tiles) {
        tile.state = 2;
    }

    const Outro protected_outro(protected_tiles, random_state);
    require(random_state == 0xc2c729dbU && snapshot(protected_outro) == protected_tiles, "All-state2 entry must consume no CRT values");

    std::uint32_t full_random = 1;
    const Outro full(grid(), full_random);
    require(full_random == 0xe1e2ee01U && bits(full.records()[767].velocity_x) == 0x3f666667U && full.records()[767].velocity_y == 0,
        "Ordinary title entry must consume exactly 1536 draws through the last tile");
    const auto first_entry = snapshot(full);
    const Outro continued(grid(), full_random);
    require(full_random == 0xf4d5dc01U && snapshot(full) == first_entry && snapshot(continued) != first_entry,
        "A later entry must continue the supplied CRT state without reseeding or changing the earlier model");
}

void reveal_boundaries() {
    std::uint32_t random_state = 1;
    Outro outro(grid(), random_state);
    const auto entry = snapshot(outro);
    require(!outro.update(0.01F) && outro.phase_time() == 1 && outro.next_tile() == 0 && snapshot(outro) == entry,
        "Delta binary32 0.01 stores accumulator 1 while its retained product remains below the strict boundary");
    require(!outro.update(0) && outro.next_tile() == 0, "Accumulator exactly 1 must not reveal");
    require(!outro.update(0x1p-32F) && outro.next_tile() == 8 && outro.phase_time() == 0 && active(outro) == 8,
        "A retained sum just above 1 must reveal eight even when its binary32 store rounds back to 1");
    require(!outro.update(10) && outro.next_tile() == 16 && outro.phase_time() == 0 && active(outro) == 16 && !outro.holding(),
        "One large update must reveal only one batch and discard all overshoot");
    require(!outro.update(0) && outro.next_tile() == 16, "Discarded reveal overshoot must not catch up on the next update");
    require(random_state == 0xe1e2ee01U, "Reveal and motion must not consume random values");
}

void motion_and_float_stores() {
    std::uint32_t random_state = 1;
    Outro outro(grid(), random_state);
    require(!outro.update(0.125F), "First reveal must continue rather than finish");
    // Literal source-derived binary32 results: vx, vy, x, y after delta 1/8.
    constexpr std::array<std::array<std::uint32_t, 4>, 8> expected{{
        {0xbf6e6667U, 0xbf79999aU, 0xc13a4000U, 0xc1430000U},
        {0x3edccccdU, 0xbfd66667U, 0x41cb2000U, 0xc1a78000U},
        {0xbe066666U, 0xbfa33333U, 0x42197000U, 0xc17f0000U},
        {0x3f54cccdU, 0x3e000000U, 0x428cc800U, 0x3fc80000U},
        {0xbf54cccdU, 0xbfa33333U, 0x428b3800U, 0xc17f0000U},
        {0xbf080000U, 0xbf966667U, 0x42bab800U, 0xc16b0001U},
        {0xbf6e6667U, 0xbf79999aU, 0x42d8b800U, 0xc1430000U},
        {0xbf6e6667U, 0xbf133333U, 0x43005c00U, 0xc0e60000U},
    }};
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto& tile = outro.records()[index];
        require(tile.state == 1 && std::array{bits(tile.velocity_x), bits(tile.velocity_y), bits(tile.x), bits(tile.y)} == expected[index],
            "Revealed motion must use accelerated, float-stored velocities and retained multiplication");
    }

    require(outro.records()[8].state == 0 && outro.records()[8].x == 160 && outro.records()[8].y == 0, "Unrevealed state0 records must remain stationary");

    auto tiles = grid();
    for (auto& tile : tiles) {
        tile.state = 2;
        tile.x = 10;
        tile.y = 3;
        tile.velocity_y = 0.25F;
    }

    tiles[0].velocity_x = 1;
    tiles[1].velocity_x = -1;
    Outro branches(tiles, random_state);
    require(!branches.update(0.125F) && branches.records()[0].velocity_x == 1.03125F && branches.records()[0].x == 22.890625F && branches.records()[1].velocity_x == -1.03125F
                && branches.records()[1].x == -2.890625F && branches.records()[2].velocity_x == -0.03125F && branches.records()[2].x == 9.609375F
                && branches.records()[2].y == 7.6875F,
        "Positive and negative velocities accelerate away from zero; zero takes the negative branch");
    require(branches.records()[8].state == 2 && branches.records()[8].x == 9.609375F, "State2 records move even before the reveal reaches their index");
}

void hold_and_skip() {
    std::uint32_t random_state = 1;
    Outro outro(grid(), random_state);
    for (std::size_t batch = 1; batch <= 96; ++batch) {
        require(!outro.update(0.02F) && active(outro) == batch * 8 && outro.next_tile() == std::min(batch * 8, std::size_t{767}) && outro.phase_time() == 0,
            "Each accepted reveal batch must activate the next eight records in ascending order");
        for (std::size_t index = 0; index < outro.records().size(); ++index) {
            require(outro.records()[index].state == (index < batch * 8 ? 1 : 0), "Reveal must activate the ascending prefix without skipping any tile");
        }

        require(outro.holding() == (batch == 96), "Hold must begin only after revealing tile767");
    }

    require(!outro.update(5) && outro.phase_time() == 5 && outro.holding(), "A hold of exactly 5 seconds must remain active");
    require(outro.update(0x1p-25F) && outro.phase_time() == 5 && outro.finished(), "Hold exit must compare the retained sum above 5 even if the store remains 5");
    const auto finished = snapshot(outro);
    require(outro.update(1) && snapshot(outro) == finished, "Finished models must not perform another update");

    Outro skipped(grid(), random_state);
    const auto before = snapshot(skipped);
    require(skipped.update(0.5F, true) && skipped.finished() && skipped.next_tile() == 8 && skipped.records()[0].x != before[0].x,
        "Escape, Space or release skip must still reveal and move during the finishing update");
    require(skipped.logo_time() == 0.5F && skipped.draws().back().destination.x == 635, "The skipped final frame must still advance and draw its logo tween");
}

void draw_order_and_logo() {
    auto tiles = grid();
    for (std::size_t index = 0; index < tiles.size(); ++index) {
        tiles[index].x = static_cast<float>(index) * 0.25F - 0.875F;
        tiles[index].y += 0.625F;
        tiles[index].frame = static_cast<int>(767 - index) + 768;
    }
    std::uint32_t random_state = 1;
    Outro outro(tiles, random_state);
    const auto before = snapshot(outro);
    for (int repeat = 0; repeat < 2; ++repeat) {
        const auto draws = outro.draws();
        require(draws.size() == 769, "Outro must draw all 768 title tiles followed by one logo and no cursor");
        for (std::size_t index = 0; index < tiles.size(); ++index) {
            const auto& draw = draws[index];
            const auto& tile = tiles[index];
            require(draw.image == "gfx/mainscreen" && draw.source.x == tile.frame % 32 * 20 && draw.source.y == (tile.frame % 768) / 32 * 20 && draw.source.width == 20
                        && draw.source.height == 20 && draw.destination.x == static_cast<int>(tile.x) && draw.destination.y == static_cast<int>(tile.y) && draw.color_key
                        && !draw.mirror_x,
                "Ascending record painter order wraps loaded frame selection and truncates stored float poses toward zero");
        }

        const auto& logo = draws.back();
        require(logo.image == "gfx/Witansmall" && logo.source.x == 0 && logo.source.y == 0 && logo.source.width == 80 && logo.source.height == 81 && logo.destination.x == 585
                    && logo.destination.y == 426 && logo.color_key && !logo.mirror_x,
            "Witansmall must draw last at the initial 585,426 anchor");
    }

    require(snapshot(outro) == before && random_state == 0xe1e2ee01U, "Drawing must be pure");
    require(!outro.update(0.25F) && outro.draws().back().destination.x == 600, "Smoothstep time 1/4 stores logo 600.625 and truncates to 600");
    require(!outro.update(0.25F) && outro.draws().back().destination.x == 635, "Smoothstep midpoint is 635");
    require(!outro.update(0.25F) && outro.draws().back().destination.x == 669, "Smoothstep time 3/4 stores logo 669.375 and truncates to 669");
    require(!outro.update(0.5F) && outro.logo_time() == 1 && outro.draws().back().destination.x == 685, "Logo time overshoot must clamp at 1 and x 685");
    require(!outro.update(0.25F) && outro.logo_time() == 1 && outro.draws().back().destination.x == 685, "Clamped logo must remain at 685");
}

void invalid_deltas() {
    std::uint32_t random_state = 1;
    Outro outro(grid(), random_state);
    const auto before = snapshot(outro);
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected = false;
        try {
            outro.update(delta, true);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && snapshot(outro) == before && outro.phase_time() == 0 && outro.logo_time() == 0 && !outro.finished() && random_state == 0xe1e2ee01U,
            "Invalid deltas must be rejected before state mutation");
    }
}

} // namespace

int main() {
    try {
        entry_and_random();
        reveal_boundaries();
        motion_and_float_stores();
        hold_and_skip();
        draw_order_and_logo();
        invalid_deltas();
        std::cout << "PASS: outro entry/RNG, strict retained reveal/hold boundaries, no catchup, float motion, skip final update, logo and painter order\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
