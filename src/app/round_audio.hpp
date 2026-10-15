#pragma once

#include "audio/engine.hpp"
#include "game/round.hpp"

#include <optional>

namespace moorhuhn2::app {

std::optional<audio::Command> round_audio(const game::RoundEvent& event);

} // namespace moorhuhn2::app
