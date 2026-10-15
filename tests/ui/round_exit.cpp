#include "ui/round_exit.hpp"

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

struct TileExpectation {
    std::size_t index{};
    std::uint32_t x_bits{};
    std::uint32_t speed_bits{};
    int state{};
};

struct ExitExpectation {
    std::uint32_t reveal_time_bits{};
    std::size_t next_tile{};
    bool finished{};
    std::size_t active{};
    std::vector<TileExpectation> records{};
};

struct StepCase {
    std::uint32_t delta_bits{};
    int repeat{};
    bool skip{};
    ExitExpectation expected{};
};

struct SequenceCase {
    std::string id{};
    std::vector<StepCase> steps{};
};

const ExitExpectation initial = {0x00000000U, 0, false, 0,
    {{0, 0xc4200000U, 0x41a00000U, 0}, {1, 0xc41b0000U, 0x41a00000U, 0}, {12, 0xc3c80000U, 0x41a00000U, 0}, {13, 0xc3be0000U, 0x41a00000U, 0}, {31, 0xc1a00000U, 0x41a00000U, 0},
        {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0}, {736, 0x44200000U, 0x41a00000U, 0},
        {767, 0x449d8000U, 0x41a00000U, 0}}};

const std::array<SequenceCase, 8> sequences = {
    {{"strict-retained-reveal",
         {{0x3c4ccccdU, 1, false,
              {0x3f400000U, 0, false, 0,
                  {{0, 0xc4200000U, 0x41a00000U, 0}, {1, 0xc41b0000U, 0x41a00000U, 0}, {12, 0xc3c80000U, 0x41a00000U, 0}, {13, 0xc3be0000U, 0x41a00000U, 0},
                      {31, 0xc1a00000U, 0x41a00000U, 0}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                      {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
             {0x3b888888U, 1, false,
                 {0x3f800000U, 0, false, 0,
                     {{0, 0xc4200000U, 0x41a00000U, 0}, {1, 0xc41b0000U, 0x41a00000U, 0}, {12, 0xc3c80000U, 0x41a00000U, 0}, {13, 0xc3be0000U, 0x41a00000U, 0},
                         {31, 0xc1a00000U, 0x41a00000U, 0}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
             {0x00000000U, 1, false,
                 {0x3f800000U, 0, false, 0,
                     {{0, 0xc4200000U, 0x41a00000U, 0}, {1, 0xc41b0000U, 0x41a00000U, 0}, {12, 0xc3c80000U, 0x41a00000U, 0}, {13, 0xc3be0000U, 0x41a00000U, 0},
                         {31, 0xc1a00000U, 0x41a00000U, 0}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
             {0x2f800000U, 1, false,
                 {0x00000000U, 32, false, 32,
                     {{0, 0xc4200000U, 0x41a00000U, 1}, {1, 0xc41b0000U, 0x41a00000U, 1}, {12, 0xc3c80000U, 0x41a00000U, 1}, {13, 0xc3be0000U, 0x41a00000U, 1},
                         {31, 0xc1a00000U, 0x41a00000U, 1}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
             {0x00000000U, 1, false,
                 {0x00000000U, 32, false, 32,
                     {{0, 0xc4200000U, 0x41a00000U, 1}, {1, 0xc41b0000U, 0x41a00000U, 1}, {12, 0xc3c80000U, 0x41a00000U, 1}, {13, 0xc3be0000U, 0x41a00000U, 1},
                         {31, 0xc1a00000U, 0x41a00000U, 1}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
             {0x3c888889U, 1, false,
                 {0x00000000U, 64, false, 64,
                     {{0, 0xc417aaabU, 0x419f5555U, 1}, {1, 0xc412aaabU, 0x419f5555U, 1}, {12, 0xc3b75555U, 0x419f5555U, 1}, {13, 0xc3ad5555U, 0x419f5555U, 1},
                         {31, 0x41555557U, 0x419f5555U, 1}, {32, 0x4417aaabU, 0x419f5555U, 1}, {63, 0x44995555U, 0x419f5555U, 1}, {64, 0xc4200000U, 0x41a00000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}}}},
        {"small-update-row-order",
            {{0x3c800000U, 1, false,
                 {0x3f700000U, 0, false, 0,
                     {{0, 0xc4200000U, 0x41a00000U, 0}, {1, 0xc41b0000U, 0x41a00000U, 0}, {12, 0xc3c80000U, 0x41a00000U, 0}, {13, 0xc3be0000U, 0x41a00000U, 0},
                         {31, 0xc1a00000U, 0x41a00000U, 0}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x3c800000U, 1, false,
                    {0x3f600000U, 32, false, 32,
                        {{0, 0xc4183000U, 0x419f6000U, 1}, {1, 0xc4133000U, 0x419f6000U, 1}, {12, 0xc3b86000U, 0x419f6000U, 1}, {13, 0xc3ae6000U, 0x419f6000U, 1},
                            {31, 0x41340000U, 0x419f6000U, 1}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                            {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x3c800000U, 2, false,
                    {0x3f400000U, 96, false, 96,
                        {{0, 0xc408a770U, 0x419e2000U, 1}, {1, 0xc403a770U, 0x419e2000U, 1}, {12, 0xc3994ee0U, 0x419e2000U, 1}, {13, 0xc38f4ee0U, 0x419e2000U, 1},
                            {31, 0x4292c480U, 0x419e2000U, 1}, {32, 0x441067d0U, 0x419ec000U, 1}, {63, 0x4495b3e8U, 0x419ec000U, 1}, {64, 0xc4183000U, 0x419f6000U, 1},
                            {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x3c800000U, 22, false,
                    {0x3ec00000U, 768, false, 640,
                        {{0, 0x00000000U, 0x41924000U, 0}, {1, 0x41a00000U, 0x41924000U, 0}, {12, 0x43700000U, 0x41924000U, 0}, {13, 0x43820000U, 0x41924000U, 0},
                            {31, 0x441b0000U, 0x41924000U, 0}, {32, 0x00000000U, 0x41924000U, 0}, {63, 0x441b0000U, 0x41924000U, 0}, {64, 0x00000000U, 0x41924000U, 0},
                            {736, 0x44183000U, 0x419f6000U, 1}, {767, 0x44999800U, 0x419f6000U, 1}}}}}},
        {"all-rows-natural-finish",
            {{0x3e000000U, 23, false,
                 {0x43158000U, 736, false, 64,
                     {{0, 0x00000000U, 0x41910000U, 0}, {1, 0x41a00000U, 0x41910000U, 0}, {12, 0x43700000U, 0x41910000U, 0}, {13, 0x43820000U, 0x41910000U, 0},
                         {31, 0x441b0000U, 0x41910000U, 0}, {32, 0x00000000U, 0x41910000U, 0}, {63, 0x441b0000U, 0x41910000U, 0}, {64, 0x00000000U, 0x41910000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x3e000000U, 1, false,
                    {0x431c0000U, 768, false, 64,
                        {{0, 0x00000000U, 0x41910000U, 0}, {1, 0x41a00000U, 0x41910000U, 0}, {12, 0x43700000U, 0x41910000U, 0}, {13, 0x43820000U, 0x41910000U, 0},
                            {31, 0x441b0000U, 0x41910000U, 0}, {32, 0x00000000U, 0x41910000U, 0}, {63, 0x441b0000U, 0x41910000U, 0}, {64, 0x00000000U, 0x41910000U, 0},
                            {736, 0x43c30000U, 0x419b0000U, 1}, {767, 0x447c8000U, 0x419b0000U, 1}}}},
                {0x3e000000U, 1, false,
                    {0x43228000U, 768, false, 32,
                        {{0, 0x00000000U, 0x41910000U, 0}, {1, 0x41a00000U, 0x41910000U, 0}, {12, 0x43700000U, 0x41910000U, 0}, {13, 0x43820000U, 0x41910000U, 0},
                            {31, 0x441b0000U, 0x41910000U, 0}, {32, 0x00000000U, 0x41910000U, 0}, {63, 0x441b0000U, 0x41910000U, 0}, {64, 0x00000000U, 0x41910000U, 0},
                            {736, 0x4313d000U, 0x41960000U, 1}, {767, 0x443ff400U, 0x41960000U, 1}}}},
                {0x3e000000U, 1, false,
                    {0x43290000U, 768, true, 0,
                        {{0, 0x00000000U, 0x41910000U, 0}, {1, 0x41a00000U, 0x41910000U, 0}, {12, 0x43700000U, 0x41910000U, 0}, {13, 0x43820000U, 0x41910000U, 0},
                            {31, 0x441b0000U, 0x41910000U, 0}, {32, 0x00000000U, 0x41910000U, 0}, {63, 0x441b0000U, 0x41910000U, 0}, {64, 0x00000000U, 0x41910000U, 0},
                            {736, 0x00000000U, 0x41910000U, 0}, {767, 0x441b0000U, 0x41910000U, 0}}}},
                {0x3f800000U, 1, false,
                    {0x43290000U, 768, true, 0,
                        {{0, 0x00000000U, 0x41910000U, 0}, {1, 0x41a00000U, 0x41910000U, 0}, {12, 0x43700000U, 0x41910000U, 0}, {13, 0x43820000U, 0x41910000U, 0},
                            {31, 0x441b0000U, 0x41910000U, 0}, {32, 0x00000000U, 0x41910000U, 0}, {63, 0x441b0000U, 0x41910000U, 0}, {64, 0x00000000U, 0x41910000U, 0},
                            {736, 0x00000000U, 0x41910000U, 0}, {767, 0x441b0000U, 0x41910000U, 0}}}}}},
        {"debt-saturation-stored-completion",
            {{0x3f800000U, 1, false,
                 {0x426c0000U, 32, false, 0,
                     {{0, 0x00000000U, 0x41700000U, 0}, {1, 0x41a00000U, 0x41700000U, 0}, {12, 0x43700000U, 0x41700000U, 0}, {13, 0x43820000U, 0x41700000U, 0},
                         {31, 0x441b0000U, 0x41700000U, 0}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x00000000U, 22, false,
                    {0x42140000U, 736, false, 704,
                        {{0, 0x00000000U, 0x41700000U, 0}, {1, 0x41a00000U, 0x41700000U, 0}, {12, 0x43700000U, 0x41700000U, 0}, {13, 0x43820000U, 0x41700000U, 0},
                            {31, 0x441b0000U, 0x41700000U, 0}, {32, 0x44200000U, 0x41a00000U, 1}, {63, 0x449d8000U, 0x41a00000U, 1}, {64, 0xc4200000U, 0x41a00000U, 1},
                            {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x00000000U, 1, false,
                    {0x42100000U, 768, false, 736,
                        {{0, 0x00000000U, 0x41700000U, 0}, {1, 0x41a00000U, 0x41700000U, 0}, {12, 0x43700000U, 0x41700000U, 0}, {13, 0x43820000U, 0x41700000U, 0},
                            {31, 0x441b0000U, 0x41700000U, 0}, {32, 0x44200000U, 0x41a00000U, 1}, {63, 0x449d8000U, 0x41a00000U, 1}, {64, 0xc4200000U, 0x41a00000U, 1},
                            {736, 0x44200000U, 0x41a00000U, 1}, {767, 0x449d8000U, 0x41a00000U, 1}}}},
                {0x00000000U, 2, false,
                    {0x42080000U, 768, false, 736,
                        {{0, 0x00000000U, 0x41700000U, 0}, {1, 0x41a00000U, 0x41700000U, 0}, {12, 0x43700000U, 0x41700000U, 0}, {13, 0x43820000U, 0x41700000U, 0},
                            {31, 0x441b0000U, 0x41700000U, 0}, {32, 0x44200000U, 0x41a00000U, 1}, {63, 0x449d8000U, 0x41a00000U, 1}, {64, 0xc4200000U, 0x41a00000U, 1},
                            {736, 0x44200000U, 0x41a00000U, 1}, {767, 0x449d8000U, 0x41a00000U, 1}}}},
                {0x3e800000U, 1, false,
                    {0x42400000U, 768, false, 736,
                        {{0, 0x00000000U, 0x41700000U, 0}, {1, 0x41a00000U, 0x41700000U, 0}, {12, 0x43700000U, 0x41700000U, 0}, {13, 0x43820000U, 0x41700000U, 0},
                            {31, 0x441b0000U, 0x41700000U, 0}, {32, 0x430c0000U, 0x41960000U, 1}, {63, 0x443e0000U, 0x41960000U, 1}, {64, 0xc30c0000U, 0x41960000U, 1},
                            {736, 0x430c0000U, 0x41960000U, 1}, {767, 0x443e0000U, 0x41960000U, 1}}}},
                {0x3d98ead6U, 1, false,
                    {0x424deb85U, 768, true, 161,
                        {{0, 0x00000000U, 0x41700000U, 0}, {1, 0x41a00000U, 0x41700000U, 0}, {12, 0x43700000U, 0x41700000U, 0}, {13, 0x43820000U, 0x41700000U, 0},
                            {31, 0x441b0000U, 0x41700000U, 0}, {32, 0x36a78000U, 0x4193036aU, 1}, {63, 0x441b0000U, 0x4193036aU, 0}, {64, 0xb6a78000U, 0x4193036aU, 1},
                            {736, 0x36a78000U, 0x4193036aU, 1}, {767, 0x441b0000U, 0x4193036aU, 0}}}}}},
        {"stored-target-clamp",
            {{0x3ea3d70aU, 1, false,
                 {0x41919999U, 32, false, 13,
                     {{0, 0xb7700000U, 0x41933333U, 1}, {1, 0x419ffff8U, 0x41933333U, 1}, {12, 0x436fffffU, 0x41933333U, 1}, {13, 0x43820000U, 0x41933333U, 0},
                         {31, 0x441b0000U, 0x41933333U, 0}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                         {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x00000000U, 1, false,
                    {0x41899999U, 64, false, 45,
                        {{0, 0xb7700000U, 0x41933333U, 1}, {1, 0x419ffff8U, 0x41933333U, 1}, {12, 0x436fffffU, 0x41933333U, 1}, {13, 0x43820000U, 0x41933333U, 0},
                            {31, 0x441b0000U, 0x41933333U, 0}, {32, 0x44200000U, 0x41a00000U, 1}, {63, 0x449d8000U, 0x41a00000U, 1}, {64, 0xc4200000U, 0x41a00000U, 0},
                            {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x30800000U, 1, false,
                    {0x41819999U, 96, false, 77,
                        {{0, 0xb7534000U, 0x41933333U, 1}, {1, 0x419ffff9U, 0x41933333U, 1}, {12, 0x436fffffU, 0x41933333U, 1}, {13, 0x43820000U, 0x41933333U, 0},
                            {31, 0x441b0000U, 0x41933333U, 0}, {32, 0x44200000U, 0x41a00000U, 1}, {63, 0x449d8000U, 0x41a00000U, 1}, {64, 0xc4200000U, 0x41a00000U, 1},
                            {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                {0x3d800000U, 1, false,
                    {0x41979999U, 128, false, 96,
                        {{0, 0x00000000U, 0x4190b333U, 0}, {1, 0x41a00000U, 0x4190b333U, 0}, {12, 0x43700000U, 0x4190b333U, 0}, {13, 0x43820000U, 0x41933333U, 0},
                            {31, 0x441b0000U, 0x41933333U, 0}, {32, 0x4400c000U, 0x419d8000U, 1}, {63, 0x448de000U, 0x419d8000U, 1}, {64, 0xc400c000U, 0x419d8000U, 1},
                            {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}}}},
        {"speed-floor", {{0x40800000U, 1, false,
                             {0x436f0000U, 32, false, 0,
                                 {{0, 0x00000000U, 0x40400000U, 0}, {1, 0x41a00000U, 0x40400000U, 0}, {12, 0x43700000U, 0x40400000U, 0}, {13, 0x43820000U, 0x40400000U, 0},
                                     {31, 0x441b0000U, 0x40400000U, 0}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                                     {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                            {0x00000000U, 1, false,
                                {0x436e0000U, 64, false, 32,
                                    {{0, 0x00000000U, 0x40400000U, 0}, {1, 0x41a00000U, 0x40400000U, 0}, {12, 0x43700000U, 0x40400000U, 0}, {13, 0x43820000U, 0x40400000U, 0},
                                        {31, 0x441b0000U, 0x40400000U, 0}, {32, 0x44200000U, 0x41a00000U, 1}, {63, 0x449d8000U, 0x41a00000U, 1}, {64, 0xc4200000U, 0x41a00000U, 0},
                                        {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                            {0x40800000U, 1, true,
                                {0x43ee8000U, 96, true, 0,
                                    {{0, 0x00000000U, 0x40400000U, 0}, {1, 0x41a00000U, 0x40400000U, 0}, {12, 0x43700000U, 0x40400000U, 0}, {13, 0x43820000U, 0x40400000U, 0},
                                        {31, 0x441b0000U, 0x40400000U, 0}, {32, 0x00000000U, 0x40400000U, 0}, {63, 0x441b0000U, 0x40400000U, 0}, {64, 0x00000000U, 0x40400000U, 0},
                                        {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}}}},
        {"skip-motion", {{0x3e000000U, 1, true,
                             {0x40d00000U, 32, true, 32,
                                 {{0, 0xc3c30000U, 0x419b0000U, 1}, {1, 0xc3b90000U, 0x419b0000U, 1}, {12, 0xc3160000U, 0x419b0000U, 1}, {13, 0xc3020000U, 0x419b0000U, 1},
                                     {31, 0x43660000U, 0x419b0000U, 1}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                                     {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}},
                            {0x3f800000U, 1, false,
                                {0x40d00000U, 32, true, 32,
                                    {{0, 0xc3c30000U, 0x419b0000U, 1}, {1, 0xc3b90000U, 0x419b0000U, 1}, {12, 0xc3160000U, 0x419b0000U, 1}, {13, 0xc3020000U, 0x419b0000U, 1},
                                        {31, 0x43660000U, 0x419b0000U, 1}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                                        {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}}}},
        {"zero-delta-skip", {{0x00000000U, 1, true,
                                {0x00000000U, 0, true, 0,
                                    {{0, 0xc4200000U, 0x41a00000U, 0}, {1, 0xc41b0000U, 0x41a00000U, 0}, {12, 0xc3c80000U, 0x41a00000U, 0}, {13, 0xc3be0000U, 0x41a00000U, 0},
                                        {31, 0xc1a00000U, 0x41a00000U, 0}, {32, 0x44200000U, 0x41a00000U, 0}, {63, 0x449d8000U, 0x41a00000U, 0}, {64, 0xc4200000U, 0x41a00000U, 0},
                                        {736, 0x44200000U, 0x41a00000U, 0}, {767, 0x449d8000U, 0x41a00000U, 0}}}}}}}};

using moorhuhn2::ui::RoundExit;
using moorhuhn2::ui::RoundExitTile;

using moorhuhn2::test::require;

float binary32(std::uint32_t value) {
    return std::bit_cast<float>(value);
}

std::uint32_t bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Snapshot {
    std::array<RoundExitTile, 768> tiles{};
    std::size_t next_tile{};
    float reveal_time{};
    bool finished{};

    bool operator==(const Snapshot&) const = default;
};

Snapshot snapshot(const RoundExit& transition) {
    Snapshot result{{}, transition.next_tile(), transition.reveal_time(), transition.finished()};
    std::ranges::copy(transition.records(), result.tiles.begin());

    return result;
}

void check_summary(const RoundExit& transition, const ExitExpectation& expected) {
    require(bits(transition.reveal_time()) == expected.reveal_time_bits, "Source reveal accumulator bits differ");
    require(transition.next_tile() == expected.next_tile, "Source saturated reveal index differs");
    require(transition.finished() == expected.finished, "Source last-tile or skip completion differs");
    for (const auto& record : expected.records) {
        const auto& tile = transition.records()[record.index];
        require(bits(tile.x) == record.x_bits && bits(tile.speed) == record.speed_bits && tile.state == record.state,
            "Source stored tile X, old-speed movement, speed reduction or target state differs");
    }
    std::size_t active = 0;
    for (std::size_t index = 0; index < transition.records().size(); ++index) {
        const auto& tile = transition.records()[index];
        const float target = static_cast<float>(index % 32 * 20);
        const bool from_left = index / 32 % 2 == 0;
        const float initial = target + (from_left ? -640.0F : 640.0F);
        require(tile.y == static_cast<float>(index / 32 * 20) && tile.target_x == target && tile.frame == static_cast<int>(index),
            "Round grid Y, target and frame must remain in original row-major order");
        require(tile.x >= std::min(initial, target) && tile.x <= std::max(initial, target) && tile.speed >= 3 && tile.speed <= 20 && (tile.state == 0 || tile.state == 1),
            "Each original tile must stay within its own horizontal travel and speed bounds");
        if (index >= transition.next_tile()) {
            require(tile.x == initial && tile.speed == 20 && tile.state == 0, "Unrevealed records must remain at their alternating offscreen origins");
        }
        active += tile.state != 0;
    }

    require(active == expected.active, "Source active count must allow both newly revealed and settled records");
}

void check_draws(const RoundExit& transition) {
    const auto before = snapshot(transition);
    const auto draws = transition.draws();
    require(draws.size() == 768, "Round exit must draw every score tile and no HUD, cursor or logo");
    for (std::size_t index = 0; index < draws.size(); ++index) {
        const auto& tile = transition.records()[index];
        const auto& draw = draws[index];
        require(draw.image == "gfx/scorescreen" && draw.source.x == tile.frame % 32 * 20 && draw.source.y == tile.frame / 32 * 20 && draw.source.width == 20
                    && draw.source.height == 20 && draw.destination.x == static_cast<int>(tile.x) && draw.destination.y == static_cast<int>(tile.y) && draw.color_key
                    && !draw.mirror_x,
            "Ascending painter order,20×20 source cells or stored-pose truncation toward zero differs");
    }

    require(snapshot(transition) == before, "Round exit drawing must be pure");
}

void check_sequence(const SequenceCase& sequence, const ExitExpectation& initial) {
    RoundExit transition;
    check_summary(transition, initial);
    check_draws(transition);
    for (const auto& step : sequence.steps) {
        for (int repeat = 0; repeat < step.repeat; ++repeat) {
            const auto previous_index = transition.next_tile();
            const auto before = snapshot(transition);
            const bool finished = transition.update(binary32(step.delta_bits), step.skip);
            require(finished == transition.finished(), "Update must return completion after its final presented state");
            require(transition.next_tile() <= std::min(previous_index + 32, std::size_t{768}), "One update must never catch up more than one 32-record group");
            if (before.finished) {
                require(snapshot(transition) == before, "Completed round exits must not perform another update");
            }
        }

        check_summary(transition, step.expected);
        check_draws(transition);
    }

    const RoundExit fresh;
    check_summary(fresh, initial);
    check_draws(fresh);
}

void invalid_inputs() {
    RoundExit transition;
    for (int phase = 0; phase < 2; ++phase) {
        const auto before = snapshot(transition);
        for (const float delta :
            {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() / 64}) {
            bool rejected = false;
            try {
                transition.update(delta, true);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }

            require(rejected && snapshot(transition) == before, "Negative, nonfinite or overflowing updates must reject skip and all mutation atomically");
        }

        transition.update(0.125F);
    }
}

} // namespace

int main() {
    try {
        for (const auto& sequence : sequences) {
            try {
                check_sequence(sequence, initial);
            } catch (const std::exception& error) {
                throw std::runtime_error(sequence.id + ": " + error.what());
            }
        }

        invalid_inputs();
        std::cout
            << "PASS: round exit,8 exact source sequences, alternating rows, retained reveal/debt, saturated index, stored target completion, speed floor, final skip and draws\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
