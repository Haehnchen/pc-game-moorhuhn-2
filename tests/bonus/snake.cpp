#include "bonus/snake.hpp"

#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct SnakeExpectation {
    std::array<int, 2> head{};
    std::vector<int> body{};
    std::uint32_t board_fnv32{};
    int direction{};
    std::uint32_t score{};
    std::uint32_t speed_le{};
    std::uint32_t accumulator_le{};
    bool playing{};
    bool reset_pending{};
    bool returned{};
    std::uint32_t random_after{};
    bool moved{};
    std::vector<std::string> sounds{};
    std::size_t draw_count{};
    std::uint32_t draws_fnv32{};
};

struct StepCase {
    std::uint32_t delta_le{};
    std::vector<std::string> controls{};
    SnakeExpectation expected{};
};

struct SequenceCase {
    std::string name{};
    std::uint32_t random_before{};
    SnakeExpectation initial{};
    std::vector<StepCase> steps{};
};

const std::array<SequenceCase, 18> sequences = {{{"idle-and-exact-threshold", 0x00000001U,
                                                     {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155,
                                                         0xb4cd15d7U},
                                                     {{0x42480000U, {"up", "right", "down", "left"},
                                                          {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155,
                                                              0xb4cd15d7U}},
                                                         {0x00000000U, {"space"},
                                                             {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {},
                                                                 138, 0x38c9d043U}},
                                                         {0x3f000000U, {},
                                                             {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x3f000000U, true, false, false, 0x00000001U, false, {},
                                                                 138, 0x38c9d043U}},
                                                         {0x3f000000U, {},
                                                             {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true,
                                                                 {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
                                                         {0x40400000U, {},
                                                             {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true,
                                                                 {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
                                                         {0x00000000U, {},
                                                             {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U,
                                                                 false, {}, 138, 0x73824087U}}}},
    {"space-during-play-resets-score-only", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x3f800000U, {"space"},
             {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f000000U, {}, {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x3f000000U, true, false, false, 0x00000001U, false, {}, 138, 0xe6dbf771U}},
            {0x00000000U, {"space"}, {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 0, 0x3f800000U, 0x3f000000U, true, false, false, 0x00000001U, false, {}, 138, 0x5388640bU}},
            {0x3f000000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xf1aab199U}}}},
    {"held-key-priority-up-right-down-left", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x3f800000U, {"space", "up", "right", "down", "left"},
            {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}}}},
    {"held-key-priority-up-right-down", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x3f800000U, {"space", "up", "right", "down"},
            {{16, 13}, {432, 400, 401}, 0x52d1a2b5U, 2, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe55cdd3dU}}}},
    {"held-key-priority-up-right-reverse-collision", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x3f800000U, {"space", "up", "right"},
             {{17, 12}, {401, 400, 401}, 0xeb2efa9dU, 1, 1, 0x3f800000U, 0x00000000U, false, true, false, 0x00000001U, true, {"sfx/sfx_typename_click", "sfx/sfx_airplane_crash"},
                 154, 0x5d6b5cf7U}},
            {0x00000000U, {}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 1, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0x1c2cca6dU}},
            {0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}}}},
    {"left-wall-collision-reset-and-restart", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x3f800000U, {},
                {{13, 12}, {397, 398, 399}, 0xb1de6855U, 3, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x6291ca05U}},
            {0x3f800000U, {},
                {{12, 12}, {396, 397, 398}, 0x477723e5U, 3, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7941680bU}},
            {0x3f800000U, {},
                {{11, 12}, {395, 396, 397}, 0x9aea2475U, 3, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x500e6884U}},
            {0x3f800000U, {},
                {{10, 12}, {394, 395, 396}, 0xbeca7a05U, 3, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7a0644f6U}},
            {0x3f800000U, {},
                {{9, 12}, {393, 394, 395}, 0x8b161e95U, 3, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x2b147020U}},
            {0x3f800000U, {},
                {{8, 12}, {392, 393, 394}, 0xe1694a25U, 3, 8, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xa1b5cd12U}},
            {0x3f800000U, {},
                {{7, 12}, {391, 392, 393}, 0x85af9eb5U, 3, 9, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xa53a1441U}},
            {0x3f800000U, {},
                {{6, 12}, {390, 391, 392}, 0xad8f2e45U, 3, 10, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xac1bec13U}},
            {0x3f800000U, {},
                {{5, 12}, {389, 390, 391}, 0x15c4a2d5U, 3, 11, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xda755426U}},
            {0x3f800000U, {},
                {{4, 12}, {388, 389, 390}, 0x1e1d7465U, 3, 12, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xf1747228U}},
            {0x3f800000U, {},
                {{3, 12}, {387, 388, 389}, 0x43a1f4f5U, 3, 13, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0x46a409aaU}},
            {0x3f800000U, {},
                {{2, 12}, {386, 387, 388}, 0xfbcbea85U, 3, 14, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xc139c2fcU}},
            {0x3f800000U, {},
                {{1, 12}, {385, 386, 387}, 0x6f01a915U, 3, 15, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xab762d97U}},
            {0x3f800000U, {},
                {{0, 12}, {384, 385, 386}, 0xfc8f27c9U, 3, 16, 0x3f800000U, 0x00000000U, false, true, false, 0x00000001U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_airplane_crash"}, 155, 0x4851bfaeU}},
            {0x41400000U, {}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 16, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 156, 0x6c6e6f54U}},
            {0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}}}},
    {"top-wall-collision", 0x00000001U, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {"up"},
                {{16, 11}, {368, 400, 401}, 0x7ba5cfb5U, 0, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xfdad466dU}},
            {0x3f800000U, {"up"},
                {{16, 10}, {336, 368, 400}, 0x28dcaf15U, 0, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xcf8eca13U}},
            {0x3f800000U, {"up"},
                {{16, 9}, {304, 336, 368}, 0x9569fc05U, 0, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xd9ecd715U}},
            {0x3f800000U, {"up"},
                {{16, 8}, {272, 304, 336}, 0x4ad041d5U, 0, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x27cfcb17U}},
            {0x3f800000U, {"up"},
                {{16, 7}, {240, 272, 304}, 0x8d7be405U, 0, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xafcfe04cU}},
            {0x3f800000U, {"up"},
                {{16, 6}, {208, 240, 272}, 0x76452fd5U, 0, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x75a060b2U}},
            {0x3f800000U, {"up"},
                {{16, 5}, {176, 208, 240}, 0x23b5a205U, 0, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x4db665b8U}},
            {0x3f800000U, {"up"},
                {{16, 4}, {144, 176, 208}, 0xd6ee53d5U, 0, 8, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x6b7b47c6U}},
            {0x3f800000U, {"up"},
                {{16, 3}, {112, 144, 176}, 0xc8355c05U, 0, 9, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe75720b4U}},
            {0x3f800000U, {"up"},
                {{16, 2}, {80, 112, 144}, 0x2d6e5dd5U, 0, 10, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0x50f81ddbU}},
            {0x3f800000U, {"up"},
                {{16, 1}, {48, 80, 112}, 0xe3405805U, 0, 11, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xffa28675U}},
            {0x3f800000U, {"up"},
                {{16, 0}, {16, 48, 80}, 0x9d8fdf09U, 0, 12, 0x3f800000U, 0x00000000U, false, true, false, 0x00000001U, true, {"sfx/sfx_typename_click", "sfx/sfx_airplane_crash"},
                    155, 0x8f14265dU}},
            {0x00000000U, {}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 12, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 156, 0xb8758347U}}}},
    {"right-wall-collision", 0x00000001U, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x3f800000U, {"space", "up"},
             {{16, 11}, {368, 400, 401}, 0x7ba5cfb5U, 0, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xfdad466dU}},
            {0x3f800000U, {"right"},
                {{17, 11}, {369, 368, 400}, 0x5126a395U, 1, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x8a225247U}},
            {0x3f800000U, {"right"},
                {{18, 11}, {370, 369, 368}, 0x26801995U, 1, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x87e094a9U}},
            {0x3f800000U, {"right"},
                {{19, 11}, {371, 370, 369}, 0x247ddb05U, 1, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x43120af7U}},
            {0x3f800000U, {"right"},
                {{20, 11}, {372, 371, 370}, 0xa202af75U, 1, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x4c6c3048U}},
            {0x3f800000U, {"right"},
                {{21, 11}, {373, 372, 371}, 0xf4b44ae5U, 1, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x2c23bd02U}},
            {0x3f800000U, {"right"},
                {{22, 11}, {374, 373, 372}, 0xa82cdf55U, 1, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xed7da89cU}},
            {0x3f800000U, {"right"},
                {{23, 11}, {375, 374, 373}, 0xf52958c5U, 1, 8, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x2e8dee46U}},
            {0x3f800000U, {"right"},
                {{24, 11}, {376, 375, 374}, 0xf00d1535U, 1, 9, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xde676f00U}},
            {0x3f800000U, {"right"},
                {{25, 11}, {377, 376, 375}, 0x8b44f4a5U, 1, 10, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xfe18ce0bU}},
            {0x3f800000U, {"right"},
                {{26, 11}, {378, 377, 376}, 0x30cb7d15U, 1, 11, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xaf06b7a1U}},
            {0x3f800000U, {"right"},
                {{27, 11}, {379, 378, 377}, 0x6236f885U, 1, 12, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0x4b5c8b3fU}},
            {0x3f800000U, {"right"},
                {{28, 11}, {380, 379, 378}, 0x93049af5U, 1, 13, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0x90820ec5U}},
            {0x3f800000U, {"right"},
                {{29, 11}, {381, 380, 379}, 0xbba2b465U, 1, 14, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xc4315623U}},
            {0x3f800000U, {"right"},
                {{30, 11}, {382, 381, 380}, 0x0faaf8d5U, 1, 15, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0xe123440cU}},
            {0x3f800000U, {"right"},
                {{31, 11}, {383, 382, 381}, 0xdf5dc5a9U, 1, 16, 0x3f800000U, 0x00000000U, false, true, false, 0x00000001U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_airplane_crash"}, 155, 0xa9eaf51aU}},
            {0x00000000U, {}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 16, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 156, 0x6c6e6f54U}}}},
    {"bottom-wall-collision", 0x00000001U, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {"down"},
                {{16, 13}, {432, 400, 401}, 0x52d1a2b5U, 2, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe55cdd3dU}},
            {0x3f800000U, {"down"},
                {{16, 14}, {464, 432, 400}, 0xc1540515U, 2, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xb4b59ad3U}},
            {0x3f800000U, {"down"},
                {{16, 15}, {496, 464, 432}, 0x8c143745U, 2, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xdcb96d24U}},
            {0x3f800000U, {"down"},
                {{16, 16}, {528, 496, 464}, 0x65759915U, 2, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x5d97a313U}},
            {0x3f800000U, {"down"},
                {{16, 17}, {560, 528, 496}, 0x53c59745U, 2, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe877e6c1U}},
            {0x3f800000U, {"down"},
                {{16, 18}, {592, 560, 528}, 0x1ce43715U, 2, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xfe8fbdb7U}},
            {0x3f800000U, {"down"},
                {{16, 19}, {624, 592, 560}, 0x3b444b45U, 2, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x338e250dU}},
            {0x3f800000U, {"down"},
                {{16, 20}, {656, 624, 592}, 0xcfec3f15U, 2, 8, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x17363aabU}},
            {0x3f800000U, {"down"},
                {{16, 21}, {688, 656, 624}, 0x85d0f745U, 2, 9, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6358719U}},
            {0x3f800000U, {"down"},
                {{16, 22}, {720, 688, 656}, 0x2fce8515U, 2, 10, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0x90820184U}},
            {0x3f800000U, {"down"},
                {{16, 23}, {752, 720, 688}, 0x6e238345U, 2, 11, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 139, 0x463a2c92U}},
            {0x3f800000U, {"down"},
                {{16, 24}, {784, 752, 720}, 0x56b86239U, 2, 12, 0x3f800000U, 0x00000000U, false, true, false, 0x00000001U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_airplane_crash"}, 155, 0x62f2a079U}},
            {0x00000000U, {}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 12, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 156, 0xb8758347U}}}},
    {"food-grow-speed-and-second-food", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x3f800000U, {},
                {{13, 12}, {397, 398, 399}, 0xb1de6855U, 3, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x6291ca05U}},
            {0x3f800000U, {},
                {{12, 12}, {396, 397, 398}, 0x477723e5U, 3, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7941680bU}},
            {0x3f800000U, {},
                {{11, 12}, {395, 396, 397}, 0x9aea2475U, 3, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x500e6884U}},
            {0x3f800000U, {},
                {{10, 12}, {394, 395, 396}, 0xbeca7a05U, 3, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7a0644f6U}},
            {0x3f800000U, {"up"},
                {{10, 11}, {362, 394, 395}, 0x9a449415U, 0, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xf19f3c9cU}},
            {0x3f800000U, {"up"},
                {{10, 10}, {330, 362, 394, 395}, 0x76b9f285U, 0, 58, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_50_point_score"}, 139, 0x875b86caU}},
            {0x3f800000U, {"left"},
                {{9, 10}, {329, 330, 362, 394}, 0x0adcee1dU, 3, 59, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140, 0x77fa1a8eU}},
            {0x3f800000U, {"down"},
                {{9, 11}, {361, 329, 330, 362}, 0x6079028dU, 2, 60, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140, 0xb7c53aefU}},
            {0x3f800000U, {"down"},
                {{9, 12}, {393, 361, 329, 330}, 0x26bbdd1dU, 2, 61, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140, 0x48d8f879U}},
            {0x3f800000U, {"down"},
                {{9, 13}, {425, 393, 361, 329}, 0xcbee286dU, 2, 62, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140, 0x512ce8e3U}},
            {0x3f800000U, {"down"},
                {{9, 14}, {457, 425, 393, 361}, 0x394438adU, 2, 63, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140, 0x6f0cd161U}},
            {0x3f800000U, {"down"},
                {{9, 15}, {489, 457, 425, 393}, 0xdf34346dU, 2, 64, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140, 0x116179a8U}},
            {0x3f800000U, {"down"},
                {{9, 16}, {521, 489, 457, 425}, 0x4a04feadU, 2, 65, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140, 0x7390c2ccU}},
            {0x3f800000U, {"down"},
                {{9, 17}, {553, 521, 489, 457, 425}, 0x81a0443dU, 2, 116, 0x3fc00000U, 0x00000000U, true, false, false, 0x72ae7869U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_50_point_score"}, 141, 0x43e5d71dU}}}},
    {"food-rejects-border", 0x00000008U, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000008U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000008U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000008U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000008U, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x3f800000U, {},
                {{13, 12}, {397, 398, 399}, 0xb1de6855U, 3, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000008U, true, {"sfx/sfx_typename_click"}, 138, 0x6291ca05U}},
            {0x3f800000U, {},
                {{12, 12}, {396, 397, 398}, 0x477723e5U, 3, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000008U, true, {"sfx/sfx_typename_click"}, 138, 0x7941680bU}},
            {0x3f800000U, {},
                {{11, 12}, {395, 396, 397}, 0x9aea2475U, 3, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000008U, true, {"sfx/sfx_typename_click"}, 138, 0x500e6884U}},
            {0x3f800000U, {},
                {{10, 12}, {394, 395, 396}, 0xbeca7a05U, 3, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000008U, true, {"sfx/sfx_typename_click"}, 138, 0x7a0644f6U}},
            {0x3f800000U, {"up"},
                {{10, 11}, {362, 394, 395}, 0x9a449415U, 0, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000008U, true, {"sfx/sfx_typename_click"}, 138, 0xf19f3c9cU}},
            {0x3f800000U, {"up"},
                {{10, 10}, {330, 362, 394, 395}, 0x0fd74045U, 0, 58, 0x3fa00000U, 0x00000000U, true, false, false, 0xf97fa34cU, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_50_point_score"}, 139, 0x69add2adU}}}},
    {"food-rejects-current-body", 0x00000257U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000257U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000257U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000257U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000257U, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x3f800000U, {},
                {{13, 12}, {397, 398, 399}, 0xb1de6855U, 3, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000257U, true, {"sfx/sfx_typename_click"}, 138, 0x6291ca05U}},
            {0x3f800000U, {},
                {{12, 12}, {396, 397, 398}, 0x477723e5U, 3, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000257U, true, {"sfx/sfx_typename_click"}, 138, 0x7941680bU}},
            {0x3f800000U, {},
                {{11, 12}, {395, 396, 397}, 0x9aea2475U, 3, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000257U, true, {"sfx/sfx_typename_click"}, 138, 0x500e6884U}},
            {0x3f800000U, {},
                {{10, 12}, {394, 395, 396}, 0xbeca7a05U, 3, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000257U, true, {"sfx/sfx_typename_click"}, 138, 0x7a0644f6U}},
            {0x3f800000U, {"up"},
                {{10, 11}, {362, 394, 395}, 0x9a449415U, 0, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000257U, true, {"sfx/sfx_typename_click"}, 138, 0xf19f3c9cU}},
            {0x3f800000U, {"up"},
                {{10, 10}, {330, 362, 394, 395}, 0x6dc46fc5U, 0, 58, 0x3fa00000U, 0x00000000U, true, false, false, 0x79ea0e4bU, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_50_point_score"}, 139, 0x1dcd00b9U}}}},
    {"food-can-occupy-cleared-growth-tail", 0x0000075bU,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x0000075bU, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x0000075bU, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x0000075bU, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x0000075bU, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x3f800000U, {},
                {{13, 12}, {397, 398, 399}, 0xb1de6855U, 3, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x0000075bU, true, {"sfx/sfx_typename_click"}, 138, 0x6291ca05U}},
            {0x3f800000U, {},
                {{12, 12}, {396, 397, 398}, 0x477723e5U, 3, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x0000075bU, true, {"sfx/sfx_typename_click"}, 138, 0x7941680bU}},
            {0x3f800000U, {},
                {{11, 12}, {395, 396, 397}, 0x9aea2475U, 3, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x0000075bU, true, {"sfx/sfx_typename_click"}, 138, 0x500e6884U}},
            {0x3f800000U, {},
                {{10, 12}, {394, 395, 396}, 0xbeca7a05U, 3, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x0000075bU, true, {"sfx/sfx_typename_click"}, 138, 0x7a0644f6U}},
            {0x3f800000U, {"up"},
                {{10, 11}, {362, 394, 395}, 0x9a449415U, 0, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x0000075bU, true, {"sfx/sfx_typename_click"}, 138, 0xf19f3c9cU}},
            {0x3f800000U, {"up"},
                {{10, 10}, {330, 362, 394, 395}, 0x15017d65U, 0, 58, 0x3fa00000U, 0x00000000U, true, false, false, 0x71b8c8adU, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_50_point_score"}, 139, 0x05f0aac1U}},
            {0x3f800000U, {},
                {{10, 9}, {298, 330, 362, 394}, 0xc19d744cU, 0, 59, 0x3fa00000U, 0x00000000U, true, false, false, 0x71b8c8adU, true, {"sfx/sfx_typename_click"}, 139,
                    0xe4c50143U}}}},
    {"cleared-tail-is-eligible-for-head", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x3f800000U, {},
                {{13, 12}, {397, 398, 399}, 0xb1de6855U, 3, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x6291ca05U}},
            {0x3f800000U, {},
                {{12, 12}, {396, 397, 398}, 0x477723e5U, 3, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7941680bU}},
            {0x3f800000U, {},
                {{11, 12}, {395, 396, 397}, 0x9aea2475U, 3, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x500e6884U}},
            {0x3f800000U, {},
                {{10, 12}, {394, 395, 396}, 0xbeca7a05U, 3, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7a0644f6U}},
            {0x3f800000U, {"up"},
                {{10, 11}, {362, 394, 395}, 0x9a449415U, 0, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xf19f3c9cU}},
            {0x3f800000U, {"up"},
                {{10, 10}, {330, 362, 394, 395}, 0x76b9f285U, 0, 58, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_50_point_score"}, 139, 0x875b86caU}},
            {0x3f800000U, {"right"},
                {{11, 10}, {331, 330, 362, 394}, 0x8e55793dU, 1, 59, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140,
                    0xda56cd3eU}},
            {0x3f800000U, {"down"},
                {{11, 11}, {363, 331, 330, 362}, 0xaf704badU, 2, 60, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140,
                    0x1ab9b70fU}},
            {0x3f800000U, {"left"},
                {{10, 11}, {362, 363, 331, 330}, 0x24df019dU, 3, 61, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140,
                    0xbdeb865dU}}}},
    {"reverse-into-body-after-growth", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x3f800000U, {},
                {{13, 12}, {397, 398, 399}, 0xb1de6855U, 3, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x6291ca05U}},
            {0x3f800000U, {},
                {{12, 12}, {396, 397, 398}, 0x477723e5U, 3, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7941680bU}},
            {0x3f800000U, {},
                {{11, 12}, {395, 396, 397}, 0x9aea2475U, 3, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x500e6884U}},
            {0x3f800000U, {},
                {{10, 12}, {394, 395, 396}, 0xbeca7a05U, 3, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7a0644f6U}},
            {0x3f800000U, {"up"},
                {{10, 11}, {362, 394, 395}, 0x9a449415U, 0, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xf19f3c9cU}},
            {0x3f800000U, {"up"},
                {{10, 10}, {330, 362, 394, 395}, 0x76b9f285U, 0, 58, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_50_point_score"}, 139, 0x875b86caU}},
            {0x3f800000U, {"down"},
                {{10, 11}, {362, 330, 362, 394}, 0x010ee485U, 2, 59, 0x3fa00000U, 0x00000000U, false, true, false, 0xc823f683U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_airplane_crash"}, 156, 0x044e4536U}},
            {0x00000000U, {}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 59, 0x3f800000U, 0x00000000U, false, false, false, 0xc823f683U, false, {}, 156, 0xd72b289fU}}}},
    {"retained-threshold-versus-rounded-timer", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x00000000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, false, {}, 138, 0x38c9d043U}},
            {0x3f800000U, {},
                {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x3f800000U, {},
                {{13, 12}, {397, 398, 399}, 0xb1de6855U, 3, 3, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x6291ca05U}},
            {0x3f800000U, {},
                {{12, 12}, {396, 397, 398}, 0x477723e5U, 3, 4, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7941680bU}},
            {0x3f800000U, {},
                {{11, 12}, {395, 396, 397}, 0x9aea2475U, 3, 5, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x500e6884U}},
            {0x3f800000U, {},
                {{10, 12}, {394, 395, 396}, 0xbeca7a05U, 3, 6, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x7a0644f6U}},
            {0x3f800000U, {"up"},
                {{10, 11}, {362, 394, 395}, 0x9a449415U, 0, 7, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xf19f3c9cU}},
            {0x3f800000U, {"up"},
                {{10, 10}, {330, 362, 394, 395}, 0x76b9f285U, 0, 58, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true,
                    {"sfx/sfx_typename_click", "sfx/sfx_50_point_score"}, 139, 0x875b86caU}},
            {0x3ecccccdU, {}, {{10, 10}, {330, 362, 394, 395}, 0x76b9f285U, 0, 58, 0x3fa00000U, 0x3f000000U, true, false, false, 0xc823f683U, false, {}, 139, 0x875b86caU}},
            {0x3eccccccU, {}, {{10, 10}, {330, 362, 394, 395}, 0x76b9f285U, 0, 58, 0x3fa00000U, 0x3f800000U, true, false, false, 0xc823f683U, false, {}, 139, 0x875b86caU}},
            {0x00000000U, {},
                {{10, 9}, {298, 330, 362, 394}, 0xeaa4c1bdU, 0, 59, 0x3fa00000U, 0x00000000U, true, false, false, 0xc823f683U, true, {"sfx/sfx_typename_click"}, 140,
                    0xf9f7a63aU}}}},
    {"escape-finishes-current-move-then-inert", 0x00000001U,
        {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x3f800000U, {"space"},
             {{15, 12}, {399, 400, 401}, 0xa3e5d435U, 3, 1, 0x3f800000U, 0x00000000U, true, false, false, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0xe6dbf771U}},
            {0x3f800000U, {"cancel"},
                {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, true, 0x00000001U, true, {"sfx/sfx_typename_click"}, 138, 0x73824087U}},
            {0x42480000U, {"space", "up"}, {{14, 12}, {398, 399, 400}, 0xb3c3b5c5U, 3, 2, 0x3f800000U, 0x00000000U, true, false, true, 0x00000001U, false, {}, 138, 0x73824087U}}}},
    {"escape-before-space", 0x00000001U, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, false, 0x00000001U, false, {}, 155, 0xb4cd15d7U},
        {{0x42c80000U, {"cancel"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, true, 0x00000001U, false, {}, 155, 0xb4cd15d7U}},
            {0x3f800000U, {"space"}, {{16, 12}, {400, 401, 402}, 0x233af5a5U, 3, 0, 0x3f800000U, 0x00000000U, false, false, true, 0x00000001U, false, {}, 155, 0xb4cd15d7U}}}}}};

using moorhuhn2::bonus::Snake;
using moorhuhn2::bonus::SnakeInput;
using moorhuhn2::bonus::SnakeState;
using moorhuhn2::bonus::SnakeUpdate;

using moorhuhn2::test::require;

float binary32(std::uint32_t value) {
    return std::bit_cast<float>(value);
}

class Checksum {
public:
    void byte(std::uint8_t value) {
        value_ = (value_ ^ value) * 16777619U;
    }

    void integer(std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) {
            byte(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void string(std::string_view value) {
        for (const unsigned char character : value) {
            byte(character);
        }

        byte(0);
    }

    [[nodiscard]] std::uint32_t value() const {
        return value_;
    }

private:
    std::uint32_t value_{2166136261U};
};

SnakeInput controls(std::span<const std::string> names) {
    SnakeInput input;
    for (const auto& name : names) {
        if (name == "space") {
            input.space = true;
        } else if (name == "cancel") {
            input.cancel = true;
        } else if (name == "up") {
            input.up = true;
        } else if (name == "right") {
            input.right = true;
        } else if (name == "down") {
            input.down = true;
        } else if (name == "left") {
            input.left = true;
        } else {
            throw std::runtime_error("Unknown Snake fixture input");
        }
    }

    return input;
}

void check(const Snake& game, std::uint32_t random_state, const SnakeUpdate& result, const SnakeExpectation& expected, std::string_view context) {
    const std::string prefix = std::string(context) + ": ";
    const auto& state = game.state();
    require(state.head_x == expected.head[0] && state.head_y == expected.head[1], prefix + "head differs");
    require(state.length == static_cast<int>(expected.body.size()), prefix + "growth length differs");
    for (int index = 0; index < state.length; ++index) {
        require(state.body[index] == expected.body[index], prefix + "ordered body differs");
    }
    Checksum board;
    for (const auto cell : state.board) {
        board.integer(std::bit_cast<std::uint32_t>(cell));
    }

    require(board.value() == expected.board_fnv32, prefix + "board cells differ");
    require(static_cast<int>(state.direction) == expected.direction && state.score == expected.score, prefix + "direction or score differs");
    require(std::bit_cast<std::uint32_t>(state.speed) == std::bit_cast<std::uint32_t>(binary32(expected.speed_le))
                && std::bit_cast<std::uint32_t>(state.accumulator) == std::bit_cast<std::uint32_t>(binary32(expected.accumulator_le)),
        prefix + "stored speed/timer differs");
    require(state.playing == expected.playing && state.reset_pending == expected.reset_pending && state.returned == expected.returned, prefix + "play/reset/return flags differ");
    require(random_state == expected.random_after, prefix + "CRT order or rejection differs");
    require(result.moved == expected.moved && result.exit_to_title == expected.returned, prefix + "move/return result differs");
    require(result.sound_count == expected.sounds.size() && result.sound_count <= result.sounds.size(), prefix + "sound count differs");
    for (std::size_t index = 0; index < result.sound_count; ++index) {
        require(result.sounds[index] == expected.sounds[index], prefix + "ordered sound differs");
    }

    for (std::size_t index = result.sound_count; index < result.sounds.size(); ++index) {
        require(result.sounds[index].empty(), prefix + "unreported sound slot populated");
    }

    const auto before_draw = state;
    for (int repeat = 0; repeat < 2; ++repeat) {
        const auto commands = game.draws();
        Checksum draw;
        for (const auto& command : commands) {
            draw.string(command.image);
            for (const int value : {command.source.x, command.source.y, command.source.width, command.source.height, command.destination.x, command.destination.y,
                     static_cast<int>(command.color_key), static_cast<int>(command.mirror_x)}) {
                draw.integer(static_cast<std::uint32_t>(value));
            }
        }

        require(commands.size() == expected.draw_count && draw.value() == expected.draws_fnv32, prefix + "source cells, anchors, glyphs or painter order differ");
    }

    require(state == before_draw, prefix + "drawing changed simulation state");
}

void invalid_deltas() {
    std::uint32_t random_state = 1;
    Snake game;
    game.update(1, {.space = true, .right = true}, random_state);
    require(game.state().reset_pending, "Invalid-delta test must begin with a pending collision reset");
    const auto before = game.state();
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected = false;
        try {
            game.update(delta, {.space = true, .cancel = true}, random_state);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && game.state() == before && random_state == 1, "Invalid delta changed state, reset pending death, or consumed RNG");
    }
    Snake maximum_delta;
    Snake one_move;
    const auto huge = maximum_delta.update(std::numeric_limits<float>::max(), {.space = true}, random_state);
    const auto ordinary = one_move.update(1, {.space = true}, random_state);
    require(huge.moved && ordinary.moved && maximum_delta.state() == one_move.state() && random_state == 1, "Finite huge delta must safely discard overshoot and make one move");
}

void independent_entries() {
    std::uint32_t random_state = 1;
    Snake first;
    first.update(6, {.space = true}, random_state);
    const auto first_before = first.state();
    const auto random_before = random_state;
    Snake second;
    require(
        second.state().score == 0 && second.state().length == 3 && !second.state().playing && second.state().board[330] == 1, "Reentry must construct a fresh idle bonus state");
    require(first.state() == first_before && random_state == random_before, "Snake entry changed another instance or reseeded session RNG");
}

} // namespace

int main() {
    try {
        std::size_t updates = 0;
        for (const auto& sequence : sequences) {
            const auto name = sequence.name;
            auto random_state = sequence.random_before;
            Snake game;
            check(game, random_state, {}, sequence.initial, name + " entry");
            for (const auto& step : sequence.steps) {
                const auto result = game.update(binary32(step.delta_le), controls(step.controls), random_state);
                check(game, random_state, result, step.expected, name + " update " + std::to_string(updates));
                ++updates;
            }
        }

        invalid_deltas();
        independent_entries();
        std::cout << "PASS: " << sequences.size() << " Snake source sequences, " << updates
                  << " updates; held priority, reversal/tail/walls, scoring/growth/RNG, retained timer, reset/return, ordered sounds and draws\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
