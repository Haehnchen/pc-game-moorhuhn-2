#include "platform/keyboard.hpp"

#include <utility>

namespace moorhuhn2::platform {

std::optional<std::uint16_t> source_to_ascii(std::uint32_t character, bool control) {
    if (control) {
        if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')) {
            return static_cast<std::uint16_t>(character & 0x1f);
        }

        switch (character) {
            case '@':
            case '2':
            case ' ':
                return 0;
            case '[':
            case '3':
                return 0x1b;
            case '\\':
            case '4':
                return 0x1c;
            case ']':
            case '5':
                return 0x1d;
            case '^':
            case '6':
                return 0x1e;
            case '_':
            case '7':
                return 0x1f;
            case '?':
            case '8':
            case 0x08:
                return 0x7f;
            case 0x0d:
                return 0x0a;
            case 0x09:
            case 0x1b:
                return static_cast<std::uint16_t>(character);
            default:
                return std::nullopt;
        }
    }

    if (character <= 0x7f || (character >= 0xa0 && character <= 0xff)) {
        return static_cast<std::uint16_t>(character);
    }

    // Windows ToAscii returns an ANSI byte rather than a Unicode code point.
    constexpr std::pair<std::uint32_t, std::uint16_t> windows_1252[]{
        {0x20ac, 0x80},
        {0x201a, 0x82},
        {0x0192, 0x83},
        {0x201e, 0x84},
        {0x2026, 0x85},
        {0x2020, 0x86},
        {0x2021, 0x87},
        {0x02c6, 0x88},
        {0x2030, 0x89},
        {0x0160, 0x8a},
        {0x2039, 0x8b},
        {0x0152, 0x8c},
        {0x017d, 0x8e},
        {0x2018, 0x91},
        {0x2019, 0x92},
        {0x201c, 0x93},
        {0x201d, 0x94},
        {0x2022, 0x95},
        {0x2013, 0x96},
        {0x2014, 0x97},
        {0x02dc, 0x98},
        {0x2122, 0x99},
        {0x0161, 0x9a},
        {0x203a, 0x9b},
        {0x0153, 0x9c},
        {0x017e, 0x9e},
        {0x0178, 0x9f},
    };

    for (const auto& [unicode, byte] : windows_1252) {
        if (character == unicode) {
            return byte;
        }
    }

    return std::nullopt;
}

Keyboard::Keyboard(KeyboardLocks locks) : locks_(locks) {}

bool Keyboard::shift_held() const {
    return held_keys_[0x154] || held_keys_[0x155];
}

std::optional<std::uint16_t> Keyboard::fixed_key(const KeyboardEvent& event) {
    switch (event.scan) {
        case 0x01d:
            return 0x152;
        case 0x11d:
            return 0x153;
        case 0x02a:
            return 0x154;
        case 0x036:
            return 0x155;
        case 0x038:
            return 0x150;
        case 0x138:
            return 0x151;
        case 0x028:
            return shift_held() ? 0x22 : 0x27;
        case 0x029:
            return shift_held() ? 0x7e : 0x60;
        case 0x03a:
            locks_.caps = event.locks.caps;

            return 0x160;
        case 0x03b:
        case 0x03c:
        case 0x03d:
        case 0x03e:
        case 0x03f:
        case 0x040:
        case 0x041:
        case 0x042:
        case 0x043:
        case 0x044:
            return static_cast<std::uint16_t>(0x12c + event.scan - 0x03b);
        case 0x057:
            return 0x136;
        case 0x058:
            return 0x137;
        case 0x045:
            return 0x10a;
        case 0x046:
            locks_.scroll = event.locks.scroll;

            return 0x162;
        case 0x145:
            locks_.num = event.locks.num;

            return 0x161;
        case 0x146:
            return 0x10b;
        case 0x147:
            return 0x106;
        case 0x14f:
            return 0x107;
        case 0x148:
            return key_code::up;
        case 0x150:
            return key_code::down;
        case 0x14b:
            return key_code::left;
        case 0x14d:
            return key_code::right;
        case 0x149:
            return 0x108;
        case 0x151:
            return 0x109;
        case 0x152:
            return 0x104;
        case 0x153:
            return 0x105;
        case 0x15b:
            return 0x156;
        case 0x15c:
            return 0x157;
        case 0x15d:
            return 0x158;
        case 0x047:
            return locks_.num ? 0x37 : 0x145;
        case 0x048:
            return locks_.num ? 0x38 : 0x143;
        case 0x049:
            return locks_.num ? 0x39 : 0x148;
        case 0x04b:
            return locks_.num ? 0x34 : 0x141;
        case 0x04c:
            return locks_.num ? 0x35 : 0x140;
        case 0x04d:
            return locks_.num ? 0x36 : 0x142;
        case 0x04f:
            return locks_.num ? 0x31 : 0x146;
        case 0x050:
            return locks_.num ? 0x32 : 0x144;
        case 0x051:
            return locks_.num ? 0x33 : 0x147;
        case 0x052:
            return locks_.num ? 0x30 : 0x149;
        case 0x053:
            return locks_.num ? 0x2e : 0x14a;
        default:
            return std::nullopt;
    }
}

std::uint16_t Keyboard::process(const KeyboardEvent& event) {
    if (event.scan >= scan_keys_.size()) {
        return 0;
    }

    auto key = fixed_key(event);

    if (!key) {
        key = event.down && event.translated ? *event.translated : scan_keys_[event.scan];
        // The original default C locale's toupper changes only ASCII letters.
        if (event.down && event.translated && locks_.caps && *key >= 'a' && *key <= 'z') {
            *key = static_cast<std::uint16_t>(*key - ('a' - 'A'));
        }
    }

    // A successful zero translation suppresses the key instead of reusing its cache.
    // External words must also fit the original 512-entry held array.
    if (*key == 0 || *key >= held_keys_.size()) {
        return 0;
    }

    const auto previous = scan_keys_[event.scan];

    if (previous != *key) {
        held_keys_[previous] = false;
    }

    scan_keys_[event.scan] = *key;
    held_keys_[*key] = event.down;

    if (!event.down) {
        return *key;
    }

    // Modifiers are attached after held-state changes, including modifier keydowns.
    if (shift_held()) {
        *key |= 0x1000;
    }

    if (held_keys_[0x150] || held_keys_[0x151]) {
        *key |= 0x2000;
    }

    if (held_keys_[0x152] || held_keys_[0x153]) {
        *key |= 0x4000;
    }

    enqueue(*key);

    return *key;
}

void Keyboard::enqueue(std::uint16_t key) {
    const auto next = (write_ + 1) % queue_.size();

    if (next != read_) {
        queue_[write_] = key;
        write_ = next;
    }
}

std::uint16_t Keyboard::read() {
    if (!pending()) {
        return 0;
    }

    const auto key = queue_[read_];
    read_ = (read_ + 1) % queue_.size();

    return key;
}

void Keyboard::clear() {
    write_ = read_;
}

bool Keyboard::held(std::uint16_t key) const {
    return held_keys_[key & 0x1ff];
}

bool Keyboard::pending() const {
    return write_ != read_;
}

std::uint16_t Keyboard::cached(std::uint16_t scan) const {
    return scan < scan_keys_.size() ? scan_keys_[scan] : 0;
}

} // namespace moorhuhn2::platform
