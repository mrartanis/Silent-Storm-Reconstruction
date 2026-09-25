#pragma once

#include <chrono>
#include <cstdint>
#include <cstring>

namespace S2Random {

// The old GetTickCount seed was a 32-bit millisecond counter. Keep that
// width and wrap behavior without depending on Windows or converting through
// a host-sized integer. The epoch is intentionally unspecified, as before.
inline std::int32_t SeedFromBits(std::uint32_t bits) {
  std::int32_t seed = 0;
  static_assert(sizeof(seed) == sizeof(bits), "game seed must be 32 bits");
  std::memcpy(&seed, &bits, sizeof(seed));
  return seed;
}

inline std::int32_t ClockSeedFromMilliseconds(std::uint64_t milliseconds) {
  return SeedFromBits(static_cast<std::uint32_t>(milliseconds));
}

inline std::int32_t ClockSeed32() {
  const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
  const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  return ClockSeedFromMilliseconds(static_cast<std::uint64_t>(milliseconds));
}

} // namespace S2Random
