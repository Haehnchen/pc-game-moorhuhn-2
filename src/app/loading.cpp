#include "app/loading.hpp"

#include "ui/loading.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <stdexcept>

namespace moorhuhn2::app {

std::optional<LoadedResources> load_resources(const platform::AppPaths& paths, platform::Display& display, render::ImageBank& images, render::Framebuffer& framebuffer) {
    bool cancelled{};
    std::size_t next_milestone{};
    const auto milestones = ui::loading_milestones();

    const auto pump = [&] {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(display.window()))) {
                cancelled = true;
            }
        }

        if (cancelled) {
            throw std::runtime_error("Loading cancelled");
        }
    };

    const auto completed = [&](std::string_view asset) {
        pump();
        const auto milestone = std::ranges::find(milestones, asset, &ui::LoadingMilestone::asset);
        if (milestone == milestones.end()) {
            return; // Bootstrap art, cursor/font and unused original audio have no progress call.
        }

        if (next_milestone >= milestones.size() || milestones[next_milestone].asset != asset) {
            throw std::logic_error("Resource preparation differs from the original loading order");
        }

        const auto draws = ui::loading_draws(milestone->progress);
        framebuffer.clear();
        framebuffer.draw(images, draws);
        // Present even before focus: Wayland needs this first buffer to map the window.
        display.present(framebuffer.bytes());
        ++next_milestone;

        pump();
    };

    try {
        pump();
        const assets::LoadObserver observer{ui::loading_image_order(),
            [&](std::string_view id, assets::ImageView image) {
                pump();
                images.add(id, image);
                completed(id);
            },
            [&](std::string_view) {
                pump();
            }};
        auto assets = assets::AssetStore::load_packages(paths.resource_root(), observer);
        const audio::LoadObserver audio_observer{ui::loading_audio_order(), completed};
        audio::Engine audio(assets, audio::OutputMode::device, 44100, audio_observer);
        pump();
        if (next_milestone != milestones.size()) {
            throw std::logic_error("Resource preparation did not complete the original loading milestones");
        }

        return LoadedResources{render::FrameCatalog(assets), std::move(audio)};
    } catch (const std::exception&) {
        if (cancelled) {
            return std::nullopt;
        }

        throw;
    }
}

} // namespace moorhuhn2::app
