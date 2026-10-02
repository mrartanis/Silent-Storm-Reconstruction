#pragma once
#include <cstdint>

namespace S2Campaign {
constexpr std::uint32_t kTravelStepsPerSecond = 60;
// Production chapter-map stepping shared with the frame-partition regression.
inline std::uint32_t TravelSteps(std::uint32_t now, std::uint32_t& last) {
  if (now < last) { last = now; return 0; }
  // Count crossings of the 60 Hz clock; preserving phase avoids rounding a
  // fractional step up on every Draw or dropping it at low frame rates.
  const std::uint32_t steps = static_cast<std::uint32_t>(
    std::uint64_t(now) * kTravelStepsPerSecond / 1000 -
    std::uint64_t(last) * kTravelStepsPerSecond / 1000);
  last = now;
  return steps;
}
inline bool AdvanceTravelStep(float& x, float& y, float targetX, float targetY,
                              float stepX, float stepY) {
  const float dx = targetX - x, dy = targetY - y;
  if (dx * dx + dy * dy <= 1) return false;
  x += stepX;
  y += stepY;
  return true;
}
}
