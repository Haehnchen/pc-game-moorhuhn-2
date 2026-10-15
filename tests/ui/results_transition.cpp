#include "ui/results_transition.hpp"

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
    std::uint32_t y_bits{};
    std::uint32_t velocity_y_bits{};
};

struct TransitionExpectation {
    std::uint32_t row_offset_bits{};
    std::uint32_t row_speed_bits{};
    std::uint32_t reveal_time_bits{};
    std::size_t next_column{};
    bool exiting{};
    bool finished{};
    std::vector<TileExpectation> records{};
};

struct EntryStep {
    std::uint32_t delta_bits{};
    TransitionExpectation expected{};
};

struct ExitStep {
    std::uint32_t delta_bits{};
    int repeat{};
    bool skip{};
    TransitionExpectation expected{};
};

struct SequenceCase {
    std::string id{};
    std::vector<EntryStep> entry_steps{};
    TransitionExpectation exit_initial{};
    std::vector<ExitStep> exit_steps{};
};

const TransitionExpectation initial = {0x44700000U, 0x41200000U, 0x00000000U, 0, false, false,
    {{0, 0x00000000U, 0x00000000U}, {1, 0x00000000U, 0x00000000U}, {2, 0x00000000U, 0x00000000U}, {30, 0x00000000U, 0x00000000U}, {31, 0x00000000U, 0x00000000U},
        {32, 0x41a00000U, 0x00000000U}, {63, 0x41a00000U, 0x00000000U}, {736, 0x43e60000U, 0x00000000U}, {767, 0x43e60000U, 0x00000000U}}};

const std::array<SequenceCase, 7> sequences = {{{"early-exit",
                                                    {{0x3e000000U, {0x4450c000U, 0x41160000U, 0x00000000U, 0, false, false}},
                                                        {0x3e800000U, {0x44162800U, 0x41020000U, 0x00000000U, 0, false, false}}},
                                                    {0x44162800U, 0x41020000U, 0x00000000U, 0, true, false,
                                                        {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U},
                                                            {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U}, {32, 0x41a00000U, 0x3f800000U},
                                                            {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}},
                                                    {{0x3e000000U, 2, false,
                                                         {0x43bef000U, 0x412a0000U, 0x3fe00000U, 2, true, false,
                                                             {{0, 0x42610000U, 0x40c00000U}, {1, 0x41480000U, 0x40600000U}, {2, 0x00000000U, 0x3f800000U},
                                                                 {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U}, {32, 0x42988000U, 0x40c00000U},
                                                                 {63, 0x41a00000U, 0x3f800000U}, {736, 0x44011000U, 0x40c00000U}, {767, 0x43e60000U, 0x3f800000U}}}},
                                                        {0x00000000U, 1, false,
                                                            {0x43bef000U, 0x412a0000U, 0x3f400000U, 3, true, false,
                                                                {{0, 0x42610000U, 0x40c00000U}, {1, 0x41480000U, 0x40600000U}, {2, 0x00000000U, 0x3f800000U},
                                                                    {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U}, {32, 0x42988000U, 0x40c00000U},
                                                                    {63, 0x41a00000U, 0x3f800000U}, {736, 0x44011000U, 0x40c00000U}, {767, 0x43e60000U, 0x3f800000U}}}},
                                                        {0x3f000000U, 4, false,
                                                            {0xc54ab200U, 0x41f00000U, 0x41d60000U, 7, true, false,
                                                                {{0, 0x45850200U, 0x42380000U}, {1, 0x45680800U, 0x422e0000U}, {2, 0x45480000U, 0x42240000U},
                                                                    {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U}, {32, 0x4585a200U, 0x42380000U},
                                                                    {63, 0x41a00000U, 0x3f800000U}, {736, 0x45936200U, 0x42380000U}, {767, 0x43e60000U, 0x3f800000U}}}},
                                                        {0x3d800000U, 1, true,
                                                            {0xc5566a00U, 0x41f00000U, 0x41d58000U, 8, true, true,
                                                                {{0, 0x458dfe00U, 0x423d0000U}, {1, 0x45790600U, 0x42330000U}, {2, 0x45580400U, 0x42290000U},
                                                                    {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U}, {32, 0x458e9e00U, 0x423d0000U},
                                                                    {63, 0x41a00000U, 0x3f800000U}, {736, 0x459c5e00U, 0x423d0000U}, {767, 0x43e60000U, 0x3f800000U}}}}}},
    {"settled-entry",
        {{0x3e000000U, {0x4450c000U, 0x41160000U, 0x00000000U, 0, false, false}}, {0x3e800000U, {0x44162800U, 0x41020000U, 0x00000000U, 0, false, false}},
            {0x3f000000U, {0x43426000U, 0x40b40000U, 0x00000000U, 0, false, false}}, {0x3e800000U, {0x42570000U, 0x408c0000U, 0x00000000U, 0, false, false}},
            {0x3e800000U, {0x00000000U, 0x40480000U, 0x00000000U, 0, false, false}}, {0x3f000000U, {0x00000000U, 0x3f800000U, 0x00000000U, 0, false, false}},
            {0x00000000U, {0x00000000U, 0x3f800000U, 0x00000000U, 0, false, false}}},
        {0x00000000U, 0x3f800000U, 0x00000000U, 0, true, false,
            {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}},
        {{0x3e000000U, 2, false,
            {0xc2228000U, 0x40600000U, 0x3fe00000U, 2, true, false,
                {{0, 0x42610000U, 0x40c00000U}, {1, 0x41480000U, 0x40600000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                    {32, 0x42988000U, 0x40c00000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x44011000U, 0x40c00000U}, {767, 0x43e60000U, 0x3f800000U}}}}}},
    {"strict-retained-reveal", {},
        {0x44700000U, 0x41200000U, 0x00000000U, 0, true, false,
            {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}},
        {{0x3d4ccccdU, 1, false,
             {0x44638000U, 0x41280000U, 0x3f400000U, 0, true, false,
                 {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                     {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x3c888888U, 1, false,
                {0x445f2000U, 0x412aaaabU, 0x3f800000U, 0, true, false,
                    {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x00000000U, 1, false,
                {0x445f2000U, 0x412aaaabU, 0x3f800000U, 0, true, false,
                    {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x2f800000U, 1, false,
                {0x445f2000U, 0x412aaaabU, 0x00000000U, 1, true, false,
                    {{0, 0x32c80000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x00000000U, 1, false,
                {0x445f2000U, 0x412aaaabU, 0x00000000U, 1, true, false,
                    {{0, 0x32c80000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x3d888889U, 1, false,
                {0x444d58e3U, 0x41355556U, 0x00000000U, 2, true, false,
                    {{0, 0x40d55556U, 0x40155556U}, {1, 0x40d55556U, 0x40155556U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x41d55556U, 0x40155556U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e95555U, 0x40155556U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x40000000U, 1, false,
                {0xc4b4a8e5U, 0x41f00000U, 0x41e80000U, 3, true, false,
                    {{0, 0x43ecaaacU, 0x42295555U}, {1, 0x43ecaaacU, 0x42295555U}, {2, 0x43480000U, 0x42240000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x43f6aaacU, 0x42295555U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x44695556U, 0x42295555U}, {767, 0x43e60000U, 0x3f800000U}}}}}},
    {"reveal-debt-and-stored-completion", {{0x3f800000U, {0x00000000U, 0x40a00000U, 0x00000000U, 0, false, false}}},
        {0x00000000U, 0x40a00000U, 0x00000000U, 0, true, false,
            {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}},
        {{0x41200000U, 1, false,
             {0xc59c4000U, 0x41f00000U, 0x43150000U, 1, true, false,
                 {{0, 0x447a0000U, 0x43490000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                     {32, 0x447f0000U, 0x43490000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x44b68000U, 0x43490000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x00000000U, 30, false,
                {0xc59c4000U, 0x41f00000U, 0x42ee0000U, 31, true, false,
                    {{0, 0x447a0000U, 0x43490000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x447f0000U, 0x43490000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x44b68000U, 0x43490000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x00000000U, 1, false,
                {0xc59c4000U, 0x41f00000U, 0x42ec0000U, 32, true, false,
                    {{0, 0x447a0000U, 0x43490000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x447f0000U, 0x43490000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x44b68000U, 0x43490000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x40800000U, 1, false,
                {0xc684d000U, 0x41f00000U, 0x43320000U, 32, true, false,
                    {{0, 0x479efc00U, 0x438c8000U}, {1, 0x43c80000U, 0x42a20000U}, {2, 0x43c80000U, 0x42a20000U}, {30, 0x43c80000U, 0x42a20000U}, {31, 0x43c80000U, 0x42a20000U},
                        {32, 0x479f0600U, 0x438c8000U}, {63, 0x43d20000U, 0x42a20000U}, {736, 0x479fe200U, 0x438c8000U}, {767, 0x44570000U, 0x42a20000U}}}},
            {0x3c21d13aU, 1, false,
                {0xc6850b42U, 0x41f00000U, 0x433225edU, 32, true, false,
                    {{0, 0x479f86c4U, 0x438c9949U}, {1, 0x43f00000U, 0x42a26523U}, {2, 0x43f00000U, 0x42a26523U}, {30, 0x43f00000U, 0x42a26523U}, {31, 0x43f00000U, 0x42a26523U},
                        {32, 0x479f90c4U, 0x438c9949U}, {63, 0x43fa0000U, 0x42a26523U}, {736, 0x47a06cc4U, 0x438c9949U}, {767, 0x446b0000U, 0x42a26523U}}}},
            {0x2f800000U, 1, false,
                {0xc6850b42U, 0x41f00000U, 0x433225edU, 32, true, false,
                    {{0, 0x479f86c4U, 0x438c9949U}, {1, 0x43f00000U, 0x42a26523U}, {2, 0x43f00000U, 0x42a26523U}, {30, 0x43f00000U, 0x42a26523U}, {31, 0x43f00000U, 0x42a26523U},
                        {32, 0x479f90c4U, 0x438c9949U}, {63, 0x43fa0000U, 0x42a26523U}, {736, 0x47a06cc4U, 0x438c9949U}, {767, 0x446b0000U, 0x42a26523U}}}},
            {0x31800000U, 1, false,
                {0xc6850b42U, 0x41f00000U, 0x433225edU, 32, true, true,
                    {{0, 0x479f86c4U, 0x438c9949U}, {1, 0x43f00001U, 0x42a26523U}, {2, 0x43f00001U, 0x42a26523U}, {30, 0x43f00001U, 0x42a26523U}, {31, 0x43f00001U, 0x42a26523U},
                        {32, 0x479f90c4U, 0x438c9949U}, {63, 0x43fa0001U, 0x42a26523U}, {736, 0x47a06cc4U, 0x438c9949U}, {767, 0x446b0000U, 0x42a26523U}}}},
            {0x3f800000U, 1, false,
                {0xc6850b42U, 0x41f00000U, 0x433225edU, 32, true, true,
                    {{0, 0x479f86c4U, 0x438c9949U}, {1, 0x43f00001U, 0x42a26523U}, {2, 0x43f00001U, 0x42a26523U}, {30, 0x43f00001U, 0x42a26523U}, {31, 0x43f00001U, 0x42a26523U},
                        {32, 0x479f90c4U, 0x438c9949U}, {63, 0x43fa0001U, 0x42a26523U}, {736, 0x47a06cc4U, 0x438c9949U}, {767, 0x446b0000U, 0x42a26523U}}}}}},
    {"all-columns-natural-finish", {{0x40000000U, {0x00000000U, 0x3f800000U, 0x00000000U, 0, false, false}}},
        {0x00000000U, 0x3f800000U, 0x00000000U, 0, true, false,
            {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}},
        {{0x3e000000U, 31, false,
             {0xc5e22c00U, 0x41f00000U, 0x41d90000U, 31, true, false,
                 {{0, 0x46691b00U, 0x429d0000U}, {1, 0x465a4300U, 0x42980000U}, {2, 0x464be800U, 0x42930000U}, {30, 0x41480000U, 0x40600000U}, {31, 0x00000000U, 0x3f800000U},
                     {32, 0x46696b00U, 0x429d0000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x46704b00U, 0x429d0000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x3e000000U, 1, false,
                {0xc5ede400U, 0x41f00000U, 0x41e00000U, 32, true, false,
                    {{0, 0x46787000U, 0x42a20000U}, {1, 0x46691b00U, 0x429d0000U}, {2, 0x465a4300U, 0x42980000U}, {30, 0x42610000U, 0x40c00000U}, {31, 0x41480000U, 0x40600000U},
                        {32, 0x4678c000U, 0x42a20000U}, {63, 0x42020000U, 0x40600000U}, {736, 0x467fa000U, 0x42a20000U}, {767, 0x43ec4000U, 0x40600000U}}}},
            {0x3e000000U, 4, false,
                {0xc60e6200U, 0x41f00000U, 0x420e0000U, 32, true, false,
                    {{0, 0x469d5300U, 0x42b60000U}, {1, 0x4694ae80U, 0x42b10000U}, {2, 0x468c4880U, 0x42ac0000U}, {30, 0x4407f000U, 0x41800000U}, {31, 0x43bb8000U, 0x41580000U},
                        {32, 0x469d7b00U, 0x42b60000U}, {63, 0x43c58000U, 0x41580000U}, {736, 0x46a0eb00U, 0x42b60000U}, {767, 0x4450c000U, 0x41580000U}}}},
            {0x3e000000U, 1, false,
                {0xc6143e00U, 0x41f00000U, 0x42158000U, 32, true, true,
                    {{0, 0x46a63600U, 0x42bb0000U}, {1, 0x469d5300U, 0x42b60000U}, {2, 0x4694ae80U, 0x42b10000U}, {30, 0x4439f000U, 0x41940000U}, {31, 0x4407f000U, 0x41800000U},
                        {32, 0x46a65e00U, 0x42bb0000U}, {63, 0x440cf000U, 0x41800000U}, {736, 0x46a9ce00U, 0x42bb0000U}, {767, 0x447af000U, 0x41800000U}}}},
            {0x3f800000U, 1, false,
                {0xc6143e00U, 0x41f00000U, 0x42158000U, 32, true, true,
                    {{0, 0x46a63600U, 0x42bb0000U}, {1, 0x469d5300U, 0x42b60000U}, {2, 0x4694ae80U, 0x42b10000U}, {30, 0x4439f000U, 0x41940000U}, {31, 0x4407f000U, 0x41800000U},
                        {32, 0x46a65e00U, 0x42bb0000U}, {63, 0x440cf000U, 0x41800000U}, {736, 0x46a9ce00U, 0x42bb0000U}, {767, 0x447af000U, 0x41800000U}}}}}},
    {"skip-motion", {{0x3e000000U, {0x4450c000U, 0x41160000U, 0x00000000U, 0, false, false}}},
        {0x4450c000U, 0x41160000U, 0x00000000U, 0, true, false,
            {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}},
        {{0x3f000000U, 1, true,
             {0x43b72000U, 0x41660000U, 0x40d00000U, 1, true, true,
                 {{0, 0x42480000U, 0x41300000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                     {32, 0x428c0000U, 0x41300000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43ff0000U, 0x41300000U}, {767, 0x43e60000U, 0x3f800000U}}}},
            {0x41200000U, 1, false,
                {0x43b72000U, 0x41660000U, 0x40d00000U, 1, true, true,
                    {{0, 0x42480000U, 0x41300000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                        {32, 0x428c0000U, 0x41300000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43ff0000U, 0x41300000U}, {767, 0x43e60000U, 0x3f800000U}}}}}},
    {"zero-delta-skip", {},
        {0x44700000U, 0x41200000U, 0x00000000U, 0, true, false,
            {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}},
        {{0x00000000U, 1, true,
            {0x44700000U, 0x41200000U, 0x00000000U, 0, true, true,
                {{0, 0x00000000U, 0x3f800000U}, {1, 0x00000000U, 0x3f800000U}, {2, 0x00000000U, 0x3f800000U}, {30, 0x00000000U, 0x3f800000U}, {31, 0x00000000U, 0x3f800000U},
                    {32, 0x41a00000U, 0x3f800000U}, {63, 0x41a00000U, 0x3f800000U}, {736, 0x43e60000U, 0x3f800000U}, {767, 0x43e60000U, 0x3f800000U}}}}}}}};

using moorhuhn2::ui::ResultsTile;
using moorhuhn2::ui::ResultsTransition;

using moorhuhn2::test::require;

float binary32(std::uint32_t value) {
    return std::bit_cast<float>(value);
}

std::uint32_t bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Snapshot {
    std::array<ResultsTile, 768> tiles{};
    float row_offset{}, row_speed{}, reveal_time{};
    std::size_t next_column{};
    bool exiting{}, finished{};

    bool operator==(const Snapshot&) const = default;
};

Snapshot snapshot(const ResultsTransition& transition) {
    Snapshot result{{}, transition.row_offset(), transition.row_speed(), transition.reveal_time(), transition.next_column(), transition.exiting(), transition.finished()};
    std::ranges::copy(transition.records(), result.tiles.begin());

    return result;
}

void check_summary(const ResultsTransition& transition, const TransitionExpectation& expected) {
    require(bits(transition.row_offset()) == expected.row_offset_bits, "Source row offset bits differ");
    require(bits(transition.row_speed()) == expected.row_speed_bits, "Source row speed bits differ");
    require(bits(transition.reveal_time()) == expected.reveal_time_bits, "Source reveal accumulator bits differ");
    require(transition.next_column() == expected.next_column, "Source reveal column differs");
    require(transition.exiting() == expected.exiting && transition.finished() == expected.finished, "Source entry/exit/completion phase differs");
    if (!expected.records.empty()) {
        for (const auto& record : expected.records) {
            const auto& tile = transition.records()[record.index];
            require(bits(tile.y) == record.y_bits && bits(tile.velocity_y) == record.velocity_y_bits, "Source stored tile Y or old-velocity movement/acceleration differs");
        }
    }
    std::size_t active = 0;
    for (std::size_t index = 0; index < transition.records().size(); ++index) {
        const auto& tile = transition.records()[index];
        require(tile.x == static_cast<float>(index % 32 * 20) && tile.frame == static_cast<int>(index), "Grid X and frame must remain in original row-major order");
        const auto column = index % 32;
        require(tile.state == (column < transition.next_column() ? 1 : 0), "Reveal must activate the same ascending columns in all 24 rows");
        if (tile.state > 0) {
            ++active;
        } else {
            require(tile.y == static_cast<float>(index / 32 * 20) && tile.velocity_y == (transition.exiting() ? 1 : 0),
                "Unrevealed records must remain at their reset grid poses and velocities");
        }
    }

    require(active == 24 * transition.next_column(), "Every reveal must activate exactly 24 tiles");
}

void check_draws(const ResultsTransition& transition) {
    const auto before = snapshot(transition);
    const auto draws = transition.background_draws();
    const std::size_t underlay_size = transition.exiting() ? 768 : 0;
    require(draws.size() == underlay_size + 768, "Entry must draw 768 score tiles; exit must draw 768 title then 768 score tiles");
    for (std::size_t index = 0; index < draws.size(); ++index) {
        const bool underlay = index < underlay_size;
        const std::size_t tile_index = 767 - (underlay ? index : index - underlay_size);
        const auto& tile = transition.records()[tile_index];
        const auto& draw = draws[index];
        const int x = static_cast<int>(tile_index % 32 * 20);
        const int y = static_cast<int>(tile_index / 32 * 20);
        require(draw.image == (underlay ? "gfx/mainscreen" : "gfx/scorescreen") && draw.source.x == x && draw.source.y == y && draw.source.width == 20 && draw.source.height == 20
                    && draw.destination.x == x && draw.destination.y == (underlay ? y : static_cast<int>(tile.y)) && draw.color_key && !draw.mirror_x,
            "Reverse painter order, fixed title grid,20×20 cells or truncated score tile poses differ");
    }

    require(snapshot(transition) == before, "Background drawing must be pure");
}

void check_sequence(const SequenceCase& sequence, const TransitionExpectation& initial) {
    ResultsTransition transition;
    check_summary(transition, initial);
    check_draws(transition);
    for (const auto& step : sequence.entry_steps) {
        transition.update_entry(binary32(step.delta_bits));
        check_summary(transition, step.expected);
    }

    const auto rows = std::array{bits(transition.row_offset()), bits(transition.row_speed())};
    transition.begin_exit();
    require(std::array{bits(transition.row_offset()), bits(transition.row_speed())} == rows, "Exit entry must retain both current row motion values");
    check_summary(transition, sequence.exit_initial);
    check_draws(transition);
    for (const auto& step : sequence.exit_steps) {
        for (int repeat = 0; repeat < step.repeat; ++repeat) {
            const auto previous_column = transition.next_column();
            const auto finished = transition.update_exit(binary32(step.delta_bits), step.skip);
            require(finished == transition.finished(), "Exit update return must describe the state of its final presented frame");
            require(transition.next_column() <= previous_column + 1, "One update must never catch up several columns");
        }

        check_summary(transition, step.expected);
        check_draws(transition);
    }

    const auto before_repeated_begin = snapshot(transition);
    transition.begin_exit();
    require(snapshot(transition) == before_repeated_begin, "Repeated exit entry must not restart motion or consume another input boundary");
}

void invalid_inputs(const TransitionExpectation& initial) {
    ResultsTransition transition;
    const auto entry = snapshot(transition);
    bool rejected = false;
    try {
        transition.update_exit(0, true);
    } catch (const std::logic_error&) {
        rejected = true;
    }

    require(rejected && snapshot(transition) == entry, "An exit update before entry must be rejected without mutation");
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()}) {
        rejected = false;
        try {
            transition.update_entry(delta);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && snapshot(transition) == entry, "Invalid or overflowing entry updates must be rejected atomically");
        check_summary(transition, initial);
    }

    transition.begin_exit();
    const auto exit = snapshot(transition);
    rejected = false;
    try {
        transition.update_entry(0);
    } catch (const std::logic_error&) {
        rejected = true;
    }

    require(rejected && snapshot(transition) == exit, "An entry update during exit must be rejected without mutation");
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()}) {
        rejected = false;
        try {
            transition.update_exit(delta, true);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && snapshot(transition) == exit, "Invalid or overflowing exit updates must reject skip and all mutation");
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

        invalid_inputs(initial);
        std::cout << "PASS: results row entry/exit,7 exact source sequences, strict reveal/debt, stored completion, skip final update, reset and painter order\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
