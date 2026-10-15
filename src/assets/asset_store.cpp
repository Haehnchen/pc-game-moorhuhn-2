#include "assets/asset_store.hpp"

#include "assets/asset_package.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <charconv>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

namespace moorhuhn2::assets {
namespace {
using Bytes = std::vector<std::uint8_t>;

struct Context {
    std::string id;
    std::string path;

    [[noreturn]] void fail(std::string cause) const {
        throw AssetError(id, path, std::move(cause));
    }
};

void require_path(std::string_view value, const Context& context) {
    if (!detail::safe_path(value)) {
        context.fail("unsafe relative path");
    }
}

std::uint64_t read_integer(std::string_view value, const Context& context, std::uint64_t low, std::uint64_t high) {
    std::uint64_t result{};
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);

    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
        context.fail("expected unsigned integer");
    }

    if (result < low || result > high) {
        context.fail("integer outside bounds");
    }

    return result;
}

std::vector<std::string> read_row(std::istream& stream, std::string_view label, std::size_t count, const Context& context) {
    std::string line;
    std::getline(stream, line);
    std::istringstream row(line);
    std::vector<std::string> fields;

    for (std::string field; row >> field;) {
        fields.push_back(std::move(field));
    }

    if (fields.size() != count || fields.front() != label) {
        context.fail("expected " + std::string(label) + " row with " + std::to_string(count) + " fields");
    }

    return fields;
}

struct ImageRecord {
    std::string id;
    ImageKind kind{};
    std::uint32_t width{}, height{};
    std::uint64_t png_bytes{}, indices_bytes{}, palette_bytes{};
    std::optional<SpriteGrid> grid;
};

struct AudioRecord {
    std::string id;
    std::uint64_t bytes{};
    AudioView metadata;
};

struct Manifest {
    std::vector<ImageRecord> images;
    std::vector<AudioRecord> audio;
};

Manifest parse_manifest(std::span<const std::uint8_t> bytes, Context context) {
    std::istringstream stream(std::string(bytes.begin(), bytes.end()));
    read_row(stream, "MH2ASSETS", 1, context);
    Manifest manifest;
    const auto image_count = read_integer(read_row(stream, "images", 2, context)[1], context, 1, 256);
    std::string previous;

    for (std::uint64_t index = 0; index < image_count; ++index) {
        const auto fields = read_row(stream, "image", 12, context);
        ImageRecord record;
        record.id = fields[1];
        context = {record.id, "manifest.txt"};
        require_path(record.id, context);

        if (!record.id.starts_with("gfx/") || (!previous.empty() && previous >= record.id)) {
            context.fail("duplicate, unordered or non-image asset ID");
        }

        previous = record.id;

        if (fields[2] != "indexed" && fields[2] != "truecolor") {
            context.fail("unknown image kind");
        }

        record.kind = fields[2] == "indexed" ? ImageKind::indexed : ImageKind::truecolor;
        record.width = static_cast<std::uint32_t>(read_integer(fields[3], context, 1, 32768));
        record.height = static_cast<std::uint32_t>(read_integer(fields[4], context, 1, 32768));

        if (std::uint64_t(record.width) * record.height > 25'000'000) {
            context.fail("image exceeds pixel allocation limit");
        }

        record.png_bytes = read_integer(fields[5], context, 1, detail::package_max_bytes);
        record.indices_bytes = read_integer(fields[6], context, 0, detail::package_max_bytes);
        record.palette_bytes = read_integer(fields[7], context, 0, detail::package_max_bytes);

        if (record.kind == ImageKind::indexed) {
            if (record.indices_bytes != std::uint64_t(record.width) * record.height || record.palette_bytes < 4 || record.palette_bytes > 1024 || record.palette_bytes % 4 != 0) {
                context.fail("invalid index/palette lengths");
            }
        } else if (record.indices_bytes != 0 || record.palette_bytes != 0) {
            context.fail("truecolor image has indexed lengths");
        }

        const SpriteGrid grid{static_cast<std::uint32_t>(read_integer(fields[8], context, 0, 32768)), static_cast<std::uint32_t>(read_integer(fields[9], context, 0, 32768)),
            static_cast<std::uint32_t>(read_integer(fields[10], context, 0, 32768)), static_cast<std::uint32_t>(read_integer(fields[11], context, 0, 32768))};

        if (grid.columns != 0 || grid.rows != 0 || grid.cell_width != 0 || grid.cell_height != 0) {
            if (grid.columns == 0 || grid.rows == 0 || grid.columns > record.width || grid.rows > record.height || grid.cell_width != record.width / grid.columns
                || grid.cell_height != record.height / grid.rows) {
                context.fail("sprite grid differs from image dimensions");
            }

            record.grid = grid;
        }

        manifest.images.push_back(std::move(record));
    }

    context = {"manifest", "manifest.txt"};
    const auto audio_count = read_integer(read_row(stream, "audio", 2, context)[1], context, 0, 256);
    previous.clear();

    for (std::uint64_t index = 0; index < audio_count; ++index) {
        const auto fields = read_row(stream, "audio", 6, context);
        AudioRecord record;
        record.id = fields[1];
        context = {record.id, "manifest.txt"};
        require_path(record.id, context);

        if (!record.id.starts_with("sfx/") || (!previous.empty() && previous >= record.id)) {
            context.fail("duplicate, unordered or non-audio asset ID");
        }

        previous = record.id;
        record.bytes = read_integer(fields[2], context, 1, detail::package_max_bytes);
        record.metadata.sample_rate = static_cast<std::uint32_t>(read_integer(fields[3], context, 1, 384000));
        record.metadata.channels = static_cast<std::uint32_t>(read_integer(fields[4], context, 1, 8));
        record.metadata.decoded_frames = read_integer(fields[5], context, 1, 1'000'000'000);
        manifest.audio.push_back(std::move(record));
    }

    std::string trailing;

    if (std::getline(stream, trailing)) {
        context.fail("unexpected trailing manifest data");
    }

    return manifest;
}

class AssetInput final {
public:
    AssetInput(std::filesystem::path directory, bool packaged) : root_(std::filesystem::canonical(directory)) {
        if (!std::filesystem::is_directory(root_)) {
            throw std::runtime_error("Asset root is not a directory");
        }

        if (packaged) {
            images_.emplace(root_ / "images.pak");
            audio_.emplace(root_ / "audio.pak");
            const auto left = images_->read("manifest.txt"), right = audio_->read("manifest.txt");

            if (left.size() != right.size() || !std::equal(left.begin(), left.end(), right.begin())) {
                throw std::runtime_error("Image/audio packages have different manifests");
            }
        }
    }

    Bytes read(std::string_view name, bool audio, const Context& context) {
        require_path(name, context);
        auto& used = audio ? audio_used_ : images_used_;

        if (!used.insert(std::string(name)).second) {
            context.fail("duplicate runtime path");
        }

        try {
            const auto& package = audio ? audio_ : images_;

            if (package) {
                const auto bytes = package->read(name);

                return {bytes.begin(), bytes.end()};
            }

            const auto real = std::filesystem::canonical(root_ / name);
            const auto relative = real.lexically_relative(root_);

            if (relative.empty() || !detail::safe_path(relative.generic_string())) {
                context.fail("runtime file escapes asset root");
            }

            return detail::read_file(real);
        } catch (const AssetError&) {
            throw;
        } catch (const std::exception& error) {
            context.fail(error.what());
        }
    }

    void complete() const {
        if (images_ && images_used_.size() != images_->entries().size()) {
            throw std::runtime_error("Image package contains unreferenced data");
        }

        // Its matching manifest was verified separately, without publishing duplicate data.
        if (audio_ && audio_used_.size() + 1 != audio_->entries().size()) {
            throw std::runtime_error("Audio package contains unreferenced data");
        }
    }

private:
    std::filesystem::path root_;
    std::optional<detail::AssetPackage> images_, audio_;
    std::set<std::string> images_used_, audio_used_;
};

Bytes read_asset_file(std::string name, std::uint64_t length, bool audio, AssetInput& input, const Context& context) {
    auto bytes = input.read(name, audio, {context.id, name});

    if (bytes.size() != length) {
        context.fail("runtime byte count mismatch");
    }

    return bytes;
}

std::vector<RgbaPixel> decode_png(const Bytes& bytes, std::uint32_t width, std::uint32_t height, const Context& context) {
    // Check IHDR before decoding, so a tiny hostile PNG cannot force an unbounded allocation.
    constexpr std::array<std::uint8_t, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};

    if (bytes.size() < 33 || !std::equal(signature.begin(), signature.end(), bytes.begin()) || !std::equal(bytes.begin() + 12, bytes.begin() + 16, "IHDR")) {
        context.fail("invalid PNG header");
    }

    auto big32 = [&](std::size_t at) {
        return (std::uint32_t(bytes[at]) << 24) | (std::uint32_t(bytes[at + 1]) << 16) | (std::uint32_t(bytes[at + 2]) << 8) | std::uint32_t(bytes[at + 3]);
    };

    if (big32(8) != 13 || big32(16) != width || big32(20) != height) {
        context.fail("PNG dimensions differ from manifest");
    }

    using Io = std::unique_ptr<SDL_IOStream, decltype(&SDL_CloseIO)>;
    using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
    Io io(SDL_IOFromConstMem(bytes.data(), bytes.size()), SDL_CloseIO);

    if (!io) {
        context.fail("cannot allocate PNG input: " + std::string(SDL_GetError()));
    }

    Surface source(SDL_LoadPNG_IO(io.get(), false), SDL_DestroySurface);

    if (!source || source->w != static_cast<int>(width) || source->h != static_cast<int>(height)) {
        context.fail("cannot decode PNG: " + std::string(SDL_GetError()));
    }

    Surface surface(SDL_ConvertSurface(source.get(), SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);

    if (!surface || !SDL_LockSurface(surface.get())) {
        context.fail("cannot convert or lock PNG pixels: " + std::string(SDL_GetError()));
    }

    std::vector<RgbaPixel> result(static_cast<std::size_t>(width) * height);

    for (std::size_t y = 0; y < height; ++y) {
        const auto* row = static_cast<const std::uint8_t*>(surface->pixels) + y * static_cast<std::size_t>(surface->pitch);
        std::memcpy(result.data() + y * width, row, static_cast<std::size_t>(width) * sizeof(RgbaPixel));
    }

    SDL_UnlockSurface(surface.get());

    return result;
}

struct ImageData {
    std::uint32_t width{}, height{};
    ImageKind kind{};
    std::vector<RgbaPixel> rgba;
    Bytes indices;
    std::vector<PaletteEntry> palette;
    std::optional<SpriteGrid> grid;
};

struct AudioData {
    AudioView metadata;
    Bytes encoded;
};

ImageData load_image(const ImageRecord& record, AssetInput& input, const Context& context) {
    const auto& id = context.id;
    ImageData data;
    data.width = record.width;
    data.height = record.height;
    data.kind = record.kind;
    auto png = read_asset_file("images/" + id + ".png", record.png_bytes, false, input, context);
    data.rgba = decode_png(png, data.width, data.height, context);

    if (data.kind == ImageKind::indexed) {
        data.indices = read_asset_file("indexed/" + id + ".indices", record.indices_bytes, false, input, context);
        const auto palette = read_asset_file("indexed/" + id + ".palette", record.palette_bytes, false, input, context);

        if (data.indices.size() != data.rgba.size() || palette.empty() || palette.size() % 4 != 0 || palette.size() > 1024) {
            context.fail("invalid index/palette lengths");
        }

        data.palette.resize(palette.size() / 4);
        std::memcpy(data.palette.data(), palette.data(), palette.size());

        for (std::size_t i = 0; i < data.indices.size(); ++i) {
            const auto index = data.indices[i];

            if (index >= data.palette.size()) {
                context.fail("index outside palette");
            }

            const auto& entry = data.palette[index];

            if (data.rgba[i] != RgbaPixel{entry[0], entry[1], entry[2], 255}) {
                context.fail("PNG and index/palette pixels disagree");
            }
        }
    } else {
        if (std::ranges::any_of(data.rgba, [](const auto& pixel) {
                return pixel.a != 255;
            })) {
            context.fail("base image contains unreviewed transparency");
        }
    }

    data.grid = record.grid;

    return data;
}

AudioData load_audio(const AudioRecord& record, AssetInput& input, const Context& context) {
    AudioData data;
    data.encoded = read_asset_file("audio/" + context.id + ".mp3", record.bytes, true, input, context);
    data.metadata = record.metadata;

    return data;
}
} // namespace

struct AssetStore::Storage {
    std::map<std::string, ImageData> images;
    std::map<std::string, AudioData> audio;
    std::vector<std::string> image_ids, audio_ids;
};

AssetError::AssetError(std::string id, std::string path, std::string cause)
    : std::runtime_error("Asset '" + id + "' at '" + path + "': " + cause), id_(std::move(id)), path_(std::move(path)), cause_(std::move(cause)) {}

AssetStore::AssetStore(std::unique_ptr<Storage> storage) : storage_(std::move(storage)) {}

AssetStore::~AssetStore() = default;
AssetStore::AssetStore(AssetStore&&) noexcept = default;
AssetStore& AssetStore::operator=(AssetStore&&) noexcept = default;

AssetStore AssetStore::load_directory(const std::filesystem::path& directory, const LoadObserver& observer) {
    return load_impl(directory, false, observer);
}

AssetStore AssetStore::load_packages(const std::filesystem::path& directory, const LoadObserver& observer) {
    return load_impl(directory, true, observer);
}

AssetStore AssetStore::load_impl(const std::filesystem::path& directory, bool packaged, const LoadObserver& observer) {
    const auto manifest_path = (directory / "manifest.txt").u8string();
    Context context{"manifest", std::string(manifest_path.begin(), manifest_path.end())};
    try {
        AssetInput input(directory, packaged);
        const auto manifest_bytes = input.read("manifest.txt", false, context);

        if (manifest_bytes.size() > 2'000'000) {
            context.fail("oversized manifest");
        }

        const auto manifest = parse_manifest(manifest_bytes, context);
        auto storage = std::make_unique<Storage>();
        const auto& images = manifest.images;
        const auto& audio = manifest.audio;

        for (const auto& record : images) {
            storage->image_ids.push_back(record.id);
        }

        for (const auto& record : audio) {
            storage->audio_ids.push_back(record.id);
        }

        // Validate the original manifest order before choosing a different decode order.
        std::vector<const ImageRecord*> image_records;
        image_records.reserve(images.size());
        std::set<std::string_view> priority_ids;

        for (const auto id : observer.image_order) {
            context = {std::string(id), "image_order"};
            const auto found = std::lower_bound(storage->image_ids.begin(), storage->image_ids.end(), id);

            if (found == storage->image_ids.end() || *found != id) {
                context.fail("image priority ID is missing from manifest");
            }

            if (!priority_ids.insert(id).second) {
                context.fail("duplicate image priority ID");
            }

            const auto index = static_cast<std::size_t>(found - storage->image_ids.begin());
            image_records.push_back(&images.at(index));
        }

        for (std::size_t index = 0; index < storage->image_ids.size(); ++index) {
            if (!priority_ids.contains(storage->image_ids[index])) {
                image_records.push_back(&images.at(index));
            }
        }

        for (const auto* image_record : image_records) {
            const auto& record = *image_record;
            auto id = record.id;
            context = {id, "manifest.txt"};
            auto data = load_image(record, input, context);
            const auto stored = storage->images.emplace(std::move(id), std::move(data)).first;

            if (observer.image_loaded) {
                const auto& image = stored->second;
                try {
                    observer.image_loaded(stored->first, {image.width, image.height, image.width, image.kind, image.rgba, image.indices, image.palette});
                } catch (const std::exception& error) {
                    context.fail("image_loaded observer failed: " + std::string(error.what()));
                } catch (...) {
                    context.fail("image_loaded observer failed with non-standard exception");
                }
            }
        }

        for (const auto& record : audio) {
            auto id = record.id;
            context = {id, "manifest.txt"};
            auto data = load_audio(record, input, context);
            const auto stored = storage->audio.emplace(std::move(id), std::move(data)).first;

            if (observer.audio_loaded) {
                try {
                    observer.audio_loaded(stored->first);
                } catch (const std::exception& error) {
                    context.fail("audio_loaded observer failed: " + std::string(error.what()));
                } catch (...) {
                    context.fail("audio_loaded observer failed with non-standard exception");
                }
            }
        }

        input.complete();

        return AssetStore(std::move(storage));
    } catch (const AssetError&) {
        throw;
    } catch (const std::exception& error) {
        context.fail(error.what());
    }
}

const AssetStore::Storage& AssetStore::storage() const {
    if (!storage_) {
        throw AssetError("store", "", "moved-from asset store");
    }

    return *storage_;
}

ImageView AssetStore::image(std::string_view id) const {
    const auto& images = storage().images;
    const auto found = images.find(std::string(id));

    if (found == images.end()) {
        throw AssetError(std::string(id), "", "unknown image ID");
    }

    const auto& data = found->second;

    return {data.width, data.height, data.width, data.kind, data.rgba, data.indices, data.palette};
}

AudioView AssetStore::audio(std::string_view id) const {
    const auto& records = storage().audio;
    const auto found = records.find(std::string(id));

    if (found == records.end()) {
        throw AssetError(std::string(id), "", "unknown audio ID");
    }

    auto view = found->second.metadata;
    view.encoded = found->second.encoded;

    return view;
}

std::optional<SpriteGrid> AssetStore::sprite_grid(std::string_view id) const {
    const auto& images = storage().images;
    const auto found = images.find(std::string(id));

    if (found == images.end()) {
        throw AssetError(std::string(id), "", "unknown image ID");
    }

    return found->second.grid;
}

std::span<const std::string> AssetStore::image_ids() const {
    return storage().image_ids;
}

std::span<const std::string> AssetStore::audio_ids() const {
    return storage().audio_ids;
}

} // namespace moorhuhn2::assets
