#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace moorhuhn2::assets::detail {

inline constexpr std::size_t package_max_entries = 256;
inline constexpr std::size_t package_max_bytes = 100'000'000;
[[nodiscard]] bool safe_path(std::string_view name);
[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> bytes);
[[nodiscard]] std::vector<std::uint8_t> read_file(const std::filesystem::path& path);

// MHASSETS: 8-byte magic, LE32 count/index size, then sorted entries.
// Each entry: LE16 name size, LE64 offset/length, LE32 CRC32, ASCII name.
// Payloads are contiguous, with no padding or recompression.
class AssetPackage final {
public:
    explicit AssetPackage(const std::filesystem::path& path);
    [[nodiscard]] std::span<const std::uint8_t> read(std::string_view name) const;

    [[nodiscard]] const std::map<std::string, std::pair<std::size_t, std::size_t>>& entries() const noexcept {
        return entries_;
    }

private:
    std::vector<std::uint8_t> bytes_;
    std::map<std::string, std::pair<std::size_t, std::size_t>> entries_;
};

} // namespace moorhuhn2::assets::detail
