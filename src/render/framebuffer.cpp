#include "render/framebuffer.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::render {
namespace {

bool valid_crop(const contracts::ImageRect& crop, const ConvertedImageView& source) {
    return crop.x >= 0 && crop.y >= 0 && crop.width > 0 && crop.height > 0 && static_cast<std::int64_t>(crop.x) + crop.width <= source.width
           && static_cast<std::int64_t>(crop.y) + crop.height <= source.height;
}

template <bool Mirror, bool ColorKey>
void draw_sprite(
    const ConvertedImageView& source, const contracts::SpriteDraw& command, int width, int height, std::span<std::uint16_t> rgb565, std::span<assets::RgbaPixel> rgba) {
    const auto& crop = command.source;
    const auto left = std::max<std::int64_t>(0, command.destination.x);
    const auto top = std::max<std::int64_t>(0, command.destination.y);
    const auto right = std::min<std::int64_t>(width, static_cast<std::int64_t>(command.destination.x) + crop.width);
    const auto bottom = std::min<std::int64_t>(height, static_cast<std::int64_t>(command.destination.y) + crop.height);

    if (left >= right || top >= bottom) {
        return;
    }

    const auto count = static_cast<std::size_t>(right - left);
    const auto local_x = left - command.destination.x;
    const auto source_x = crop.x + (Mirror ? crop.width - 1 - local_x : local_x);

    for (auto y = top; y < bottom; ++y) {
        const auto source_row = static_cast<std::size_t>(crop.y + y - command.destination.y) * source.width;
        const auto destination_row = static_cast<std::size_t>(y) * width + left;

        if constexpr (!Mirror && !ColorKey) {
            std::copy_n(source.rgb565.data() + source_row + source_x, count, rgb565.data() + destination_row);
            std::copy_n(source.rgba.data() + source_row + source_x, count, rgba.data() + destination_row);
        } else {
            for (std::size_t x = 0; x < count; ++x) {
                const auto source_position = source_row + static_cast<std::size_t>(source_x + (Mirror ? -static_cast<std::int64_t>(x) : static_cast<std::int64_t>(x)));
                const auto word = source.rgb565[source_position];

                if constexpr (ColorKey) {
                    if (word == drawing_key_rgb565) {
                        continue;
                    }
                }

                rgb565[destination_row + x] = word;
                rgba[destination_row + x] = source.rgba[source_position];
            }
        }
    }
}

} // namespace

Framebuffer::Framebuffer(int width, int height) : width_(width), height_(height) {
    if (width <= 0 || height <= 0 || static_cast<std::uint64_t>(width) * height > std::numeric_limits<std::size_t>::max() / 4) {
        throw std::invalid_argument("Invalid framebuffer dimensions");
    }

    rgb565_.resize(static_cast<std::size_t>(width) * height);
    rgba_.resize(rgb565_.size());
    clear();
}

void Framebuffer::clear(std::uint16_t color) {
    std::ranges::fill(rgb565_, color);
    std::ranges::fill(rgba_, expand_rgb565(color));
}

void Framebuffer::draw(const ImageBank& images, std::span<const contracts::SpriteDraw> commands) {
    if (commands.empty()) {
        return;
    }

    // Tile animations submit hundreds of consecutive crops from the same image.
    auto image_id = commands.front().image;
    auto image = images.image(image_id);
    const auto resolve = [&](std::string_view id) -> const ConvertedImageView& {
        if (id != image_id) {
            image = images.image(id);
            image_id = id;
        }
        return image;
    };

    for (const auto& command : commands) {
        const auto& source = resolve(command.image);
        if (!valid_crop(command.source, source)) {
            throw std::invalid_argument("Renderer crop outside source image");
        }
    }

    for (const auto& command : commands) {
        const auto& source = resolve(command.image);
        const bool color_key = command.color_key && source.has_color_key;
        // Select once per sprite so the pixel loop only tests the color key itself.
        if (command.mirror_x) {
            if (color_key) {
                draw_sprite<true, true>(source, command, width_, height_, rgb565_, rgba_);
            } else {
                draw_sprite<true, false>(source, command, width_, height_, rgb565_, rgba_);
            }
        } else if (color_key) {
            draw_sprite<false, true>(source, command, width_, height_, rgb565_, rgba_);
        } else {
            draw_sprite<false, false>(source, command, width_, height_, rgb565_, rgba_);
        }
    }
}

std::span<const std::uint8_t> Framebuffer::bytes() const noexcept {
    return {reinterpret_cast<const std::uint8_t*>(rgba_.data()), rgba_.size() * sizeof(assets::RgbaPixel)};
}

std::uint16_t Framebuffer::rgb565_at(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        throw std::out_of_range("Framebuffer pixel out of range");
    }

    return rgb565_[static_cast<std::size_t>(y) * width_ + x];
}

void Framebuffer::set_rgb565_at(int x, int y, std::uint16_t color) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        throw std::out_of_range("Framebuffer pixel out of range");
    }

    const auto index = static_cast<std::size_t>(y) * width_ + x;
    rgb565_[index] = color;
    rgba_[index] = expand_rgb565(color);
}

bool Framebuffer::matches_sprite_pixel(const ImageBank& images, const contracts::SpriteDraw& sprite, contracts::ScreenPoint point) const {
    const auto& crop = sprite.source;
    ConvertedImageView source;

    if (sprite.image.empty()) {
        if (crop.x != 0 || crop.y != 0 || crop.width <= 0 || crop.height <= 0) {
            throw std::invalid_argument("Invalid cleared-scratch hit rectangle");
        }
    } else {
        source = images.image(sprite.image);

        if (!valid_crop(crop, source)) {
            throw std::invalid_argument("Renderer hit crop outside source image");
        }
    }

    const auto x = static_cast<std::int64_t>(point.x) - sprite.destination.x;
    const auto y = static_cast<std::int64_t>(point.y) - sprite.destination.y;

    if (point.x < 0 || point.y < 0 || point.x >= width_ || point.y >= height_ || x < 0 || y < 0 || x >= crop.width || y >= crop.height) {
        return false;
    }

    if (sprite.image.empty()) {
        return rgb565_at(point.x, point.y) == 0;
    }

    const auto source_x = crop.x + (sprite.mirror_x ? crop.width - 1 - x : x);
    const auto source_y = crop.y + y;
    const auto word = source.rgb565[static_cast<std::size_t>(source_y) * source.width + source_x];
    const auto scratch = sprite.color_key && word == drawing_key_rgb565 ? 0 : word;

    return scratch == rgb565_at(point.x, point.y);
}

} // namespace moorhuhn2::render
