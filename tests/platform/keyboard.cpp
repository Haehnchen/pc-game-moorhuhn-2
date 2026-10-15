#include "platform/keyboard.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using moorhuhn2::platform::Keyboard;
using moorhuhn2::platform::KeyboardLocks;

struct KeyCache {
    std::uint16_t scan{};
    std::uint16_t key{};
};

struct StepCase {
    std::uint16_t scan{};
    std::optional<std::uint16_t> translated{};
    std::uint16_t result{};
    std::vector<std::uint16_t> held{};
    std::string op{};
    int count{};
    bool down{};
    KeyboardLocks locks{};
    std::uint16_t key{};
    std::optional<bool> pending{};
    std::vector<KeyCache> cache{};
    std::vector<std::uint16_t> released{};
};

struct SequenceCase {
    std::string id{};
    std::vector<StepCase> steps{};
    KeyboardLocks initial_locks{};
};

struct FixedKeyCase {
    std::string id{};
    std::uint16_t scan{};
    std::uint16_t key{};
    std::uint16_t down{};
    KeyboardLocks locks{};
    KeyboardLocks initial_locks{};
};

struct AnsiCase {
    std::string id{};
    std::uint32_t unicode{};
    bool control{};
    std::optional<std::uint16_t> byte{};
};

const std::array<SequenceCase, 13> sequences = {{{"queue-unused-before-space",
                                                     {{0x010U, 0x71U, 0x71U, {0x71U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
                                                         {0x039U, 0x20U, 0x20U, {0x20U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
                                                         {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x71U, true, {}, {}},
                                                         {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x20U, false, {}, {}},
                                                         {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
                                                     {false, false, false}},
    {"queue-63-drop-incoming-and-wrap",
        {{0x010U, 0x71U, 0x71U, {}, "repeat", 63, true, {false, false, false}, 0, true, {}, {}},
            {0x14bU, std::nullopt, 0x102U, {0x102U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {{0x14bU, 0x102U}}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x71U, std::nullopt, {}, {}},
            {0x039U, 0x20U, 0x20U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x14dU, std::nullopt, 0x103U, {0x103U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 62, true, {false, false, false}, 0x71U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x20U, false, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}},
            {0x150U, std::nullopt, 0x101U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x101U, false, {}, {}}},
        {false, false, false}},
    {"repeat-remaps-cached-lowercase-to-uppercase",
        {{0x030U, 0x62U, 0x62U, {0x62U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x02aU, std::nullopt, 0x1154U, {0x154U, 0x1154U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, 0x42U, 0x1042U, {0x42U, 0x1042U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {{0x030U, 0x42U}}, {0x62U}},
            {0x030U, 0x62U, 0x42U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x42U, 0x62U}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x62U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1154U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1042U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"translation-failure-cache-and-successful-zero",
        {{0x030U, std::nullopt, 0x00U, {}, "event", 1, true, {false, false, false}, 0, false, {{0x030U, 0x00U}}, {}},
            {0x030U, 0x62U, 0x62U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, std::nullopt, 0x62U, {0x62U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, 0x00U, 0x00U, {0x62U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {{0x030U, 0x62U}}, {}},
            {0x030U, 0x00U, 0x62U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x62U}},
            {0, std::nullopt, 0, {}, "read", 2, true, {false, false, false}, 0x62U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"queue-clear-preserves-held-and-cache",
        {{0x030U, 0x62U, 0x62U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {0x62U}, "clear", 1, true, {false, false, false}, 0, false, {{0x030U, 0x62U}}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}},
            {0x030U, std::nullopt, 0x62U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, std::nullopt, 0x62U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x62U}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x62U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"caps-forces-uppercase-even-shift-caps",
        {{0x030U, 0x62U, 0x42U, {0x42U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {0x62U}},
            {0x02aU, std::nullopt, 0x1154U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, 0x62U, 0x1042U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x42U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1154U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1042U, std::nullopt, {}, {}},
            {0x03aU, std::nullopt, 0x1160U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, 0x62U, 0x1062U, {0x62U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {0x42U}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1160U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1062U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {true, false, false}},
    {"all-modifier-bits-after-held-update",
        {{0x02aU, std::nullopt, 0x1154U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x11dU, std::nullopt, 0x5153U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x138U, std::nullopt, 0x7151U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x001U, 0x1bU, 0x701bU, {0x1bU, 0x701bU}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x02aU, std::nullopt, 0x154U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x154U}},
            {0x001U, std::nullopt, 0x1bU, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, 0x02U, 0x6002U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x11dU, std::nullopt, 0x153U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x138U, std::nullopt, 0x151U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x001U, 0x1bU, 0x1bU, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1154U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x5153U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x7151U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x701bU, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x6002U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1bU, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"quote-release-remaps-with-current-held-shift",
        {{0x028U, 0xe4U, 0x27U, {0x27U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x02aU, std::nullopt, 0x1154U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x028U, std::nullopt, 0x22U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {{0x028U, 0x22U}}, {0x27U, 0x22U}},
            {0x028U, std::nullopt, 0x1022U, {0x22U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x02aU, std::nullopt, 0x154U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x028U, std::nullopt, 0x27U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x22U, 0x27U}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x27U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1154U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1022U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"grave-release-remaps-with-right-shift",
        {{0x029U, std::nullopt, 0x60U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x036U, std::nullopt, 0x1155U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x029U, std::nullopt, 0x7eU, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {{0x029U, 0x7eU}}, {0x60U, 0x7eU}},
            {0x029U, std::nullopt, 0x107eU, {0x7eU}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x60U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x1155U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x107eU, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"keypad-cached-numlock-distinct-from-arrows",
        {{0x04bU, std::nullopt, 0x141U, {0x141U}, "event", 1, true, {false, true, false}, 0, std::nullopt, {}, {0x34U}},
            {0x14bU, std::nullopt, 0x102U, {0x141U, 0x102U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x145U, std::nullopt, 0x161U, {}, "event", 1, true, {false, true, false}, 0, std::nullopt, {}, {}},
            {0x04bU, std::nullopt, 0x34U, {0x102U}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x141U, 0x34U}},
            {0x04bU, std::nullopt, 0x34U, {0x34U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x141U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x102U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x161U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x34U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"same-logical-key-has-no-held-reference-count",
        {{0x01eU, 0x61U, 0x61U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x010U, 0x61U, 0x61U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x01eU, std::nullopt, 0x61U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {{0x010U, 0x61U}}, {0x61U}},
            {0, std::nullopt, 0, {}, "read", 2, true, {false, false, false}, 0x61U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"native-bounds-reject-malformed-scan-and-character",
        {{0x030U, 0x62U, 0x62U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x200U, 0x61U, 0x00U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {0x61U}},
            {0x030U, 0x1234U, 0x00U, {0x62U}, "event", 1, true, {false, false, false}, 0, std::nullopt, {{0x030U, 0x62U}}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x62U, std::nullopt, {}, {}},
            {0, std::nullopt, 0, {}, "read", 1, true, {false, false, false}, 0x00U, std::nullopt, {}, {}}},
        {false, false, false}},
    {"focus-synthesized-releases-use-caches",
        {{0x030U, 0x62U, 0x62U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x14bU, std::nullopt, 0x102U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x02aU, std::nullopt, 0x1154U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, 0x42U, 0x1042U, {}, "event", 1, true, {false, false, false}, 0, std::nullopt, {}, {}},
            {0x030U, std::nullopt, 0x42U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x62U, 0x42U}},
            {0x14bU, std::nullopt, 0x102U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x102U}},
            {0x02aU, std::nullopt, 0x154U, {}, "event", 1, false, {false, false, false}, 0, std::nullopt, {}, {0x154U}},
            {0, std::nullopt, 0, {}, "clear", 1, true, {false, false, false}, 0, false, {{0x030U, 0x42U}, {0x14bU, 0x102U}}, {}}},
        {false, false, false}}}};

const std::array<FixedKeyCase, 58> fixed_key_cases = {{{"ctrl-left", 0x01dU, 0x152U, 0x4152U, {false, false, false}, {false, false, false}},
    {"ctrl-right", 0x11dU, 0x153U, 0x4153U, {false, false, false}, {false, false, false}}, {"shift-left", 0x02aU, 0x154U, 0x1154U, {false, false, false}, {false, false, false}},
    {"shift-right", 0x036U, 0x155U, 0x1155U, {false, false, false}, {false, false, false}}, {"alt-left", 0x038U, 0x150U, 0x2150U, {false, false, false}, {false, false, false}},
    {"alt-right", 0x138U, 0x151U, 0x2151U, {false, false, false}, {false, false, false}}, {"f1", 0x03bU, 0x12cU, 0x12cU, {false, false, false}, {false, false, false}},
    {"f2", 0x03cU, 0x12dU, 0x12dU, {false, false, false}, {false, false, false}}, {"f3", 0x03dU, 0x12eU, 0x12eU, {false, false, false}, {false, false, false}},
    {"f4", 0x03eU, 0x12fU, 0x12fU, {false, false, false}, {false, false, false}}, {"f5", 0x03fU, 0x130U, 0x130U, {false, false, false}, {false, false, false}},
    {"f6", 0x040U, 0x131U, 0x131U, {false, false, false}, {false, false, false}}, {"f7", 0x041U, 0x132U, 0x132U, {false, false, false}, {false, false, false}},
    {"f8", 0x042U, 0x133U, 0x133U, {false, false, false}, {false, false, false}}, {"f9", 0x043U, 0x134U, 0x134U, {false, false, false}, {false, false, false}},
    {"f10", 0x044U, 0x135U, 0x135U, {false, false, false}, {false, false, false}}, {"f11", 0x057U, 0x136U, 0x136U, {false, false, false}, {false, false, false}},
    {"f12", 0x058U, 0x137U, 0x137U, {false, false, false}, {false, false, false}}, {"pause", 0x045U, 0x10aU, 0x10aU, {false, false, false}, {false, false, false}},
    {"break", 0x146U, 0x10bU, 0x10bU, {false, false, false}, {false, false, false}}, {"home", 0x147U, 0x106U, 0x106U, {false, false, false}, {false, false, false}},
    {"end", 0x14fU, 0x107U, 0x107U, {false, false, false}, {false, false, false}}, {"up", 0x148U, 0x100U, 0x100U, {false, false, false}, {false, false, false}},
    {"down", 0x150U, 0x101U, 0x101U, {false, false, false}, {false, false, false}}, {"left", 0x14bU, 0x102U, 0x102U, {false, false, false}, {false, false, false}},
    {"right", 0x14dU, 0x103U, 0x103U, {false, false, false}, {false, false, false}}, {"page-up", 0x149U, 0x108U, 0x108U, {false, false, false}, {false, false, false}},
    {"page-down", 0x151U, 0x109U, 0x109U, {false, false, false}, {false, false, false}}, {"insert", 0x152U, 0x104U, 0x104U, {false, false, false}, {false, false, false}},
    {"delete", 0x153U, 0x105U, 0x105U, {false, false, false}, {false, false, false}}, {"win-left", 0x15bU, 0x156U, 0x156U, {false, false, false}, {false, false, false}},
    {"win-right", 0x15cU, 0x157U, 0x157U, {false, false, false}, {false, false, false}}, {"menu", 0x15dU, 0x158U, 0x158U, {false, false, false}, {false, false, false}},
    {"caps", 0x03aU, 0x160U, 0x160U, {false, false, false}, {false, false, false}}, {"num", 0x145U, 0x161U, 0x161U, {false, false, false}, {false, false, false}},
    {"scroll", 0x046U, 0x162U, 0x162U, {false, false, false}, {false, false, false}}, {"kp7-num-off", 0x047U, 0x145U, 0x145U, {false, false, false}, {false, false, false}},
    {"kp7-num-on", 0x047U, 0x37U, 0x37U, {false, false, false}, {false, true, false}}, {"kp8-num-off", 0x048U, 0x143U, 0x143U, {false, false, false}, {false, false, false}},
    {"kp8-num-on", 0x048U, 0x38U, 0x38U, {false, false, false}, {false, true, false}}, {"kp9-num-off", 0x049U, 0x148U, 0x148U, {false, false, false}, {false, false, false}},
    {"kp9-num-on", 0x049U, 0x39U, 0x39U, {false, false, false}, {false, true, false}}, {"kp4-num-off", 0x04bU, 0x141U, 0x141U, {false, false, false}, {false, false, false}},
    {"kp4-num-on", 0x04bU, 0x34U, 0x34U, {false, false, false}, {false, true, false}}, {"kp5-num-off", 0x04cU, 0x140U, 0x140U, {false, false, false}, {false, false, false}},
    {"kp5-num-on", 0x04cU, 0x35U, 0x35U, {false, false, false}, {false, true, false}}, {"kp6-num-off", 0x04dU, 0x142U, 0x142U, {false, false, false}, {false, false, false}},
    {"kp6-num-on", 0x04dU, 0x36U, 0x36U, {false, false, false}, {false, true, false}}, {"kp1-num-off", 0x04fU, 0x146U, 0x146U, {false, false, false}, {false, false, false}},
    {"kp1-num-on", 0x04fU, 0x31U, 0x31U, {false, false, false}, {false, true, false}}, {"kp2-num-off", 0x050U, 0x144U, 0x144U, {false, false, false}, {false, false, false}},
    {"kp2-num-on", 0x050U, 0x32U, 0x32U, {false, false, false}, {false, true, false}}, {"kp3-num-off", 0x051U, 0x147U, 0x147U, {false, false, false}, {false, false, false}},
    {"kp3-num-on", 0x051U, 0x33U, 0x33U, {false, false, false}, {false, true, false}}, {"kp0-num-off", 0x052U, 0x149U, 0x149U, {false, false, false}, {false, false, false}},
    {"kp0-num-on", 0x052U, 0x30U, 0x30U, {false, false, false}, {false, true, false}}, {"kp-dot-num-off", 0x053U, 0x14aU, 0x14aU, {false, false, false}, {false, false, false}},
    {"kp-dot-num-on", 0x053U, 0x2eU, 0x2eU, {false, false, false}, {false, true, false}}}};

const std::array<AnsiCase, 24> ansi_cases = {{{"ascii-lower", 0x0062U, false, 0x62U}, {"ascii-upper", 0x0042U, false, 0x42U}, {"shift-symbol", 0x0021U, false, 0x21U},
    {"escape", 0x001bU, false, 0x1bU}, {"latin-umlaut", 0x00e4U, false, 0xe4U}, {"latin-sharp-s", 0x00dfU, false, 0xdfU}, {"euro-cp1252", 0x20acU, false, 0x80U},
    {"unicode-unrepresentable", 0x03b1U, false, std::nullopt}, {"non-byte-unicode", 0x1f414U, false, std::nullopt}, {"ctrl-b", 0x0062U, true, 0x02U},
    {"ctrl-uppercase-b", 0x0042U, true, 0x02U}, {"ctrl-at-zero", 0x0040U, true, 0x00U}, {"ctrl-space-zero", 0x0020U, true, 0x00U}, {"ctrl-2-zero", 0x0032U, true, 0x00U},
    {"ctrl-bracket", 0x005bU, true, 0x1bU}, {"ctrl-backslash", 0x005cU, true, 0x1cU}, {"ctrl-close-bracket", 0x005dU, true, 0x1dU}, {"ctrl-caret", 0x005eU, true, 0x1eU},
    {"ctrl-underscore", 0x005fU, true, 0x1fU}, {"ctrl-question", 0x003fU, true, 0x7fU}, {"ctrl-backspace", 0x0008U, true, 0x7fU}, {"ctrl-return", 0x000dU, true, 0x0aU},
    {"ctrl-tab", 0x0009U, true, 0x09U}, {"ctrl-unsupported-symbol", 0x0021U, true, std::nullopt}}};

void check(bool condition, std::string_view id, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(id) + ": " + std::string(message));
    }
}

void check_state(const Keyboard& keyboard, const StepCase& row, std::string_view id) {
    for (const auto key : row.held) {
        check(keyboard.held(key), id, "Original logical held key missing");
    }

    for (const auto key : row.released) {
        check(!keyboard.held(key), id, "Original logical release missing");
    }

    for (const auto& cached : row.cache) {
        check(keyboard.cached(cached.scan) == cached.key, id, "Per-scan translation cache changed");
    }

    if (row.pending) {
        check(keyboard.pending() == *row.pending, id, "Queue pending boundary changed");
    }
}

void sequence(const SequenceCase& row) {
    const auto id = row.id;
    Keyboard keyboard(row.initial_locks);

    for (const auto& step : row.steps) {
        const auto operation = step.op;

        if (operation == "event" || operation == "repeat") {
            for (int index = 0; index < step.count; ++index) {
                const auto result = keyboard.process({step.scan, step.down, step.translated, step.locks});
                check(result == step.result, id, "Resolved key or modifier word changed");
            }
        } else if (operation == "read") {
            for (int index = 0; index < step.count; ++index) {
                check(keyboard.read() == step.key, id, "Consumed key order, capacity or wrap changed");
            }
        } else if (operation == "clear") {
            keyboard.clear();
        } else {
            throw std::runtime_error(id + ": Unknown fixture operation " + operation);
        }

        check_state(keyboard, step, id);
    }
}

void check_fixed_keys(std::span<const FixedKeyCase> rows) {
    for (const auto& row : rows) {
        const auto id = row.id;
        Keyboard keyboard(row.initial_locks);
        const auto scan = row.scan;
        const auto key = row.key;
        const auto down = keyboard.process({scan, true, 0xffff, row.locks});
        check(down == row.down, id, "Original fixed down mapping");
        check(keyboard.held(key) && keyboard.cached(scan) == key, id, "Fixed down held/cache state");
        check(keyboard.read() == row.down && keyboard.read() == 0, id, "Fixed down queue");
        const auto up = keyboard.process({scan, false, 0xffff, row.locks});
        check(up == key && !keyboard.held(key) && keyboard.cached(scan) == key && !keyboard.pending(), id, "Fixed release ignores translation and enqueues nothing");
    }
}

void check_ansi_bridge(std::span<const AnsiCase> rows) {
    for (const auto& row : rows) {
        check(moorhuhn2::platform::source_to_ascii(row.unicode, row.control) == row.byte, row.id, "Single ANSI byte or control-character bridge");
    }
}

} // namespace

int main() {
    try {
        for (const auto& row : sequences) {
            sequence(row);
        }

        check_fixed_keys(fixed_key_cases);
        check_ansi_bridge(ansi_cases);
        std::cout << sequences.size() << " keyboard sequences, " << fixed_key_cases.size() << " fixed-key vectors, " << ansi_cases.size()
                  << " ANSI/control bridge vectors passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
