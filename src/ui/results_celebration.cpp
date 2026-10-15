#include "ui/results_celebration.hpp"

#include "contracts/time.hpp"

#include <limits>
#include <stdexcept>

namespace moorhuhn2::ui {
namespace {

void initialize_rise(ResultsCelebrationRecord& record, ResultsRandom& random, bool recycle) {
    // Entry consumes X then Y; recycling consumes Y then X.
    if (recycle) {
        record.y = static_cast<float>(480 + random.next() % 128);
        record.x = static_cast<float>(random.next() % 600);
    } else {
        record.x = static_cast<float>(random.next() % 600);
        record.y = static_cast<float>(480 + random.next() % 128);
    }

    record.frame = static_cast<int>(random.next() % 4);
    record.vertical_speed = static_cast<float>(30 + random.next() % 40);
    record.horizontal_speed = static_cast<float>(2 + random.next() % 5);
}

void initialize_drop(ResultsCelebrationRecord& record, ResultsRandom& random, bool recycle) {
    if (recycle) {
        record.y = static_cast<float>(-64 - static_cast<int>(random.next() % 128));
        record.x = static_cast<float>(random.next() % 600);
    } else {
        record.x = static_cast<float>(random.next() % 600);
        record.y = static_cast<float>(-64 - static_cast<int>(random.next() % 128));
        record.frame = 5;
    }

    record.vertical_speed = static_cast<float>(80 + random.next() % 50);
    record.variant = static_cast<int>(random.next() % 2);
}

float store_coordinate(long double coordinate) {
    constexpr auto maximum = static_cast<long double>(std::numeric_limits<float>::max());
    if (coordinate < -maximum || coordinate > maximum) {
        throw std::invalid_argument("Results movement must fit a finite binary32 coordinate");
    }

    return static_cast<float>(coordinate);
}
} // namespace

ResultsCelebration::ResultsCelebration(int actual_score, bool first_place, ResultsRandom& random) {
    if (first_place) {
        kind_ = ResultsCelebrationKind::first_place;
        for (auto& record : records_) {
            initialize_rise(record, random, false);
        }
    }

    if (actual_score >= 1500) {
        kind_ = ResultsCelebrationKind::high_score;
        for (auto& record : records_) {
            initialize_drop(record, random, false);
        }
    }
}

void ResultsCelebration::update(float delta, ResultsRandom& random) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Results delta must be finite and nonnegative");
    }
    // Commit only a valid update, including random draws from recycled records.
    auto next_records = records_;
    auto next_random = random;
    for (auto& record : next_records) {
        if (kind_ == ResultsCelebrationKind::first_place) {
            record.y = store_coordinate(static_cast<long double>(record.y) - static_cast<long double>(delta) * record.vertical_speed);
            record.x = store_coordinate(static_cast<long double>(record.x) - static_cast<long double>(delta) * record.horizontal_speed);
            // FSTP then FLD: the rise boundary uses the stored binary32 Y.
            if (record.y < -32) {
                initialize_rise(record, next_random, true);
            }
        } else if (kind_ == ResultsCelebrationKind::high_score) {
            const auto y = static_cast<long double>(record.y) + static_cast<long double>(delta) * record.vertical_speed;
            record.y = store_coordinate(y);
            // FST retains the x87 sum, unlike the House88 stored-Y boundary.
            if (y > 480) {
                initialize_drop(record, next_random, true);
            }
        }
    }

    records_ = next_records;
    random = next_random;
}

std::vector<contracts::SpriteDraw> ResultsCelebration::draws() const {
    std::vector<contracts::SpriteDraw> result;
    result.reserve(records().size());
    for (const auto& record : records()) {
        const auto destination = contracts::ScreenPoint{static_cast<int>(record.x), static_cast<int>(record.y)};
        if (kind_ == ResultsCelebrationKind::first_place) {
            result.push_back({"gfx/ballon", {record.frame * 22, 0, 22, 42}, destination});
        } else {
            result.push_back({"gfx/scheuche", {0, 160, 32, 32}, destination});
        }
    }

    return result;
}

} // namespace moorhuhn2::ui
