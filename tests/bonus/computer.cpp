#include "bonus/computer.hpp"

#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct ChickenExpectation {
    int direction{};
    std::uint32_t x_bits{};
    int y{};
    int speed{};
    std::uint32_t animation_time_bits{};
    int frame{};
    int draw_x{};
};

struct InitCase {
    std::uint32_t random_before{};
    std::uint32_t first_update_delta_bits{};
    std::uint32_t random_after{};
    std::vector<ChickenExpectation> records{};
};

struct StepCase {
    std::uint32_t delta_bits{};
    std::uint32_t random_after{};
    std::vector<ChickenExpectation> records{};
};

struct SequenceCase {
    std::uint32_t random_before{};
    std::vector<StepCase> steps{};
};

const std::array<InitCase, 2> initializations = {
    {{0x00000001U, 0x00000000U, 0xa6a6b409U,
         {{1, 0x4428c000U, 334, 119, 0x00000000U, 0, 675}, {0, 0xc3ab0000U, 158, 114, 0x00000000U, 0, -342}, {1, 0x445c4000U, 81, 111, 0x00000000U, 0, 881},
             {1, 0x444cc000U, 342, 86, 0x00000000U, 0, 819}, {1, 0x44230000U, 302, 142, 0x00000000U, 0, 652}, {0, 0xc30d0000U, 316, 145, 0x00000000U, 0, -141},
             {1, 0x44578000U, 371, 119, 0x00000000U, 0, 862}, {0, 0xc3430000U, 299, 144, 0x00000000U, 0, -195}}},
        {0xa6a6b409U, 0x00000000U, 0x8ddc2911U,
            {{1, 0x4420c000U, 122, 123, 0x00000000U, 0, 643}, {0, 0xc3250000U, 111, 118, 0x00000000U, 0, -165}, {1, 0x445f0000U, 262, 87, 0x00000000U, 0, 892},
                {1, 0x4424c000U, 141, 128, 0x00000000U, 0, 659}, {0, 0xc3ad8000U, 190, 138, 0x00000000U, 0, -347}, {0, 0xc3500000U, 142, 98, 0x00000000U, 0, -208},
                {0, 0xc3be8000U, 290, 120, 0x00000000U, 0, -381}, {0, 0xc38f0000U, 301, 98, 0x00000000U, 0, -286}}}}};

const std::array<SequenceCase, 6> sequences = {
    {{0x00000001U, {{0x3d800000U, 0xa6a6b409U,
                        {{1, 0x4426e400U, 334, 119, 0x3f700000U, 0, 667}, {0, 0xc3a77000U, 158, 114, 0x3f700000U, 0, -334}, {1, 0x445a8400U, 81, 111, 0x3f700000U, 0, 874},
                            {1, 0x444b6800U, 342, 86, 0x3f700000U, 0, 813}, {1, 0x4420c800U, 302, 142, 0x3f700000U, 0, 643}, {0, 0xc303f000U, 316, 145, 0x3f700000U, 0, -131},
                            {1, 0x4455a400U, 371, 119, 0x3f700000U, 0, 854}, {0, 0xc33a0000U, 299, 144, 0x3f700000U, 0, -186}}},
                       {0x3b888887U, 0xa6a6b409U,
                           {{1, 0x4426c444U, 334, 119, 0x3f800000U, 0, 667}, {0, 0xc3a73333U, 158, 114, 0x3f800000U, 0, -334}, {1, 0x445a6666U, 81, 111, 0x3f800000U, 0, 873},
                               {1, 0x444b5111U, 342, 86, 0x3f800000U, 0, 813}, {1, 0x4420a222U, 302, 142, 0x3f800000U, 0, 642}, {0, 0xc3035555U, 316, 145, 0x3f800000U, 0, -131},
                               {1, 0x44558444U, 371, 119, 0x3f800000U, 0, 854}, {0, 0xc3396666U, 299, 144, 0x3f800000U, 0, -185}}},
                       {0x00000000U, 0xa6a6b409U,
                           {{1, 0x4426c444U, 334, 119, 0x3f800000U, 1, 667}, {0, 0xc3a73333U, 158, 114, 0x3f800000U, 1, -334}, {1, 0x445a6666U, 81, 111, 0x3f800000U, 1, 873},
                               {1, 0x444b5111U, 342, 86, 0x3f800000U, 1, 813}, {1, 0x4420a222U, 302, 142, 0x3f800000U, 1, 642}, {0, 0xc3035555U, 316, 145, 0x3f800000U, 1, -131},
                               {1, 0x44558444U, 371, 119, 0x3f800000U, 1, 854}, {0, 0xc3396666U, 299, 144, 0x3f800000U, 1, -185}}},
                       {0x3fa00000U, 0xa6a6b409U,
                           {{1, 0x44019444U, 334, 119, 0x00000000U, 0, 518}, {0, 0xc33fe666U, 158, 114, 0x00000000U, 0, -191}, {1, 0x4437b666U, 81, 111, 0x00000000U, 0, 734},
                               {1, 0x44307111U, 342, 86, 0x00000000U, 0, 705}, {1, 0x43e88444U, 302, 142, 0x00000000U, 0, 465}, {0, 0x4247aaacU, 316, 145, 0x00000000U, 0, 49},
                               {1, 0x44305444U, 371, 119, 0x00000000U, 0, 705}, {0, 0xc0acccc0U, 299, 144, 0x00000000U, 0, -5}}}}},
        {0x00000001U,
            {{0x3f99999aU, 0xa6a6b409U,
                 {{1, 0x44050ccdU, 334, 119, 0x41900000U, 18, 532}, {0, 0xc34d3333U, 158, 114, 0x41900000U, 18, -205}, {1, 0x443af333U, 81, 111, 0x41900000U, 18, 747},
                     {1, 0x4432f333U, 342, 86, 0x41900000U, 18, 715}, {1, 0x43f0cccdU, 302, 142, 0x41900000U, 18, 481}, {0, 0x42040002U, 316, 145, 0x41900000U, 18, 33},
                     {1, 0x4433cccdU, 371, 119, 0x41900000U, 18, 719}, {0, 0xc1b19996U, 299, 144, 0x41900000U, 18, -22}}},
                {0x3d800000U, 0xa6a6b409U,
                    {{1, 0x440330cdU, 334, 119, 0x41978000U, 18, 524}, {0, 0xc3461333U, 158, 114, 0x41978000U, 18, -198}, {1, 0x44393733U, 81, 111, 0x41978000U, 18, 740},
                        {1, 0x44319b33U, 342, 86, 0x41978000U, 18, 710}, {1, 0x43ec5ccdU, 302, 142, 0x41978000U, 18, 472}, {0, 0x42284002U, 316, 145, 0x41978000U, 18, 42},
                        {1, 0x4431f0cdU, 371, 119, 0x41978000U, 18, 711}, {0, 0xc153332cU, 299, 144, 0x41978000U, 18, -13}}},
                {0x3d800000U, 0xa6a6b409U,
                    {{1, 0x440154cdU, 334, 119, 0x00000000U, 0, 517}, {0, 0xc33ef333U, 158, 114, 0x00000000U, 0, -190}, {1, 0x44377b33U, 81, 111, 0x00000000U, 0, 733},
                        {1, 0x44304333U, 342, 86, 0x00000000U, 0, 705}, {1, 0x43e7eccdU, 302, 142, 0x00000000U, 0, 463}, {0, 0x424c8002U, 316, 145, 0x00000000U, 0, 51},
                        {1, 0x443014cdU, 371, 119, 0x00000000U, 0, 704}, {0, 0xc0866658U, 299, 144, 0x00000000U, 0, -4}}},
                {0x00000000U, 0xa6a6b409U,
                    {{1, 0x440154cdU, 334, 119, 0x00000000U, 0, 517}, {0, 0xc33ef333U, 158, 114, 0x00000000U, 0, -190}, {1, 0x44377b33U, 81, 111, 0x00000000U, 0, 733},
                        {1, 0x44304333U, 342, 86, 0x00000000U, 0, 705}, {1, 0x43e7eccdU, 302, 142, 0x00000000U, 0, 463}, {0, 0x424c8002U, 316, 145, 0x00000000U, 0, 51},
                        {1, 0x443014cdU, 371, 119, 0x00000000U, 0, 704}, {0, 0xc0866658U, 299, 144, 0x00000000U, 0, -4}}}}},
        {0x00000001U,
            {{0x3f911111U, 0xa6a6b409U,
                 {{1, 0x44070889U, 334, 119, 0x41880000U, 16, 540}, {0, 0xc354cccdU, 158, 114, 0x41880000U, 16, -212}, {1, 0x443ccccdU, 81, 111, 0x41880000U, 16, 755},
                     {1, 0x44346222U, 342, 86, 0x41880000U, 16, 721}, {1, 0x43f58889U, 302, 142, 0x41880000U, 16, 491}, {0, 0x41baaaaaU, 316, 145, 0x41880000U, 16, 23},
                     {1, 0x4435c889U, 371, 119, 0x41880000U, 16, 727}, {0, 0xc1fe6667U, 299, 144, 0x41880000U, 16, -31}}},
                {0x00000000U, 0xa6a6b409U,
                    {{1, 0x44070889U, 334, 119, 0x41880000U, 17, 540}, {0, 0xc354cccdU, 158, 114, 0x41880000U, 17, -212}, {1, 0x443ccccdU, 81, 111, 0x41880000U, 17, 755},
                        {1, 0x44346222U, 342, 86, 0x41880000U, 17, 721}, {1, 0x43f58889U, 302, 142, 0x41880000U, 17, 491}, {0, 0x41baaaaaU, 316, 145, 0x41880000U, 17, 23},
                        {1, 0x4435c889U, 371, 119, 0x41880000U, 17, 727}, {0, 0xc1fe6667U, 299, 144, 0x41880000U, 17, -31}}}}},
        {0x0000028eU, {{0x40fe4000U, 0xfe14cb28U,
                           {{0, 0x44200000U, 230, 128, 0x00000000U, 0, 640}, {1, 0x4446c000U, 285, 124, 0x00000000U, 0, 795}, {1, 0x4450c000U, 141, 126, 0x00000000U, 0, 835},
                               {0, 0x439a0800U, 157, 72, 0x00000000U, 0, 308}, {0, 0x43fd5900U, 152, 97, 0x00000000U, 0, 506}, {0, 0x43d22d00U, 234, 85, 0x00000000U, 0, 420},
                               {0, 0x43987100U, 232, 57, 0x00000000U, 0, 304}, {0, 0x439af800U, 64, 56, 0x00000000U, 0, 309}}},
                          {0x33800000U, 0x73dd957fU,
                              {{0, 0xc3a50000U, 383, 111, 0x41300001U, 11, -330}, {1, 0x4446c000U, 285, 124, 0x35700000U, 0, 795}, {1, 0x4450c000U, 141, 126, 0x35700000U, 0, 835},
                                  {0, 0x439a0800U, 157, 72, 0x35700000U, 0, 308}, {0, 0x43fd5900U, 152, 97, 0x35700000U, 0, 506}, {0, 0x43d22d00U, 234, 85, 0x35700000U, 0, 420},
                                  {0, 0x43987100U, 232, 57, 0x35700000U, 0, 304}, {0, 0x439af800U, 64, 56, 0x35700000U, 0, 309}}},
                          {0x00000000U, 0x73dd957fU,
                              {{0, 0xc3a50000U, 383, 111, 0x41300001U, 11, -330}, {1, 0x4446c000U, 285, 124, 0x35700000U, 0, 795}, {1, 0x4450c000U, 141, 126, 0x35700000U, 0, 835},
                                  {0, 0x439a0800U, 157, 72, 0x35700000U, 0, 308}, {0, 0x43fd5900U, 152, 97, 0x35700000U, 0, 506}, {0, 0x43d22d00U, 234, 85, 0x35700000U, 0, 420},
                                  {0, 0x43987100U, 232, 57, 0x35700000U, 0, 304}, {0, 0x439af800U, 64, 56, 0x35700000U, 0, 309}}}}},
        {0x00000320U, {{0x40fb0000U, 0xc271fbb9U,
                           {{1, 0xc3000000U, 127, 128, 0x00000000U, 0, -128}, {1, 0x44420000U, 106, 88, 0x00000000U, 0, 776}, {0, 0x43747800U, 323, 61, 0x00000000U, 0, 244},
                               {0, 0xc3818000U, 7, 85, 0x00000000U, 0, -259}, {0, 0x44196a00U, 61, 111, 0x00000000U, 0, 613}, {1, 0x44254000U, 58, 59, 0x00000000U, 0, 661},
                               {0, 0x43999000U, 147, 76, 0x00000000U, 0, 307}, {0, 0x43cc8400U, 272, 83, 0x00000000U, 0, 409}}},
                          {0x33800000U, 0xa1664fdcU,
                              {{0, 0xc3840000U, 92, 100, 0x40800002U, 4, -264}, {1, 0x44420000U, 106, 88, 0x35700000U, 0, 776}, {0, 0x43747800U, 323, 61, 0x35700000U, 0, 244},
                                  {0, 0xc3818000U, 7, 85, 0x35700000U, 0, -259}, {0, 0x44196a00U, 61, 111, 0x35700000U, 0, 613}, {1, 0x44254000U, 58, 59, 0x35700000U, 0, 661},
                                  {0, 0x43999000U, 147, 76, 0x35700000U, 0, 307}, {0, 0x43cc8400U, 272, 83, 0x35700000U, 0, 409}}},
                          {0x00000000U, 0xa1664fdcU,
                              {{0, 0xc3840000U, 92, 100, 0x40800002U, 4, -264}, {1, 0x44420000U, 106, 88, 0x35700000U, 0, 776}, {0, 0x43747800U, 323, 61, 0x35700000U, 0, 244},
                                  {0, 0xc3818000U, 7, 85, 0x35700000U, 0, -259}, {0, 0x44196a00U, 61, 111, 0x35700000U, 0, 613}, {1, 0x44254000U, 58, 59, 0x35700000U, 0, 661},
                                  {0, 0x43999000U, 147, 76, 0x35700000U, 0, 307}, {0, 0x43cc8400U, 272, 83, 0x35700000U, 0, 409}}}}},
        {0x00000001U,
            {{0x41800000U, 0x8ddc2911U,
                 {{1, 0x4420c000U, 122, 123, 0x00000000U, 0, 643}, {0, 0xc3250000U, 111, 118, 0x00000000U, 0, -165}, {1, 0x445f0000U, 262, 87, 0x00000000U, 0, 892},
                     {1, 0x4424c000U, 141, 128, 0x00000000U, 0, 659}, {0, 0xc3ad8000U, 190, 138, 0x00000000U, 0, -347}, {0, 0xc3500000U, 142, 98, 0x00000000U, 0, -208},
                     {0, 0xc3be8000U, 290, 120, 0x00000000U, 0, -381}, {0, 0xc38f0000U, 301, 98, 0x00000000U, 0, -286}}},
                {0x3d800000U, 0x8ddc2911U,
                    {{1, 0x441ed400U, 122, 123, 0x3f700000U, 0, 635}, {0, 0xc31da000U, 111, 118, 0x3f700000U, 0, -157}, {1, 0x445da400U, 262, 87, 0x3f700000U, 0, 886},
                        {1, 0x4422c000U, 141, 128, 0x3f700000U, 0, 651}, {0, 0xc3a93000U, 190, 138, 0x3f700000U, 0, -338}, {0, 0xc349e000U, 142, 98, 0x3f700000U, 0, -201},
                        {0, 0xc3bac000U, 290, 120, 0x3f700000U, 0, -373}, {0, 0xc38bf000U, 301, 98, 0x3f700000U, 0, -279}}}}}}};

using moorhuhn2::bonus::Computer;
using moorhuhn2::bonus::ComputerChicken;

using moorhuhn2::test::require;

float binary32(std::uint32_t value) {
    return std::bit_cast<float>(value);
}

std::array<ComputerChicken, 8> snapshot(const Computer& screen) {
    std::array<ComputerChicken, 8> result;
    std::ranges::copy(screen.records(), result.begin());

    return result;
}

void check(const Computer& screen, std::uint32_t random_state, const auto& expected) {
    require(random_state == expected.random_after, "Original CRT consumption/order differs");
    const auto& records = expected.records;
    require(records.size() == screen.records().size(), "Expected eight original Computer slots");
    for (std::size_t index = 0; index < records.size(); ++index) {
        const auto& actual = screen.records()[index];
        const auto& record = records[index];
        require(actual.direction == record.direction && std::bit_cast<std::uint32_t>(actual.x) == record.x_bits && actual.y == record.y && actual.speed == record.speed
                    && std::bit_cast<std::uint32_t>(actual.animation_time) == record.animation_time_bits && actual.frame == record.frame,
            "Original Computer movement, phase, speed or direction differs at slot " + std::to_string(index));
    }

    const auto before = snapshot(screen);
    for (int repeat = 0; repeat < 2; ++repeat) {
        const auto draws = screen.draws();
        const auto& background = draws[0];
        require(background.image == "gfx/Computer" && background.source.x == 0 && background.source.y == 0 && background.source.width == 640 && background.source.height == 480
                    && background.destination.x == 0 && background.destination.y == 0 && background.color_key && !background.mirror_x,
            "Original Computer background must draw first at its 640x480 anchor");
        for (std::size_t index = 0; index < records.size(); ++index) {
            const auto& actual = draws[index + 1];
            const auto& record = records[index];
            const auto image = record.direction == 0 ? "gfx/Chickright1" : "gfx/Chickleft1";
            require(actual.image == image && actual.source.x == 0 && actual.source.y == record.frame * 124 && actual.source.width == 165 && actual.source.height == 124
                        && actual.destination.x == record.draw_x && actual.destination.y == record.y && actual.color_key && !actual.mirror_x,
                "Original Computer sheet/cell, truncated pose or ascending painter order differs");
        }
    }

    require(snapshot(screen) == before, "Computer drawing must not advance its records");
}

void invalid_inputs() {
    std::uint32_t random_state = 1;
    Computer screen(random_state);
    const auto before = snapshot(screen);
    const auto random_before = random_state;
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max(), 2147483648.0F}) {
        bool rejected = false;
        try {
            screen.update(delta, random_state);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && snapshot(screen) == before && random_state == random_before, "Invalid Computer delta accepted or changed records/random state");
    }

    screen.update(0, random_state);
    check(screen, random_state, initializations[0]);
}

void continued_entry() {
    std::uint32_t random_state = 1;
    Computer first(random_state);
    first.update(0, random_state);
    check(first, random_state, initializations[0]);
    first.update(binary32(sequences[1].steps[0].delta_bits), random_state);
    const auto before_reentry = snapshot(first);
    Computer second(random_state);
    second.update(0, random_state);
    check(second, random_state, initializations[1]);
    require(snapshot(first) == before_reentry, "Entering another Computer screen changed the previous instance");
}

} // namespace

int main() {
    try {
        for (const auto& expected : initializations) {
            auto random_state = expected.random_before;
            Computer screen(random_state);
            screen.update(binary32(expected.first_update_delta_bits), random_state);
            check(screen, random_state, expected);
        }
        std::size_t updates = 0;
        for (const auto& sequence : sequences) {
            auto random_state = sequence.random_before;
            Computer screen(random_state);
            for (const auto& expected : sequence.steps) {
                screen.update(binary32(expected.delta_bits), random_state);
                check(screen, random_state, expected);
                ++updates;
            }
        }

        continued_entry();
        invalid_inputs();
        std::cout << "PASS: " << initializations.size() << " Computer entries, " << updates
                  << " original updates, strict/retained boundaries, phase reset, RNG continuation, draw order and invalid-delta atomicity\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
