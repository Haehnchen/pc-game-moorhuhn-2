#include "platform/input.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>

#include <array>
#include <utility>

namespace moorhuhn2::platform {
namespace {

KeyboardLocks keyboard_locks(SDL_Keymod modifiers) {
    return {(modifiers & SDL_KMOD_CAPS) != 0, (modifiers & SDL_KMOD_NUM) != 0, (modifiers & SDL_KMOD_SCROLL) != 0};
}

std::optional<std::uint16_t> source_scan(SDL_Scancode scancode) {
    if (scancode >= SDL_SCANCODE_COUNT) {
        return std::nullopt;
    }

    static const auto scans = [] {
        constexpr std::pair<SDL_Scancode, std::uint16_t> pc_scans[]{
            {SDL_SCANCODE_UNKNOWN, 0x000},
            {SDL_SCANCODE_ESCAPE, 0x001},
            {SDL_SCANCODE_1, 0x002},
            {SDL_SCANCODE_2, 0x003},
            {SDL_SCANCODE_3, 0x004},
            {SDL_SCANCODE_4, 0x005},
            {SDL_SCANCODE_5, 0x006},
            {SDL_SCANCODE_6, 0x007},
            {SDL_SCANCODE_7, 0x008},
            {SDL_SCANCODE_8, 0x009},
            {SDL_SCANCODE_9, 0x00a},
            {SDL_SCANCODE_0, 0x00b},
            {SDL_SCANCODE_MINUS, 0x00c},
            {SDL_SCANCODE_EQUALS, 0x00d},
            {SDL_SCANCODE_BACKSPACE, 0x00e},
            {SDL_SCANCODE_TAB, 0x00f},
            {SDL_SCANCODE_Q, 0x010},
            {SDL_SCANCODE_W, 0x011},
            {SDL_SCANCODE_E, 0x012},
            {SDL_SCANCODE_R, 0x013},
            {SDL_SCANCODE_T, 0x014},
            {SDL_SCANCODE_Y, 0x015},
            {SDL_SCANCODE_U, 0x016},
            {SDL_SCANCODE_I, 0x017},
            {SDL_SCANCODE_O, 0x018},
            {SDL_SCANCODE_P, 0x019},
            {SDL_SCANCODE_LEFTBRACKET, 0x01a},
            {SDL_SCANCODE_RIGHTBRACKET, 0x01b},
            {SDL_SCANCODE_RETURN, 0x01c},
            {SDL_SCANCODE_LCTRL, 0x01d},
            {SDL_SCANCODE_A, 0x01e},
            {SDL_SCANCODE_S, 0x01f},
            {SDL_SCANCODE_D, 0x020},
            {SDL_SCANCODE_F, 0x021},
            {SDL_SCANCODE_G, 0x022},
            {SDL_SCANCODE_H, 0x023},
            {SDL_SCANCODE_J, 0x024},
            {SDL_SCANCODE_K, 0x025},
            {SDL_SCANCODE_L, 0x026},
            {SDL_SCANCODE_SEMICOLON, 0x027},
            {SDL_SCANCODE_APOSTROPHE, 0x028},
            {SDL_SCANCODE_GRAVE, 0x029},
            {SDL_SCANCODE_LSHIFT, 0x02a},
            {SDL_SCANCODE_BACKSLASH, 0x02b},
            {SDL_SCANCODE_Z, 0x02c},
            {SDL_SCANCODE_X, 0x02d},
            {SDL_SCANCODE_C, 0x02e},
            {SDL_SCANCODE_V, 0x02f},
            {SDL_SCANCODE_B, 0x030},
            {SDL_SCANCODE_N, 0x031},
            {SDL_SCANCODE_M, 0x032},
            {SDL_SCANCODE_COMMA, 0x033},
            {SDL_SCANCODE_PERIOD, 0x034},
            {SDL_SCANCODE_SLASH, 0x035},
            {SDL_SCANCODE_RSHIFT, 0x036},
            {SDL_SCANCODE_KP_MULTIPLY, 0x037},
            {SDL_SCANCODE_LALT, 0x038},
            {SDL_SCANCODE_SPACE, 0x039},
            {SDL_SCANCODE_CAPSLOCK, 0x03a},
            {SDL_SCANCODE_F1, 0x03b},
            {SDL_SCANCODE_F2, 0x03c},
            {SDL_SCANCODE_F3, 0x03d},
            {SDL_SCANCODE_F4, 0x03e},
            {SDL_SCANCODE_F5, 0x03f},
            {SDL_SCANCODE_F6, 0x040},
            {SDL_SCANCODE_F7, 0x041},
            {SDL_SCANCODE_F8, 0x042},
            {SDL_SCANCODE_F9, 0x043},
            {SDL_SCANCODE_F10, 0x044},
            {SDL_SCANCODE_PAUSE, 0x045},
            {SDL_SCANCODE_SCROLLLOCK, 0x046},
            {SDL_SCANCODE_KP_7, 0x047},
            {SDL_SCANCODE_KP_8, 0x048},
            {SDL_SCANCODE_KP_9, 0x049},
            {SDL_SCANCODE_KP_MINUS, 0x04a},
            {SDL_SCANCODE_KP_4, 0x04b},
            {SDL_SCANCODE_KP_5, 0x04c},
            {SDL_SCANCODE_KP_6, 0x04d},
            {SDL_SCANCODE_KP_PLUS, 0x04e},
            {SDL_SCANCODE_KP_1, 0x04f},
            {SDL_SCANCODE_KP_2, 0x050},
            {SDL_SCANCODE_KP_3, 0x051},
            {SDL_SCANCODE_KP_0, 0x052},
            {SDL_SCANCODE_KP_PERIOD, 0x053},
            {SDL_SCANCODE_NONUSBACKSLASH, 0x056},
            {SDL_SCANCODE_F11, 0x057},
            {SDL_SCANCODE_F12, 0x058},
            {SDL_SCANCODE_KP_ENTER, 0x11c},
            {SDL_SCANCODE_RCTRL, 0x11d},
            {SDL_SCANCODE_KP_DIVIDE, 0x135},
            {SDL_SCANCODE_PRINTSCREEN, 0x137},
            {SDL_SCANCODE_RALT, 0x138},
            {SDL_SCANCODE_NUMLOCKCLEAR, 0x145},
            {SDL_SCANCODE_CANCEL, 0x146},
            {SDL_SCANCODE_HOME, 0x147},
            {SDL_SCANCODE_UP, 0x148},
            {SDL_SCANCODE_PAGEUP, 0x149},
            {SDL_SCANCODE_LEFT, 0x14b},
            {SDL_SCANCODE_RIGHT, 0x14d},
            {SDL_SCANCODE_END, 0x14f},
            {SDL_SCANCODE_DOWN, 0x150},
            {SDL_SCANCODE_PAGEDOWN, 0x151},
            {SDL_SCANCODE_INSERT, 0x152},
            {SDL_SCANCODE_DELETE, 0x153},
            {SDL_SCANCODE_LGUI, 0x15b},
            {SDL_SCANCODE_RGUI, 0x15c},
            {SDL_SCANCODE_APPLICATION, 0x15d},
        };
        std::array<std::uint16_t, SDL_SCANCODE_COUNT> mapping{};
        mapping.fill(0xffff);
        constexpr std::size_t scan_slots = 512;
        static_assert(SDL_SCANCODE_COUNT <= scan_slots);
        std::array<bool, scan_slots> assigned{};

        for (const auto& [sdl, pc] : pc_scans) {
            mapping[sdl] = pc;
            assigned[pc] = true;
        }

        // SDL's additional device keys have no original PC identity. Allocate
        // stable unused scan slots, preserving their independent translation caches.
        std::uint16_t next{};

        for (auto& scan : mapping) {
            if (scan == 0xffff) {
                while (next < scan_slots && assigned[next]) {
                    ++next;
                }

                if (next == scan_slots) {
                    break;
                }

                scan = next;
                assigned[next] = true;
            }
        }

        return mapping;
    }();

    return scans[scancode];
}

std::optional<std::uint16_t> translated_key(const SDL_KeyboardEvent& event) {
    auto character = SDL_GetKeyFromScancode(event.scancode, event.mod, false);
    // SDL keeps these keypad keycodes outside Unicode even when key_event=false.
    switch (character) {
        case SDLK_KP_ENTER:
            character = '\r';
            break;
        case SDLK_KP_DIVIDE:
            character = '/';
            break;
        case SDLK_KP_MULTIPLY:
            character = '*';
            break;
        case SDLK_KP_MINUS:
            character = '-';
            break;
        case SDLK_KP_PLUS:
            character = '+';
            break;
        case SDLK_KP_EQUALS:
        case SDLK_KP_EQUALSAS400:
            character = '=';
            break;
        case SDLK_KP_COMMA:
            character = ',';
            break;
        case SDLK_UNKNOWN:
            return std::nullopt;
        default:
            break;
    }

    return source_to_ascii(character, (event.mod & SDL_KMOD_CTRL) != 0);
}

std::optional<contracts::InputControl> button_control(Uint8 button) {
    if (button == SDL_BUTTON_LEFT) {
        return contracts::InputControl::primary;
    }

    if (button == SDL_BUTTON_RIGHT) {
        return contracts::InputControl::secondary;
    }

    return std::nullopt;
}

std::optional<Coordinate> hovered_mouse_position(SDL_Window* window) {
    if (SDL_GetMouseFocus() != window) {
        return std::nullopt;
    }

    float x{};
    float y{};
    SDL_GetMouseState(&x, &y);
    return Coordinate{x, y};
}

} // namespace

InputAdapter::InputAdapter(Display& display)
    : display_(display), keyboard_(keyboard_locks(SDL_GetModState())), focused_((SDL_GetWindowFlags(display.window()) & SDL_WINDOW_INPUT_FOCUS) != 0),
      window_pointer_(hovered_mouse_position(display.window())) {}

std::uint16_t InputAdapter::take_key() {
    return keyboard_.read();
}

void InputAdapter::clear_keys() {
    keyboard_.clear();
}

bool InputAdapter::key_held(std::uint16_t key) const {
    return keyboard_.held(key);
}

contracts::InputFrame InputAdapter::poll() {
    display_.refresh();
    contracts::InputFrame frame;
    const auto window_id = SDL_GetWindowID(display_.window());
    const auto pointer = [&]() -> std::optional<contracts::ScreenPoint> {
        return window_pointer_ && focused_ ? display_.viewport().window_to_logical(*window_pointer_) : std::nullopt;
    };

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                frame.quit_requested = true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (event.window.windowID == window_id) {
                    frame.quit_requested = true;
                }
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (event.window.windowID != window_id) {
                    break;
                }

                focused_ = event.type == SDL_EVENT_WINDOW_FOCUS_GAINED;

                if (!focused_) {
                    window_pointer_.reset();
                } else if (const auto position = hovered_mouse_position(display_.window())) {
                    window_pointer_ = position;
                }
                break;
            case SDL_EVENT_WINDOW_MOUSE_ENTER:
                if (event.window.windowID != window_id) {
                    break;
                }

                if (const auto position = hovered_mouse_position(display_.window())) {
                    window_pointer_ = position;
                }
                break;
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                if (event.window.windowID == window_id) {
                    window_pointer_.reset();
                }
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_MINIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
                if (event.window.windowID != window_id) {
                    break;
                }

                display_.refresh();
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (event.motion.windowID == window_id) {
                    window_pointer_ = Coordinate{event.motion.x, event.motion.y};
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.windowID != window_id) {
                    break;
                }

                window_pointer_ = Coordinate{event.button.x, event.button.y};

                if (const auto control = button_control(event.button.button)) {
                    frame.actions.push_back({*control, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? contracts::InputEdge::pressed : contracts::InputEdge::released});
                }
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                if (event.key.windowID != window_id) {
                    break;
                }

                const auto down = event.type == SDL_EVENT_KEY_DOWN;

                if (const auto scan = source_scan(event.key.scancode)) {
                    keyboard_.process({*scan, down, down ? translated_key(event.key) : std::nullopt, keyboard_locks(event.key.mod)});
                }
                break;
            }
            default:
                break;
        }
    }

    frame.focused = focused_;
    frame.pointer = pointer();
    frame.clamped_pointer = window_pointer_ && focused_ ? display_.viewport().window_to_logical_clamped(*window_pointer_) : std::nullopt;

    return frame;
}

} // namespace moorhuhn2::platform
