#include "render/image_bank.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace moorhuhn2::render {

ImageBank::ImageBank(std::span<const NamedImageView> images) {
    for (const auto& entry : images) {
        add(entry.id, entry.image);
    }
}

ImageBank::ImageBank(const assets::AssetStore& assets) {
    for (const auto& id : assets.image_ids()) {
        add(id, assets.image(id));
    }
}

void ImageBank::add(std::string_view id, assets::ImageView source) {
    if (id.empty() || source.width == 0 || source.height == 0 || source.width > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || source.height > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || source.row_stride < source.width) {
        throw std::invalid_argument("Invalid renderer image dimensions or identity");
    }

    const auto source_size = static_cast<std::uint64_t>(source.row_stride) * source.height;

    if (source.rgba.size() < source_size || (source.kind == assets::ImageKind::indexed && (source.indices.size() < source_size || source.palette.empty()))) {
        throw std::invalid_argument("Incomplete renderer image storage");
    }

    Image output{static_cast<int>(source.width), static_cast<int>(source.height), {}, {}};
    const auto count = static_cast<std::size_t>(source.width) * source.height;
    output.rgb565.reserve(count);
    output.rgba.reserve(count);

    for (std::uint32_t y = 0; y < source.height; ++y) {
        for (std::uint32_t x = 0; x < source.width; ++x) {
            const auto position = static_cast<std::size_t>(y) * source.row_stride + x;
            auto pixel = source.rgba[position];

            if (source.kind == assets::ImageKind::indexed) {
                const auto index = source.indices[position];

                if (index >= source.palette.size()) {
                    throw std::invalid_argument("Renderer palette index out of range");
                }

                const auto& palette = source.palette[index];
                pixel = {palette[0], palette[1], palette[2], 255};
            }

            const auto word = to_rgb565(pixel);
            output.has_color_key |= word == drawing_key_rgb565;
            output.rgb565.push_back(word);
            output.rgba.push_back(expand_rgb565(word));
        }
    }

    if (!images_.emplace(id, std::move(output)).second) {
        throw std::invalid_argument("Duplicate renderer image identity");
    }
}

ConvertedImageView ImageBank::image(std::string_view id) const {
    const auto iterator = images_.find(id);

    if (iterator == images_.end()) {
        throw std::out_of_range("Unknown renderer image: " + std::string(id));
    }

    const auto& result = iterator->second;

    return {result.width, result.height, result.rgb565, result.rgba, result.has_color_key};
}

std::uint16_t ImageBank::rgb565_at(std::string_view id, int x, int y) const {
    const auto source = image(id);

    if (x < 0 || y < 0 || x >= source.width || y >= source.height) {
        throw std::out_of_range("Renderer source pixel out of range");
    }

    return source.rgb565[static_cast<std::size_t>(y) * source.width + x];
}

bool ImageBank::opaque(std::string_view id, int x, int y, bool color_key) const {
    const auto source = image(id);

    if (x < 0 || y < 0 || x >= source.width || y >= source.height) {
        return false;
    }

    return !color_key || source.rgb565[static_cast<std::size_t>(y) * source.width + x] != drawing_key_rgb565;
}

} // namespace moorhuhn2::render
