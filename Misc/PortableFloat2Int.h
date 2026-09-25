#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace S2Math {

// FISTP and CVTSS2SI round using the active floating-point mode and return
// the integer-indefinite value (INT32_MIN) for NaN or an out-of-range result.
// This implementation is used when the target has no x86 conversion opcode.
inline std::int32_t Float2IntWithCurrentRounding(float value) {
  if (!std::isfinite(value)) return (std::numeric_limits<std::int32_t>::min)();
  const double rounded = std::nearbyint(static_cast<double>(value));
  if (rounded < -2147483648.0 || rounded >= 2147483648.0)
    return (std::numeric_limits<std::int32_t>::min)();
  return static_cast<std::int32_t>(rounded);
}

} // namespace S2Math
