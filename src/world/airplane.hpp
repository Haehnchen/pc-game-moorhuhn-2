#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace moorhuhn2::world {

using AircraftRandom = std::function<std::uint32_t()>;

struct Airplane {
    int type{-1};
    int depth{4};
    float world_x{};
    float world_y{};
    float screen_x{-1024};
    float screen_y{-1024};
    float wait{};
    float animation_time{};
    float smoke_time{};
    int frame{};
    int frame_count{20};
    float animation_rate{5};
    int points{-25};

    bool operator==(const Airplane&) const = default;
};

// These are original round globals, distinct from the reusable scene slot.
struct AirplaneState {
    bool active{};
    int hits{};
    bool penalty_applied{};
    bool engine_loop{};
    bool failure_loop{};

    bool operator==(const AirplaneState&) const = default;
};

enum class AirplaneEventKind : std::uint8_t {
    start_engine,
    pan_engine,
    stop_engine,
    start_failure,
    pan_failure,
    stop_failure,
    shot,
    penalty,
    crash,
    smoke
};

struct AirplaneEvent {
    AirplaneEventKind kind{};
    float pan{};
    // Penalty: score first, then one depth4 popup at this pose. Smoke: current
    // source type/pose, including type-1 on the removal update, before allocation.
    float world_x{};
    float world_y{};
    int points{};
    int source_type{};

    bool operator==(const AirplaneEvent&) const = default;
};

class AirplaneEvents {
public:
    void push_back(AirplaneEvent event) {
        events_.at(size_) = event;
        ++size_;
    }

    [[nodiscard]] const AirplaneEvent* begin() const {
        return events_.data();
    }

    [[nodiscard]] const AirplaneEvent* end() const {
        return begin() + size_;
    }

    [[nodiscard]] std::size_t size() const {
        return size_;
    }

private:
    // Two pans, two stops, one crash and one smoke emission can coincide.
    std::array<AirplaneEvent, 6> events_{};
    std::size_t size_{};
};

// Caller finds a free scene slot first. A closed active gate consumes no RNG.
// Success appends unpanned start_engine and consumes four ordered CRT draws.
[[nodiscard]] std::optional<Airplane> make_airplane(AirplaneState& state, const AircraftRandom& random, std::vector<AirplaneEvent>& events);
// Commands preserve the original pan -> exit -> fall/crash -> animation -> smoke
// order. Process smoke synchronously before updating the next scene slot; its
// shared 100-particle allocator consumes RNG only when allocation succeeds.
[[nodiscard]] AirplaneEvents update_airplane(Airplane& airplane, AirplaneState& state, float delta, float camera, int recoil, const AircraftRandom& random);
// Shot always precedes third-hit penalty/popup, engine stop and failure start.
// Every selected hit credits accuracy; ammunition kinds agree. Repeats remain
// eligible, reset wait to 0, and never repeat the penalty.
[[nodiscard]] std::vector<AirplaneEvent> hit_airplane(Airplane& airplane, AirplaneState& state);
[[nodiscard]] std::optional<contracts::SpriteDraw> airplane_draw(const Airplane& airplane);
[[nodiscard]] std::optional<contracts::SpriteDraw> airplane_collision(const Airplane& airplane);

// Original particles 4/5 share the 100-particle pool and particle2 art. Store this
// state in a free shared slot; never allocate a separate independent smoke pool.
struct AircraftSmoke {
    int type{-1};
    float world_x{};
    float world_y{};
    float screen_x{-1024};
    float screen_y{-1024};
    float velocity_x{};
    float velocity_y{};
    float age{};
    int frame{5};

    bool operator==(const AircraftSmoke&) const = default;
};

// Both creation routines consume exactly two CRT draws after particle allocation.
[[nodiscard]] AircraftSmoke make_airplane_smoke(int source_type, float world_x, float world_y, const AircraftRandom& random);
// The blimp supplies world position plus its retained hit offsets.
[[nodiscard]] AircraftSmoke make_blimp_smoke(float world_x, float world_y, const AircraftRandom& random);
void update_aircraft_smoke(AircraftSmoke& smoke, float delta, float camera, int recoil);
[[nodiscard]] std::optional<contracts::SpriteDraw> aircraft_smoke_draw(const AircraftSmoke& smoke);

} // namespace moorhuhn2::world
