#include "platform/paths.hpp"

#include "platform/display.hpp"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace moorhuhn2::platform {
namespace {

std::filesystem::path from_utf8(const char* value) {
    const std::string bytes(value);

    return std::filesystem::path(std::u8string(bytes.begin(), bytes.end()));
}

std::filesystem::path within(const std::filesystem::path& root, const std::filesystem::path& relative) {
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) {
        throw std::invalid_argument("Path must be nonempty and relative");
    }

    for (const auto& part : relative) {
        if (part == "..") {
            throw std::invalid_argument("Path cannot contain '..'");
        }
    }

    const auto resolved = std::filesystem::weakly_canonical(root / relative);
    auto entry = resolved.begin();

    for (const auto& part : root) {
        if (entry == resolved.end() || *entry++ != part) {
            throw std::invalid_argument("Path escapes its root");
        }
    }

    return resolved;
}

} // namespace

AppPaths AppPaths::discover() {
    AppPaths paths;
    const char* base = SDL_GetBasePath();

    if (!base) {
        throw_sdl_error("SDL_GetBasePath");
    }

    const auto resource_root = from_utf8(base);

    if (!std::filesystem::is_directory(resource_root)) {
        throw std::runtime_error("Resource root is missing: " + resource_root.string());
    }

    paths.resource_root_ = std::filesystem::canonical(resource_root);

    std::unique_ptr<char, decltype(&SDL_free)> user(SDL_GetPrefPath("Moorhuhn2", "Moorhuhn2"), SDL_free);

    if (!user) {
        throw_sdl_error("SDL_GetPrefPath");
    }

    paths.user_root_ = std::filesystem::canonical(from_utf8(user.get()));

    return paths;
}

std::filesystem::path AppPaths::user_file(const std::filesystem::path& relative) const {
    return within(user_root_, relative);
}

} // namespace moorhuhn2::platform
