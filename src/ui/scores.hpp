#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace moorhuhn2::ui {
struct ScoreEntry {
    std::string name;
    std::uint32_t score{}, accuracy{};
    bool operator==(const ScoreEntry&) const = default;
};

class Scores {
public:
    Scores();

    [[nodiscard]] std::span<const ScoreEntry, 5> entries() const {
        return entries_;
    }

    [[nodiscard]] std::optional<std::size_t> insert(int score, int hits, int misses);
    bool type(std::size_t row, char character);
    bool backspace(std::size_t row);
    // Invalid input throws without changing this table.
    void decode(std::span<const std::uint8_t> bytes);
    [[nodiscard]] std::vector<std::uint8_t> encode() const;
    // Missing files keep the current table.
    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;

private:
    std::array<ScoreEntry, 5> entries_;
};
} // namespace moorhuhn2::ui
