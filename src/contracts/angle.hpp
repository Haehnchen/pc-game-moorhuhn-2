#pragma once

namespace moorhuhn2::contracts {

// The original scales radians by this binary32 constant (EXE 0x412228), not by an exact 180/pi.
inline constexpr float degrees_per_radian = 57.295776F;
static_assert(static_cast<long double>(degrees_per_radian) == 57.2957763671875L);

} // namespace moorhuhn2::contracts
