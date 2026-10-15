#include "platform/display.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace moorhuhn2::platform {

void throw_sdl_error(const char* operation) {
    throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}

SdlRuntime::SdlRuntime() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        const std::string cause = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error("SDL_Init: " + cause);
    }
}

SdlRuntime::~SdlRuntime() {
    SDL_Quit();
}

Display::Display() {
    window_.reset(SDL_CreateWindow("Moorhuhn 2", contracts::logical_width, contracts::logical_height, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY));

    if (!window_) {
        throw_sdl_error("SDL_CreateWindow");
    }

    renderer_.reset(SDL_CreateRenderer(window_.get(), nullptr));

    if (!renderer_) {
        throw_sdl_error("SDL_CreateRenderer");
    }

    if (!SDL_SetRenderVSync(renderer_.get(), 1)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Enable VSync: %s", SDL_GetError());
    }

    texture_.reset(SDL_CreateTexture(renderer_.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, contracts::logical_width, contracts::logical_height));

    if (!texture_) {
        throw_sdl_error("SDL_CreateTexture");
    }

    if (!SDL_SetTextureScaleMode(texture_.get(), SDL_SCALEMODE_NEAREST) || !SDL_SetTextureBlendMode(texture_.get(), SDL_BLENDMODE_NONE)) {
        throw_sdl_error("Configure presentation texture");
    }

    if (!SDL_SyncWindow(window_.get())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Wait for fullscreen window: %s", SDL_GetError());
    }

    refresh();
}

void Display::refresh() {
    int window_width{};
    int window_height{};
    int pixel_width{};
    int pixel_height{};

    if (!SDL_GetWindowSize(window_.get(), &window_width, &window_height) || !SDL_GetRenderOutputSize(renderer_.get(), &pixel_width, &pixel_height)) {
        throw_sdl_error("Read window dimensions");
    }

    const bool minimized = (SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_MINIMIZED) != 0;
    viewport_ = Viewport::fit(window_width, window_height, minimized ? 0 : pixel_width, minimized ? 0 : pixel_height);
}

void Display::present(std::span<const std::uint8_t> rgba) {
    constexpr std::size_t expected = static_cast<std::size_t>(contracts::logical_width) * contracts::logical_height * 4;

    if (rgba.size() != expected) {
        throw std::invalid_argument("Presentation requires 640 x 480 RGBA bytes");
    }

    refresh();

    if (!viewport_.drawable()) {
        return;
    }

    const SDL_FRect destination{static_cast<float>(viewport_.x), static_cast<float>(viewport_.y), static_cast<float>(viewport_.width), static_cast<float>(viewport_.height)};

    if (!SDL_UpdateTexture(texture_.get(), nullptr, rgba.data(), contracts::logical_width * 4) || !SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255)
        || !SDL_RenderClear(renderer_.get()) || !SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, &destination) || !SDL_RenderPresent(renderer_.get())) {
        throw_sdl_error("Present frame");
    }
}

} // namespace moorhuhn2::platform
