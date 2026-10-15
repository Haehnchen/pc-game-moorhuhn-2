#pragma once

#include "assets/asset_store.hpp"
#include "audio/engine.hpp"
#include "platform/display.hpp"
#include "platform/paths.hpp"
#include "render/frame_catalog.hpp"
#include "render/framebuffer.hpp"
#include "render/image_bank.hpp"

#include <optional>

namespace moorhuhn2::app {

struct LoadedResources {
    render::FrameCatalog catalog;
    audio::Engine audio;
};

std::optional<LoadedResources> load_resources(const platform::AppPaths& paths, platform::Display& display, render::ImageBank& images, render::Framebuffer& framebuffer);

} // namespace moorhuhn2::app
