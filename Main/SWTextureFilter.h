#pragma once

#include <cstdint>

namespace S2TerrainFilter {

// Blend packed BGRA channels together, including the alpha used by terrain spots.
inline std::uint32_t BilinearColor(std::uint32_t p00, std::uint32_t p10,
                                  std::uint32_t p01, std::uint32_t p11,
                                  unsigned u, unsigned v) {
  const unsigned weights[] = {(256 - u) * (256 - v), u * (256 - v),
                              (256 - u) * v, u * v};
  std::uint32_t result = 0;
  for (unsigned shift = 0; shift < 32; shift += 8) {
    const unsigned channel = (((p00 >> shift) & 255) * weights[0] +
                              ((p10 >> shift) & 255) * weights[1] +
                              ((p01 >> shift) & 255) * weights[2] +
                              ((p11 >> shift) & 255) * weights[3] + 32768) >> 16;
    result |= channel << shift;
  }
  return result;
}

inline float BilinearSlope(float p00, float p10, float p01, float p11,
                           unsigned u, unsigned v) {
  const float x = u / 256.0f, y = v / 256.0f;
  const float top = p00 + (p10 - p00) * x;
  const float bottom = p01 + (p11 - p01) * x;
  return top + (bottom - top) * y;
}

} // namespace S2TerrainFilter
