#pragma once

#include "platform/viewport.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace moorhuhn2::platform {

// Construct before SDL resources, which must be destroyed before this lifetime.
class SdlRuntime {
public:
    SdlRuntime();
    ~SdlRuntime();
    SdlRuntime(const SdlRuntime&) = delete;
    SdlRuntime& operator=(const SdlRuntime&) = delete;
};

class Display {
public:
    Display();
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    void refresh();
    void present(std::span<const std::uint8_t> rgba);

    [[nodiscard]] const Viewport& viewport() const noexcept {
        return viewport_;
    }

    [[nodiscard]] SDL_Window* window() const noexcept {
        return window_.get();
    }

private:
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_{nullptr, SDL_DestroyWindow};
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer_{nullptr, SDL_DestroyRenderer};
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture_{nullptr, SDL_DestroyTexture};
    Viewport viewport_;
};

[[noreturn]] void throw_sdl_error(const char* operation);

} // namespace moorhuhn2::platform
