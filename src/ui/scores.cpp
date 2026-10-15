#include "ui/scores.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace moorhuhn2::ui {
namespace {
bool letter(char value) {
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}
} // namespace

Scores::Scores() : entries_{{{"Frank", 250, 70}, {"Ralf", 150, 60}, {"Ingo", 100, 50}, {"Stefanie", 50, 40}, {"Jendrik", 25, 30}}} {}

std::optional<std::size_t> Scores::insert(int score, int hits, int misses) {
    if (score < 0 || hits < 0 || misses < 0) {
        throw std::invalid_argument("Invalid round result");
    }

    // Like the original, ties keep their place.
    const auto value = static_cast<std::uint32_t>(score);
    if (value <= entries_.back().score) {
        return {};
    }

    const auto at = std::find_if(entries_.begin(), entries_.end(), [value](const auto& entry) {
        return value > entry.score;
    });
    const auto index = static_cast<std::size_t>(at - entries_.begin());
    std::move_backward(at, entries_.end() - 1, entries_.end());
    const auto attempts = static_cast<std::uint64_t>(hits) + static_cast<std::uint64_t>(misses);
    // Safe native policy for the original's undefined zero-shot conversion.
    const auto accuracy = attempts == 0 ? 0U : static_cast<std::uint32_t>(static_cast<std::uint64_t>(hits) * 100 / attempts);
    entries_[index] = {"", value, accuracy};
    return index;
}

bool Scores::type(std::size_t row, char character) {
    // Like the original, names accept up to eight ASCII letters.
    auto& name = entries_.at(row).name;
    if (!letter(character) || name.size() >= 8) {
        return false;
    }

    name.push_back(character);

    return true;
}

bool Scores::backspace(std::size_t row) {
    auto& name = entries_.at(row).name;
    if (name.empty()) {
        return false;
    }

    name.pop_back();

    return true;
}

void Scores::decode(std::span<const std::uint8_t> bytes) {
    std::size_t offset = 0;
    const auto read = [&]() {
        if (bytes.size() - offset < 4) {
            throw std::runtime_error("Truncated highscore record");
        }
        std::uint32_t value{};
        for (unsigned shift = 0; shift < 32; shift += 8) {
            value |= static_cast<std::uint32_t>(bytes[offset++]) << shift;
        }

        return value;
    };
    std::array<ScoreEntry, 5> decoded;
    std::uint8_t name_key = 0xec;
    for (auto& entry : decoded) {
        entry.score = read() ^ 0xaa5533ffU;
        entry.accuracy = read() ^ 0xff8888ffU;
        const auto length = read();
        if (length > 8 || length > bytes.size() - offset || entry.score > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || entry.accuracy > 100) {
            throw std::runtime_error("Invalid highscore record");
        }

        for (std::uint32_t i = 0; i < length; ++i) {
            const auto character = static_cast<char>(bytes[offset++] ^ name_key);
            if (!letter(character)) {
                throw std::runtime_error("Invalid highscore name");
            }

            entry.name.push_back(character);
        }
        ++name_key;
    }

    if (offset != bytes.size() || !std::is_sorted(decoded.begin(), decoded.end(), [](const auto& a, const auto& b) {
            return a.score > b.score;
        })) {
        throw std::runtime_error("Invalid highscore table");
    }

    entries_ = std::move(decoded);
}

std::vector<std::uint8_t> Scores::encode() const {
    std::vector<std::uint8_t> bytes;
    const auto write = [&](std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    };
    std::uint8_t name_key = 0xec;
    for (const auto& entry : entries_) {
        write(entry.score ^ 0xaa5533ffU);
        write(entry.accuracy ^ 0xff8888ffU);
        write(static_cast<std::uint32_t>(entry.name.size()));
        for (std::size_t i = 0; i < entry.name.size(); ++i) {
            bytes.push_back(static_cast<std::uint8_t>(entry.name[i]) ^ name_key);
        }
        ++name_key;
    }

    return bytes;
}

void Scores::load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return;
    }

    if (std::filesystem::file_size(path) > 100) {
        throw std::runtime_error("Highscore file exceeds five bounded records");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot read highscores");
    }

    const std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) {
        throw std::runtime_error("Highscore read failed");
    }

    decode(bytes);
}

void Scores::save(const std::filesystem::path& path) const {
    auto temporary = path;
    temporary += ".tmp";
    try {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        const auto bytes = encode();
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        output.close();
        if (!output) {
            throw std::runtime_error("Cannot write highscores");
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            throw std::filesystem::filesystem_error("Cannot replace highscores", temporary, path, std::error_code(GetLastError(), std::system_category()));
        }
#else
        std::filesystem::rename(temporary, path);
#endif
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}
} // namespace moorhuhn2::ui
