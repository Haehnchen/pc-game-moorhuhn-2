#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace moorhuhn2::assets {

struct RgbaPixel {
    std::uint8_t r{}, g{}, b{}, a{};
    bool operator==(const RgbaPixel&) const = default;
};

static_assert(sizeof(RgbaPixel) == 4 && std::is_standard_layout_v<RgbaPixel>);

// RGB followed by the original BMP reserved byte, which is not alpha.
using PaletteEntry = std::array<std::uint8_t, 4>;
enum class ImageKind {
    truecolor,
    indexed
};

struct ImageView {
    std::uint32_t width{}, height{}, row_stride{};
    ImageKind kind{};
    std::span<const RgbaPixel> rgba;
    std::span<const std::uint8_t> indices;
    std::span<const PaletteEntry> palette;
};

struct AudioView {
    std::uint32_t sample_rate{}, channels{};
    std::uint64_t decoded_frames{};
    std::span<const std::uint8_t> encoded;
};

struct SpriteGrid {
    std::uint32_t columns{}, rows{}, cell_width{}, cell_height{};
    bool operator==(const SpriteGrid&) const = default;
};

struct LoadObserver {
    // Decode this validated priority prefix before the remaining canonical image IDs.
    std::span<const std::string_view> image_order{};
    // Arguments borrow loader storage only during the callback. Throwing aborts the load.
    std::function<void(std::string_view, ImageView)> image_loaded{};
    std::function<void(std::string_view)> audio_loaded{};
};

class AssetError final : public std::runtime_error {
public:
    AssetError(std::string id, std::string path, std::string cause);

    [[nodiscard]] const std::string& id() const noexcept {
        return id_;
    }

    [[nodiscard]] const std::string& path() const noexcept {
        return path_;
    }

    [[nodiscard]] const std::string& cause() const noexcept {
        return cause_;
    }

private:
    std::string id_, path_, cause_;
};

// Loads transactionally. Failure publishes no store. Views borrow immutable storage;
// moving preserves source views; assignment expires views of the old destination.
class AssetStore final {
public:
    [[nodiscard]] static AssetStore load_directory(const std::filesystem::path& directory, const LoadObserver& observer = {});
    [[nodiscard]] static AssetStore load_packages(const std::filesystem::path& directory, const LoadObserver& observer = {});
    ~AssetStore();
    AssetStore(AssetStore&&) noexcept;
    AssetStore& operator=(AssetStore&&) noexcept;
    AssetStore(const AssetStore&) = delete;
    AssetStore& operator=(const AssetStore&) = delete;

    [[nodiscard]] ImageView image(std::string_view id) const;
    [[nodiscard]] AudioView audio(std::string_view id) const;
    [[nodiscard]] std::optional<SpriteGrid> sprite_grid(std::string_view id) const;
    [[nodiscard]] std::span<const std::string> image_ids() const;
    [[nodiscard]] std::span<const std::string> audio_ids() const;

private:
    struct Storage;
    explicit AssetStore(std::unique_ptr<Storage> storage);
    [[nodiscard]] static AssetStore load_impl(const std::filesystem::path& directory, bool packaged, const LoadObserver& observer);
    [[nodiscard]] const Storage& storage() const;
    std::unique_ptr<Storage> storage_;
};

} // namespace moorhuhn2::assets
