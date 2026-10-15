#pragma once

#include "assets/asset_package.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace asset_test {
using Bytes = std::vector<std::uint8_t>;
namespace detail = moorhuhn2::assets::detail;

inline Bytes bytes(std::string_view value) {
    return {value.begin(), value.end()};
}

inline void write(const std::filesystem::path& path, std::span<const std::uint8_t> data) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

    if (!file) {
        throw std::runtime_error("Cannot write fixture");
    }
}

inline void number(Bytes& data, std::size_t at, std::uint64_t value, std::size_t width) {
    for (std::size_t i = 0; i < width; ++i) {
        data.at(at + i) = static_cast<std::uint8_t>(value >> (i * 8));
    }
}

inline Bytes package(const std::vector<std::pair<std::string, Bytes>>& files) {
    std::size_t index_size{};
    for (const auto& [name, unused] : files) {
        static_cast<void>(unused);
        index_size += 22 + name.size();
    }
    auto data = bytes("MHASSETS");
    data.resize(16 + index_size);
    number(data, 8, files.size(), 4);
    number(data, 12, index_size, 4);
    std::size_t at = 16, offset = data.size();
    for (const auto& [name, payload] : files) {
        number(data, at, name.size(), 2);
        number(data, at + 2, offset, 8);
        number(data, at + 10, payload.size(), 8);
        number(data, at + 18, detail::crc32(payload), 4);
        std::copy(name.begin(), name.end(), data.begin() + static_cast<std::ptrdiff_t>(at + 22));
        data.insert(data.end(), payload.begin(), payload.end());
        at += 22 + name.size();
        offset += payload.size();
    }
    return data;
}

class Fixtures final {
public:
    explicit Fixtures(const std::filesystem::path& base) {
        path = base / ("case-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

        if (!std::filesystem::create_directories(path)) {
            throw std::runtime_error("Cannot create private fixture directory");
        }
    }

    ~Fixtures() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

    Fixtures(const Fixtures&) = delete;
    Fixtures& operator=(const Fixtures&) = delete;
    std::filesystem::path path;
};

} // namespace asset_test
