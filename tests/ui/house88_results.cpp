#include "ui/house88_results.hpp"

#include "support/require.hpp"

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
    std::uint32_t x_bits{};
    std::uint32_t y_bits{};
    std::uint32_t speed_bits{};
    std::uint32_t animation_time_bits{};
    int frame{};
    int variant{};
};

struct DrawExpectation {
    std::string image{};
    std::array<int, 4> source{};
    std::array<int, 2> destination{};
};

struct InitCase {
    int actual_score{};
    moorhuhn2::ui::ResultsRandom random_before{};
    std::vector<ChickenExpectation> records{};
    moorhuhn2::ui::ResultsRandom random_after{};
    std::vector<DrawExpectation> draws{};
};

struct StepCase {
    std::uint32_t delta_bits{};
    std::vector<ChickenExpectation> records{};
    moorhuhn2::ui::ResultsRandom random_after{};
    std::vector<DrawExpectation> draws{};
};

struct SequenceCase {
    int actual_score{};
    moorhuhn2::ui::ResultsRandom random_before{};
    std::vector<StepCase> steps{};
};

const std::array<InitCase, 2> initializations = {
    {{98, {1, 0},
         {{0x42240000U, 0xc2c60000U, 0x42a80000U, 0x00000000U, 0, 0}, {0x440e4000U, 0xc32c0000U, 0x429c0000U, 0x00000000U, 0, 0},
             {0x440c8000U, 0xc2a00000U, 0x425c0000U, 0x00000000U, 0, 1}, {0x43f08000U, 0xc2f60000U, 0x42740000U, 0x00000000U, 0, 1},
             {0x4414c000U, 0xc2cc0000U, 0x429a0000U, 0x00000000U, 0, 0}, {0x4413c000U, 0xc2980000U, 0x42500000U, 0x00000000U, 0, 1},
             {0x43920000U, 0xc31e0000U, 0x428e0000U, 0x00000000U, 0, 0}, {0x44018000U, 0xc2ee0000U, 0x42c20000U, 0x00000000U, 0, 0},
             {0x43b98000U, 0xc2a40000U, 0x428a0000U, 0x00000000U, 0, 0}, {0x43e98000U, 0xc2f60000U, 0x42aa0000U, 0x00000000U, 0, 0},
             {0x43fb8000U, 0xc2860000U, 0x42900000U, 0x00000000U, 0, 1}, {0x43888000U, 0xc2f00000U, 0x42b60000U, 0x00000000U, 0, 1},
             {0x42540000U, 0xc3140000U, 0x42c20000U, 0x00000000U, 0, 0}, {0x43830000U, 0xc3350000U, 0x42ae0000U, 0x00000000U, 0, 1},
             {0x43a18000U, 0xc29a0000U, 0x429e0000U, 0x00000000U, 0, 0}, {0x439e0000U, 0xc31b0000U, 0x42b40000U, 0x00000000U, 0, 0}},
         {120733505, 64},
         {{"gfx/DChickleft2", {0, 0, 83, 62}, {41, -99}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {569, -172}}, {"gfx/DChickright2", {0, 0, 83, 62}, {562, -80}},
             {"gfx/DChickright2", {0, 0, 83, 62}, {481, -123}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {595, -102}}, {"gfx/DChickright2", {0, 0, 83, 62}, {591, -76}},
             {"gfx/DChickleft2", {0, 0, 83, 62}, {292, -158}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {518, -119}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {371, -82}},
             {"gfx/DChickleft2", {0, 0, 83, 62}, {467, -123}}, {"gfx/DChickright2", {0, 0, 83, 62}, {503, -67}}, {"gfx/DChickright2", {0, 0, 83, 62}, {273, -120}},
             {"gfx/DChickleft2", {0, 0, 83, 62}, {53, -148}}, {"gfx/DChickright2", {0, 0, 83, 62}, {262, -181}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {323, -77}},
             {"gfx/DChickleft2", {0, 0, 83, 62}, {316, -155}}}},
        {1500, {1, 0},
            {{0x43900000U, 0xc2b40000U, 0x42b40000U, 0x00000000U, 0, 0}, {0x42800000U, 0xc3380000U, 0x42c00000U, 0x00000000U, 0, 1},
                {0x43910000U, 0xc3090000U, 0x428c0000U, 0x00000000U, 0, 0}, {0x40c00000U, 0xc33d0000U, 0x42ba0000U, 0x00000000U, 0, 0},
                {0x43d68000U, 0xc30f0000U, 0x42a80000U, 0x00000000U, 0, 0}, {0x431c0000U, 0xc3000000U, 0x42840000U, 0x00000000U, 0, 0},
                {0x43030000U, 0xc3040000U, 0x42bc0000U, 0x00000000U, 0, 1}, {0x41d00000U, 0xc2f60000U, 0x42ae0000U, 0x00000000U, 0, 0},
                {0x44018000U, 0xc2c40000U, 0x429e0000U, 0x00000000U, 0, 1}, {0x42040000U, 0xc2960000U, 0x42b20000U, 0x00000000U, 0, 0},
                {0x43fc0000U, 0xc30a0000U, 0x429a0000U, 0x00000000U, 0, 0}, {0x43ec8000U, 0xc3320000U, 0x428e0000U, 0x00000000U, 0, 1},
                {0x44030000U, 0xc2800000U, 0x428c0000U, 0x00000000U, 0, 1}, {0x43bc8000U, 0xc3150000U, 0x42c20000U, 0x00000000U, 0, 0},
                {0x44128000U, 0xc32a0000U, 0x42740000U, 0x00000000U, 0, 0}, {0x431b0000U, 0xc2fe0000U, 0x425c0000U, 0x00000000U, 0, 0}},
            {3168190081, 128},
            {{"gfx/DChickleft2", {0, 0, 83, 62}, {288, -90}}, {"gfx/DChickright2", {0, 0, 83, 62}, {64, -184}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {290, -137}},
                {"gfx/DChickleft2", {0, 0, 83, 62}, {6, -189}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {429, -143}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {156, -128}},
                {"gfx/DChickright2", {0, 0, 83, 62}, {131, -132}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {26, -123}}, {"gfx/DChickright2", {0, 0, 83, 62}, {518, -98}},
                {"gfx/DChickleft2", {0, 0, 83, 62}, {33, -75}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {504, -138}}, {"gfx/DChickright2", {0, 0, 83, 62}, {473, -178}},
                {"gfx/DChickright2", {0, 0, 83, 62}, {524, -64}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {377, -149}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {586, -170}},
                {"gfx/DChickleft2", {0, 0, 83, 62}, {155, -127}}}}}};

const std::array<SequenceCase, 4> sequences = {
    {{98, {1, 0},
         {{0x00000000U,
              {{0x42240000U, 0xc2c60000U, 0x42a80000U, 0x00000000U, 0, 0}, {0x440e4000U, 0xc32c0000U, 0x429c0000U, 0x00000000U, 0, 0},
                  {0x440c8000U, 0xc2a00000U, 0x425c0000U, 0x00000000U, 0, 1}, {0x43f08000U, 0xc2f60000U, 0x42740000U, 0x00000000U, 0, 1},
                  {0x4414c000U, 0xc2cc0000U, 0x429a0000U, 0x00000000U, 0, 0}, {0x4413c000U, 0xc2980000U, 0x42500000U, 0x00000000U, 0, 1},
                  {0x43920000U, 0xc31e0000U, 0x428e0000U, 0x00000000U, 0, 0}, {0x44018000U, 0xc2ee0000U, 0x42c20000U, 0x00000000U, 0, 0},
                  {0x43b98000U, 0xc2a40000U, 0x428a0000U, 0x00000000U, 0, 0}, {0x43e98000U, 0xc2f60000U, 0x42aa0000U, 0x00000000U, 0, 0},
                  {0x43fb8000U, 0xc2860000U, 0x42900000U, 0x00000000U, 0, 1}, {0x43888000U, 0xc2f00000U, 0x42b60000U, 0x00000000U, 0, 1},
                  {0x42540000U, 0xc3140000U, 0x42c20000U, 0x00000000U, 0, 0}, {0x43830000U, 0xc3350000U, 0x42ae0000U, 0x00000000U, 0, 1},
                  {0x43a18000U, 0xc29a0000U, 0x429e0000U, 0x00000000U, 0, 0}, {0x439e0000U, 0xc31b0000U, 0x42b40000U, 0x00000000U, 0, 0}},
              {120733505, 64},
              {{"gfx/DChickleft2", {0, 0, 83, 62}, {41, -99}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {569, -172}}, {"gfx/DChickright2", {0, 0, 83, 62}, {562, -80}},
                  {"gfx/DChickright2", {0, 0, 83, 62}, {481, -123}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {595, -102}}, {"gfx/DChickright2", {0, 0, 83, 62}, {591, -76}},
                  {"gfx/DChickleft2", {0, 0, 83, 62}, {292, -158}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {518, -119}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {371, -82}},
                  {"gfx/DChickleft2", {0, 0, 83, 62}, {467, -123}}, {"gfx/DChickright2", {0, 0, 83, 62}, {503, -67}}, {"gfx/DChickright2", {0, 0, 83, 62}, {273, -120}},
                  {"gfx/DChickleft2", {0, 0, 83, 62}, {53, -148}}, {"gfx/DChickright2", {0, 0, 83, 62}, {262, -181}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {323, -77}},
                  {"gfx/DChickleft2", {0, 0, 83, 62}, {316, -155}}}},
             {0x3dccccccU,
                 {{0x42240000U, 0xc2b53333U, 0x42a80000U, 0x3f7fffffU, 0, 0}, {0x440e4000U, 0xc3243333U, 0x429c0000U, 0x3f7fffffU, 0, 0},
                     {0x440c8000U, 0xc2950000U, 0x425c0000U, 0x3f7fffffU, 0, 1}, {0x43f08000U, 0xc2e9cccdU, 0x42740000U, 0x3f7fffffU, 0, 1},
                     {0x4414c000U, 0xc2bc999aU, 0x429a0000U, 0x3f7fffffU, 0, 0}, {0x4413c000U, 0xc28d999aU, 0x42500000U, 0x3f7fffffU, 0, 1},
                     {0x43920000U, 0xc316e666U, 0x428e0000U, 0x3f7fffffU, 0, 0}, {0x44018000U, 0xc2da999aU, 0x42c20000U, 0x3f7fffffU, 0, 0},
                     {0x43b98000U, 0xc2963333U, 0x428a0000U, 0x3f7fffffU, 0, 0}, {0x43e98000U, 0xc2e50000U, 0x42aa0000U, 0x3f7fffffU, 0, 0},
                     {0x43fb8000U, 0xc26f3333U, 0x42900000U, 0x3f7fffffU, 0, 1}, {0x43888000U, 0xc2ddcccdU, 0x42b60000U, 0x3f7fffffU, 0, 1},
                     {0x42540000U, 0xc30a4ccdU, 0x42c20000U, 0x3f7fffffU, 0, 0}, {0x43830000U, 0xc32c4ccdU, 0x42ae0000U, 0x3f7fffffU, 0, 1},
                     {0x43a18000U, 0xc28a3333U, 0x429e0000U, 0x3f7fffffU, 0, 0}, {0x439e0000U, 0xc3120000U, 0x42b40000U, 0x3f7fffffU, 0, 0}},
                 {120733505, 64},
                 {{"gfx/DChickleft2", {0, 0, 83, 62}, {41, -90}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {569, -164}}, {"gfx/DChickright2", {0, 0, 83, 62}, {562, -74}},
                     {"gfx/DChickright2", {0, 0, 83, 62}, {481, -116}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {595, -94}}, {"gfx/DChickright2", {0, 0, 83, 62}, {591, -70}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {292, -150}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {518, -109}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {371, -75}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {467, -114}}, {"gfx/DChickright2", {0, 0, 83, 62}, {503, -59}}, {"gfx/DChickright2", {0, 0, 83, 62}, {273, -110}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {53, -138}}, {"gfx/DChickright2", {0, 0, 83, 62}, {262, -172}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {323, -69}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {316, -146}}}},
             {0x31800000U,
                 {{0x42240000U, 0xc2b53333U, 0x42a80000U, 0x3f800000U, 0, 0}, {0x440e4000U, 0xc3243333U, 0x429c0000U, 0x3f800000U, 0, 0},
                     {0x440c8000U, 0xc2950000U, 0x425c0000U, 0x3f800000U, 0, 1}, {0x43f08000U, 0xc2e9cccdU, 0x42740000U, 0x3f800000U, 0, 1},
                     {0x4414c000U, 0xc2bc999aU, 0x429a0000U, 0x3f800000U, 0, 0}, {0x4413c000U, 0xc28d999aU, 0x42500000U, 0x3f800000U, 0, 1},
                     {0x43920000U, 0xc316e666U, 0x428e0000U, 0x3f800000U, 0, 0}, {0x44018000U, 0xc2da999aU, 0x42c20000U, 0x3f800000U, 0, 0},
                     {0x43b98000U, 0xc2963333U, 0x428a0000U, 0x3f800000U, 0, 0}, {0x43e98000U, 0xc2e50000U, 0x42aa0000U, 0x3f800000U, 0, 0},
                     {0x43fb8000U, 0xc26f3333U, 0x42900000U, 0x3f800000U, 0, 1}, {0x43888000U, 0xc2ddcccdU, 0x42b60000U, 0x3f800000U, 0, 1},
                     {0x42540000U, 0xc30a4ccdU, 0x42c20000U, 0x3f800000U, 0, 0}, {0x43830000U, 0xc32c4ccdU, 0x42ae0000U, 0x3f800000U, 0, 1},
                     {0x43a18000U, 0xc28a3333U, 0x429e0000U, 0x3f800000U, 0, 0}, {0x439e0000U, 0xc3120000U, 0x42b40000U, 0x3f800000U, 0, 0}},
                 {120733505, 64},
                 {{"gfx/DChickleft2", {0, 0, 83, 62}, {41, -90}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {569, -164}}, {"gfx/DChickright2", {0, 0, 83, 62}, {562, -74}},
                     {"gfx/DChickright2", {0, 0, 83, 62}, {481, -116}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {595, -94}}, {"gfx/DChickright2", {0, 0, 83, 62}, {591, -70}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {292, -150}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {518, -109}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {371, -75}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {467, -114}}, {"gfx/DChickright2", {0, 0, 83, 62}, {503, -59}}, {"gfx/DChickright2", {0, 0, 83, 62}, {273, -110}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {53, -138}}, {"gfx/DChickright2", {0, 0, 83, 62}, {262, -172}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {323, -69}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {316, -146}}}},
             {0x3f800000U,
                 {{0x42240000U, 0xc0d33330U, 0x42a80000U, 0x41300000U, 1, 0}, {0x440e4000U, 0xc2ac6666U, 0x429c0000U, 0x41300000U, 1, 0},
                     {0x440c8000U, 0xc19c0000U, 0x425c0000U, 0x41300000U, 1, 1}, {0x43f08000U, 0xc25f999aU, 0x42740000U, 0x41300000U, 1, 1},
                     {0x4414c000U, 0xc18a6668U, 0x429a0000U, 0x41300000U, 1, 0}, {0x4413c000U, 0xc1966668U, 0x42500000U, 0x41300000U, 1, 1},
                     {0x43920000U, 0xc29fccccU, 0x428e0000U, 0x41300000U, 1, 0}, {0x44018000U, 0xc144ccd0U, 0x42c20000U, 0x41300000U, 1, 0},
                     {0x43b98000U, 0xc0c33330U, 0x428a0000U, 0x41300000U, 1, 0}, {0x43e98000U, 0xc1ec0000U, 0x42aa0000U, 0x41300000U, 1, 0},
                     {0x43fb8000U, 0x41433334U, 0x42900000U, 0x41300000U, 1, 1}, {0x43888000U, 0xc19f3334U, 0x42b60000U, 0x41300000U, 1, 1},
                     {0x42540000U, 0xc2253334U, 0x42c20000U, 0x41300000U, 1, 0}, {0x43830000U, 0xc2aa999aU, 0x42ae0000U, 0x41300000U, 1, 1},
                     {0x43a18000U, 0x411e6668U, 0x429e0000U, 0x41300000U, 1, 0}, {0x439e0000U, 0xc2600000U, 0x42b40000U, 0x41300000U, 1, 0}},
                 {120733505, 64},
                 {{"gfx/DChickleft2", {0, 62, 83, 62}, {41, -6}}, {"gfx/DChickleft2", {0, 62, 83, 62}, {569, -86}}, {"gfx/DChickright2", {0, 62, 83, 62}, {562, -19}},
                     {"gfx/DChickright2", {0, 62, 83, 62}, {481, -55}}, {"gfx/DChickleft2", {0, 62, 83, 62}, {595, -17}}, {"gfx/DChickright2", {0, 62, 83, 62}, {591, -18}},
                     {"gfx/DChickleft2", {0, 62, 83, 62}, {292, -79}}, {"gfx/DChickleft2", {0, 62, 83, 62}, {518, -12}}, {"gfx/DChickleft2", {0, 62, 83, 62}, {371, -6}},
                     {"gfx/DChickleft2", {0, 62, 83, 62}, {467, -29}}, {"gfx/DChickright2", {0, 62, 83, 62}, {503, 12}}, {"gfx/DChickright2", {0, 62, 83, 62}, {273, -19}},
                     {"gfx/DChickleft2", {0, 62, 83, 62}, {53, -41}}, {"gfx/DChickright2", {0, 62, 83, 62}, {262, -85}}, {"gfx/DChickleft2", {0, 62, 83, 62}, {323, 9}},
                     {"gfx/DChickleft2", {0, 62, 83, 62}, {316, -56}}}}}},
        {98, {1, 0},
            {{0x40e00000U,
                 {{0x42d40000U, 0xc2c00000U, 0x42b40000U, 0x428c0000U, 0, 0}, {0x440e4000U, 0x43bb0000U, 0x429c0000U, 0x428c0000U, 0, 0},
                     {0x440c8000U, 0x43988000U, 0x425c0000U, 0x428c0000U, 0, 1}, {0x43f08000U, 0x43980000U, 0x42740000U, 0x428c0000U, 0, 1},
                     {0x4414c000U, 0x43da8000U, 0x429a0000U, 0x428c0000U, 0, 0}, {0x4413c000U, 0x43900000U, 0x42500000U, 0x428c0000U, 0, 1},
                     {0x43920000U, 0x43a98000U, 0x428e0000U, 0x428c0000U, 0, 0}, {0x43e00000U, 0xc3000000U, 0x42c00000U, 0x428c0000U, 0, 1},
                     {0x43b98000U, 0x43c88000U, 0x428a0000U, 0x428c0000U, 0, 0}, {0x43e98000U, 0x43ec0000U, 0x42aa0000U, 0x428c0000U, 0, 0},
                     {0x43fb8000U, 0x43da8000U, 0x42900000U, 0x428c0000U, 0, 1}, {0x43010000U, 0xc2a40000U, 0x428c0000U, 0x428c0000U, 0, 0},
                     {0x43fa8000U, 0xc2bc0000U, 0x42ba0000U, 0x428c0000U, 0, 0}, {0x43830000U, 0x43d60000U, 0x42ae0000U, 0x428c0000U, 0, 1},
                     {0x43a18000U, 0x43ee0000U, 0x429e0000U, 0x428c0000U, 0, 0}, {0x439e0000U, 0x43ed8000U, 0x42b40000U, 0x428c0000U, 0, 0}},
                 {2380015889, 80},
                 {{"gfx/DChickleft2", {0, 0, 83, 62}, {106, -96}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {569, 374}}, {"gfx/DChickright2", {0, 0, 83, 62}, {562, 305}},
                     {"gfx/DChickright2", {0, 0, 83, 62}, {481, 304}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {595, 437}}, {"gfx/DChickright2", {0, 0, 83, 62}, {591, 288}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {292, 339}}, {"gfx/DChickright2", {0, 0, 83, 62}, {448, -128}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {371, 401}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {467, 472}}, {"gfx/DChickright2", {0, 0, 83, 62}, {503, 437}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {129, -82}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {501, -94}}, {"gfx/DChickright2", {0, 0, 83, 62}, {262, 428}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {323, 476}},
                     {"gfx/DChickleft2", {0, 0, 83, 62}, {316, 475}}}},
                {0x3d800000U,
                    {{0x42d40000U, 0xc2b4c000U, 0x42b40000U, 0x428d4000U, 0, 0}, {0x440e4000U, 0x43bd7000U, 0x429c0000U, 0x428d4000U, 0, 0},
                        {0x440c8000U, 0x439a3800U, 0x425c0000U, 0x428d4000U, 0, 1}, {0x43f08000U, 0x4399e800U, 0x42740000U, 0x428d4000U, 0, 1},
                        {0x4414c000U, 0x43dce800U, 0x429a0000U, 0x428d4000U, 0, 0}, {0x4413c000U, 0x4391a000U, 0x42500000U, 0x428d4000U, 0, 1},
                        {0x43920000U, 0x43abb800U, 0x428e0000U, 0x428d4000U, 0, 0}, {0x43e00000U, 0xc2f40000U, 0x42c00000U, 0x428d4000U, 0, 1},
                        {0x43b98000U, 0x43caa800U, 0x428a0000U, 0x428d4000U, 0, 0}, {0x43e98000U, 0x43eea800U, 0x42aa0000U, 0x428d4000U, 0, 0},
                        {0x43fb8000U, 0x43dcc000U, 0x42900000U, 0x428d4000U, 0, 1}, {0x43010000U, 0xc29b4000U, 0x428c0000U, 0x428d4000U, 0, 0},
                        {0x43fa8000U, 0xc2b06000U, 0x42ba0000U, 0x428d4000U, 0, 0}, {0x43830000U, 0x43d8b800U, 0x42ae0000U, 0x428d4000U, 0, 1},
                        {0x41b80000U, 0xc2da0000U, 0x42a80000U, 0x428d4000U, 0, 0}, {0x43dc0000U, 0xc3040000U, 0x42840000U, 0x428d4000U, 0, 0}},
                    {483449721, 88},
                    {{"gfx/DChickleft2", {0, 0, 83, 62}, {106, -90}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {569, 378}}, {"gfx/DChickright2", {0, 0, 83, 62}, {562, 308}},
                        {"gfx/DChickright2", {0, 0, 83, 62}, {481, 307}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {595, 441}}, {"gfx/DChickright2", {0, 0, 83, 62}, {591, 291}},
                        {"gfx/DChickleft2", {0, 0, 83, 62}, {292, 343}}, {"gfx/DChickright2", {0, 0, 83, 62}, {448, -122}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {371, 405}},
                        {"gfx/DChickleft2", {0, 0, 83, 62}, {467, 477}}, {"gfx/DChickright2", {0, 0, 83, 62}, {503, 441}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {129, -77}},
                        {"gfx/DChickleft2", {0, 0, 83, 62}, {501, -88}}, {"gfx/DChickright2", {0, 0, 83, 62}, {262, 433}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {23, -109}},
                        {"gfx/DChickleft2", {0, 0, 83, 62}, {440, -132}}}}}},
        {98, {1, 0},
            {{0x40dc9249U,
                {{0x42240000U, 0x43f00000U, 0x42a80000U, 0x4289db6eU, 8, 0}, {0x440e4000U, 0x43b6d249U, 0x429c0000U, 0x4289db6eU, 8, 0},
                    {0x440c8000U, 0x43958db7U, 0x425c0000U, 0x4289db6eU, 8, 1}, {0x43f08000U, 0x4394bb6eU, 0x42740000U, 0x4289db6eU, 8, 1},
                    {0x4414c000U, 0x43d66000U, 0x429a0000U, 0x4289db6eU, 8, 0}, {0x4413c000U, 0x438d36dbU, 0x42500000U, 0x4289db6eU, 8, 1},
                    {0x43920000U, 0x43a5b249U, 0x428e0000U, 0x4289db6eU, 8, 0}, {0x42d40000U, 0xc2c00000U, 0x42b40000U, 0x4289db6eU, 0, 0},
                    {0x43b98000U, 0x43c4cdb7U, 0x428a0000U, 0x4289db6eU, 8, 0}, {0x43e98000U, 0x43e77249U, 0x42aa0000U, 0x4289db6eU, 8, 0},
                    {0x43fb8000U, 0x43d6a492U, 0x42900000U, 0x4289db6eU, 8, 1}, {0x43e00000U, 0xc3000000U, 0x42c00000U, 0x4289db6eU, 0, 1},
                    {0x43010000U, 0xc2a40000U, 0x428c0000U, 0x4289db6eU, 0, 0}, {0x43830000U, 0x43d156dbU, 0x42ae0000U, 0x4289db6eU, 8, 1},
                    {0x43a18000U, 0x43e9c492U, 0x429e0000U, 0x4289db6eU, 8, 0}, {0x439e0000U, 0x43e8adb7U, 0x42b40000U, 0x4289db6eU, 8, 0}},
                {1005999421, 76},
                {{"gfx/DChickleft2", {0, 496, 83, 62}, {41, 480}}, {"gfx/DChickleft2", {0, 496, 83, 62}, {569, 365}}, {"gfx/DChickright2", {0, 496, 83, 62}, {562, 299}},
                    {"gfx/DChickright2", {0, 496, 83, 62}, {481, 297}}, {"gfx/DChickleft2", {0, 496, 83, 62}, {595, 428}}, {"gfx/DChickright2", {0, 496, 83, 62}, {591, 282}},
                    {"gfx/DChickleft2", {0, 496, 83, 62}, {292, 331}}, {"gfx/DChickleft2", {0, 0, 83, 62}, {106, -96}}, {"gfx/DChickleft2", {0, 496, 83, 62}, {371, 393}},
                    {"gfx/DChickleft2", {0, 496, 83, 62}, {467, 462}}, {"gfx/DChickright2", {0, 496, 83, 62}, {503, 429}}, {"gfx/DChickright2", {0, 0, 83, 62}, {448, -128}},
                    {"gfx/DChickleft2", {0, 0, 83, 62}, {129, -82}}, {"gfx/DChickright2", {0, 496, 83, 62}, {262, 418}}, {"gfx/DChickleft2", {0, 496, 83, 62}, {323, 467}},
                    {"gfx/DChickleft2", {0, 496, 83, 62}, {316, 465}}}}}},
        {98, {1, 0},
            {{0x3e888888U,
                 {{0x42240000U, 0xc2993333U, 0x42a80000U, 0x402aaaaaU, 2, 0}, {0x440e4000U, 0xc3173333U, 0x429c0000U, 0x402aaaaaU, 2, 0},
                     {0x440c8000U, 0xc282aaabU, 0x425c0000U, 0x402aaaaaU, 2, 1}, {0x43f08000U, 0xc2d57778U, 0x42740000U, 0x402aaaaaU, 2, 1},
                     {0x4414c000U, 0xc2a2eeefU, 0x429a0000U, 0x402aaaaaU, 2, 0}, {0x4413c000U, 0xc2788889U, 0x42500000U, 0x402aaaaaU, 2, 1},
                     {0x43920000U, 0xc30b1111U, 0x428e0000U, 0x402aaaaaU, 2, 0}, {0x44018000U, 0xc2ba4444U, 0x42c20000U, 0x402aaaaaU, 2, 0},
                     {0x43b98000U, 0xc27e6667U, 0x428a0000U, 0x402aaaaaU, 2, 0}, {0x43e98000U, 0xc2c8aaabU, 0x42aa0000U, 0x402aaaaaU, 2, 0},
                     {0x43fb8000U, 0xc23f3334U, 0x42900000U, 0x402aaaaaU, 2, 1}, {0x43888000U, 0xc2bf7778U, 0x42b60000U, 0x402aaaaaU, 2, 1},
                     {0x42540000U, 0xc2f44444U, 0x42c20000U, 0x402aaaaaU, 2, 0}, {0x43830000U, 0xc31dcccdU, 0x42ae0000U, 0x402aaaaaU, 2, 1},
                     {0x43a18000U, 0xc25fbbbcU, 0x429e0000U, 0x402aaaaaU, 2, 0}, {0x439e0000U, 0xc3030000U, 0x42b40000U, 0x402aaaaaU, 2, 0}},
                 {120733505, 64},
                 {{"gfx/DChickleft2", {0, 124, 83, 62}, {41, -76}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {569, -151}}, {"gfx/DChickright2", {0, 124, 83, 62}, {562, -65}},
                     {"gfx/DChickright2", {0, 124, 83, 62}, {481, -106}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {595, -81}}, {"gfx/DChickright2", {0, 124, 83, 62}, {591, -62}},
                     {"gfx/DChickleft2", {0, 124, 83, 62}, {292, -139}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {518, -93}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {371, -63}},
                     {"gfx/DChickleft2", {0, 124, 83, 62}, {467, -100}}, {"gfx/DChickright2", {0, 124, 83, 62}, {503, -47}}, {"gfx/DChickright2", {0, 124, 83, 62}, {273, -95}},
                     {"gfx/DChickleft2", {0, 124, 83, 62}, {53, -122}}, {"gfx/DChickright2", {0, 124, 83, 62}, {262, -157}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {323, -55}},
                     {"gfx/DChickleft2", {0, 124, 83, 62}, {316, -131}}}},
                {0x32800001U,
                    {{0x42240000U, 0xc2993333U, 0x42a80000U, 0x402aaaabU, 2, 0}, {0x440e4000U, 0xc3173333U, 0x429c0000U, 0x402aaaabU, 2, 0},
                        {0x440c8000U, 0xc282aaabU, 0x425c0000U, 0x402aaaabU, 2, 1}, {0x43f08000U, 0xc2d57778U, 0x42740000U, 0x402aaaabU, 2, 1},
                        {0x4414c000U, 0xc2a2eeefU, 0x429a0000U, 0x402aaaabU, 2, 0}, {0x4413c000U, 0xc2788889U, 0x42500000U, 0x402aaaabU, 2, 1},
                        {0x43920000U, 0xc30b1111U, 0x428e0000U, 0x402aaaabU, 2, 0}, {0x44018000U, 0xc2ba4444U, 0x42c20000U, 0x402aaaabU, 2, 0},
                        {0x43b98000U, 0xc27e6667U, 0x428a0000U, 0x402aaaabU, 2, 0}, {0x43e98000U, 0xc2c8aaabU, 0x42aa0000U, 0x402aaaabU, 2, 0},
                        {0x43fb8000U, 0xc23f3334U, 0x42900000U, 0x402aaaabU, 2, 1}, {0x43888000U, 0xc2bf7778U, 0x42b60000U, 0x402aaaabU, 2, 1},
                        {0x42540000U, 0xc2f44444U, 0x42c20000U, 0x402aaaabU, 2, 0}, {0x43830000U, 0xc31dcccdU, 0x42ae0000U, 0x402aaaabU, 2, 1},
                        {0x43a18000U, 0xc25fbbbcU, 0x429e0000U, 0x402aaaabU, 2, 0}, {0x439e0000U, 0xc3030000U, 0x42b40000U, 0x402aaaabU, 2, 0}},
                    {120733505, 64},
                    {{"gfx/DChickleft2", {0, 124, 83, 62}, {41, -76}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {569, -151}}, {"gfx/DChickright2", {0, 124, 83, 62}, {562, -65}},
                        {"gfx/DChickright2", {0, 124, 83, 62}, {481, -106}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {595, -81}}, {"gfx/DChickright2", {0, 124, 83, 62}, {591, -62}},
                        {"gfx/DChickleft2", {0, 124, 83, 62}, {292, -139}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {518, -93}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {371, -63}},
                        {"gfx/DChickleft2", {0, 124, 83, 62}, {467, -100}}, {"gfx/DChickright2", {0, 124, 83, 62}, {503, -47}}, {"gfx/DChickright2", {0, 124, 83, 62}, {273, -95}},
                        {"gfx/DChickleft2", {0, 124, 83, 62}, {53, -122}}, {"gfx/DChickright2", {0, 124, 83, 62}, {262, -157}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {323, -55}},
                        {"gfx/DChickleft2", {0, 124, 83, 62}, {316, -131}}}},
                {0x00000000U,
                    {{0x42240000U, 0xc2993333U, 0x42a80000U, 0x402aaaabU, 2, 0}, {0x440e4000U, 0xc3173333U, 0x429c0000U, 0x402aaaabU, 2, 0},
                        {0x440c8000U, 0xc282aaabU, 0x425c0000U, 0x402aaaabU, 2, 1}, {0x43f08000U, 0xc2d57778U, 0x42740000U, 0x402aaaabU, 2, 1},
                        {0x4414c000U, 0xc2a2eeefU, 0x429a0000U, 0x402aaaabU, 2, 0}, {0x4413c000U, 0xc2788889U, 0x42500000U, 0x402aaaabU, 2, 1},
                        {0x43920000U, 0xc30b1111U, 0x428e0000U, 0x402aaaabU, 2, 0}, {0x44018000U, 0xc2ba4444U, 0x42c20000U, 0x402aaaabU, 2, 0},
                        {0x43b98000U, 0xc27e6667U, 0x428a0000U, 0x402aaaabU, 2, 0}, {0x43e98000U, 0xc2c8aaabU, 0x42aa0000U, 0x402aaaabU, 2, 0},
                        {0x43fb8000U, 0xc23f3334U, 0x42900000U, 0x402aaaabU, 2, 1}, {0x43888000U, 0xc2bf7778U, 0x42b60000U, 0x402aaaabU, 2, 1},
                        {0x42540000U, 0xc2f44444U, 0x42c20000U, 0x402aaaabU, 2, 0}, {0x43830000U, 0xc31dcccdU, 0x42ae0000U, 0x402aaaabU, 2, 1},
                        {0x43a18000U, 0xc25fbbbcU, 0x429e0000U, 0x402aaaabU, 2, 0}, {0x439e0000U, 0xc3030000U, 0x42b40000U, 0x402aaaabU, 2, 0}},
                    {120733505, 64},
                    {{"gfx/DChickleft2", {0, 124, 83, 62}, {41, -76}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {569, -151}}, {"gfx/DChickright2", {0, 124, 83, 62}, {562, -65}},
                        {"gfx/DChickright2", {0, 124, 83, 62}, {481, -106}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {595, -81}}, {"gfx/DChickright2", {0, 124, 83, 62}, {591, -62}},
                        {"gfx/DChickleft2", {0, 124, 83, 62}, {292, -139}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {518, -93}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {371, -63}},
                        {"gfx/DChickleft2", {0, 124, 83, 62}, {467, -100}}, {"gfx/DChickright2", {0, 124, 83, 62}, {503, -47}}, {"gfx/DChickright2", {0, 124, 83, 62}, {273, -95}},
                        {"gfx/DChickleft2", {0, 124, 83, 62}, {53, -122}}, {"gfx/DChickright2", {0, 124, 83, 62}, {262, -157}}, {"gfx/DChickleft2", {0, 124, 83, 62}, {323, -55}},
                        {"gfx/DChickleft2", {0, 124, 83, 62}, {316, -131}}}}}}}};

using moorhuhn2::ui::House88Results;
using moorhuhn2::ui::ResultsRandom;

using moorhuhn2::test::require;

float binary32(std::uint32_t value) {
    return std::bit_cast<float>(value);
}

void check(const House88Results& screen, const ResultsRandom& random, const auto& expected) {
    require(random == expected.random_after, "Source RNG state/count differs");
    for (std::size_t index = 0; index < screen.records().size(); ++index) {
        const auto& actual = screen.records()[index];
        const auto& record = expected.records[index];
        require(std::bit_cast<std::uint32_t>(actual.x) == record.x_bits && std::bit_cast<std::uint32_t>(actual.y) == record.y_bits
                    && std::bit_cast<std::uint32_t>(actual.speed) == record.speed_bits && std::bit_cast<std::uint32_t>(actual.animation_time) == record.animation_time_bits
                    && actual.frame == record.frame && actual.variant == record.variant,
            "Source chicken record differs");
    }

    const auto before = random;
    for (int repeat = 0; repeat < 2; ++repeat) {
        const auto draws = screen.draws();
        for (std::size_t index = 0; index < draws.size(); ++index) {
            const auto& actual = draws[index];
            const auto& draw = expected.draws[index];
            const auto& source = draw.source;
            const auto& destination = draw.destination;
            require(actual.image == draw.image && actual.source.x == source[0] && actual.source.y == source[1] && actual.source.width == source[2]
                        && actual.source.height == source[3] && actual.destination.x == destination[0] && actual.destination.y == destination[1] && actual.color_key
                        && !actual.mirror_x,
                "Original middle dying art, integer placement or ascending painter order differs");
        }
    }

    require(random == before, "Drawing must not consume RNG");
}
} // namespace

int main() {
    try {
        for (const auto& expected : initializations) {
            auto random = expected.random_before;
            const House88Results screen(expected.actual_score, random);
            check(screen, random, expected);
        }

        auto threshold_random = initializations[0].random_before;
        const House88Results below_threshold(1499, threshold_random);
        check(below_threshold, threshold_random, initializations[0]);
        threshold_random = initializations[0].random_before;
        const House88Results negative_score(-1, threshold_random);
        check(negative_score, threshold_random, initializations[0]);
        for (const auto& sequence : sequences) {
            auto random = sequence.random_before;
            House88Results screen(sequence.actual_score, random);
            for (const auto& expected : sequence.steps) {
                screen.update(binary32(expected.delta_bits), random);
                check(screen, random, expected);
            }
        }

        auto random = initializations[0].random_before;
        House88Results screen(98, random);
        for (const auto delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), 2147483648.0F}) {
            bool rejected = false;
            try {
                screen.update(delta, random);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }

            require(rejected, "Invalid delta must be rejected");
            check(screen, random, initializations[0]);
        }
        std::cout << "House88 results source outcomes passed: init/threshold, 10 updates, recycle/RNG/art and invalid input\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
