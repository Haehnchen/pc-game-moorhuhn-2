#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace moorhuhn2::platform {

namespace key_code {
inline constexpr std::uint16_t accept = 13;
inline constexpr std::uint16_t cancel = 27;
inline constexpr std::uint16_t space = 32;
inline constexpr std::uint16_t backspace = 8;
inline constexpr std::uint16_t boss = 'b';
inline constexpr std::uint16_t up = 0x100;
inline constexpr std::uint16_t down = 0x101;
inline constexpr std::uint16_t left = 0x102;
inline constexpr std::uint16_t right = 0x103;
} // namespace key_code

struct KeyboardLocks {
    bool caps{};
    bool num{};
    bool scroll{};
};

struct KeyboardEvent {
    // Original PC scan code, including its extended bit, in the 9-bit domain.
    std::uint16_t scan{};
    bool down{};
    // One successful ToAscii word, including zero; nullopt means translation failed.
    std::optional<std::uint16_t> translated;
    KeyboardLocks locks;
};

// Bridge a native Unicode key to one Windows-1252 byte. Ctrl applies the original
// control-character convention; composed/dead keys and unrepresentable keys fail.
[[nodiscard]] std::optional<std::uint16_t> source_to_ascii(std::uint32_t character, bool control);

class Keyboard {
public:
    explicit Keyboard(KeyboardLocks locks = {});
    // Returns the queued word for down, or the logical key for up.
    // Zero denotes an ignored translation. Held state changes even when the queue is full.
    std::uint16_t process(const KeyboardEvent& event);
    [[nodiscard]] std::uint16_t read();
    void clear();
    [[nodiscard]] bool held(std::uint16_t key) const;
    [[nodiscard]] bool pending() const;
    [[nodiscard]] std::uint16_t cached(std::uint16_t scan) const;

private:
    [[nodiscard]] std::optional<std::uint16_t> fixed_key(const KeyboardEvent& event);
    [[nodiscard]] bool shift_held() const;
    void enqueue(std::uint16_t key);

    KeyboardLocks locks_;
    std::array<std::uint16_t, 512> scan_keys_{};
    std::array<bool, 512> held_keys_{};
    // One empty slot distinguishes full from empty; overflow drops the newest key.
    std::array<std::uint16_t, 64> queue_{};
    std::size_t read_{};
    std::size_t write_{};
};

} // namespace moorhuhn2::platform
