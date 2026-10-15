#pragma once

#include "contracts/draw.hpp"
#include "ui/results_random.hpp"

#include <array>
#include <span>
#include <vector>

namespace moorhuhn2::ui {

enum class ResultsCelebrationKind {
    none,
    first_place,
    high_score
};

struct ResultsCelebrationRecord {
    float x{}, y{}, horizontal_speed{}, vertical_speed{};
    int frame{}, variant{};

    bool operator==(const ResultsCelebrationRecord&) const = default;
};

class ResultsCelebration {
public:
    // first_place means the current round inserted row one. The >=1500
    // initializer then overrides that decoration, retaining its consumed RNG.
    // House88 has its own initializer and must not also use this model.
    ResultsCelebration(int actual_score, bool first_place, ResultsRandom& random);
    void update(float delta, ResultsRandom& random);

    [[nodiscard]] ResultsCelebrationKind kind() const noexcept {
        return kind_;
    }

    [[nodiscard]] std::span<const ResultsCelebrationRecord> records() const noexcept {
        return kind_ == ResultsCelebrationKind::none ? std::span<const ResultsCelebrationRecord>{} : records_;
    }

    [[nodiscard]] std::vector<contracts::SpriteDraw> draws() const;

private:
    ResultsCelebrationKind kind_{ResultsCelebrationKind::none};
    std::array<ResultsCelebrationRecord, 16> records_{};
};

} // namespace moorhuhn2::ui
