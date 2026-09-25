#ifndef S2_MISC_PORTABLE_VECTOR_HASH_H
#define S2_MISC_PORTABLE_VECTOR_HASH_H

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace S2Math {

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
              "game vector hashes require IEEE-754 float32");
static_assert(sizeof(int) == 4 && sizeof(std::int32_t) == 4,
              "game vector hashes require 32-bit int");

inline std::uint32_t FloatBits(float value) {
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

inline int SignedHash(std::uint32_t bits) {
  std::int32_t signedBits = 0;
  std::memcpy(&signedBits, &bits, sizeof(bits));
  return signedBits;
}

inline std::uint32_t Vec3HashBits(float x, float y, float z) {
  return FloatBits(x) ^ FloatBits(y) ^ FloatBits(z);
}

inline std::uint32_t VisionHashTerm(float value, float scale) {
  const float scaled = value * scale;
  // Valid game ranges match the historical truncation. Malformed/NaN cache
  // values have no historical defined conversion to int; hash them as zero.
  if (!std::isfinite(scaled) || scaled < -2147483648.0f ||
      scaled >= 2147483648.0f) return 0;
  return static_cast<std::uint32_t>(static_cast<std::int32_t>(scaled));
}

inline std::uint32_t VisionQueryHashBits(float fromX, float fromY, float fromZ,
                                         float targetX, float targetY, float targetZ,
                                         float range, float cosFov) {
  return Vec3HashBits(fromX, fromY, fromZ) +
         Vec3HashBits(targetX, targetY, targetZ) +
         VisionHashTerm(range, 16.0f) + VisionHashTerm(cosFov, 1024.0f);
}

} // namespace S2Math

#endif
