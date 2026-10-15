#pragma once

#include <filesystem>

namespace moorhuhn2::platform {

class AppPaths {
public:
    // Resources live beside the executable; user files live in the SDL preference directory.
    [[nodiscard]] static AppPaths discover();

    [[nodiscard]] const std::filesystem::path& resource_root() const noexcept {
        return resource_root_;
    }

    [[nodiscard]] std::filesystem::path user_file(const std::filesystem::path& relative) const;

private:
    std::filesystem::path resource_root_;
    std::filesystem::path user_root_;
};

} // namespace moorhuhn2::platform
