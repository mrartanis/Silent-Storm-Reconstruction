#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace S2AI {

// Historical packed path-place word: tile x/y, integral, layer, final,
// moving, direction, pose. Keep bit positions explicit across host compilers.
constexpr std::uint32_t kXMask = UINT32_C(0x000000ff);
constexpr std::uint32_t kYMask = UINT32_C(0x0000ff00);
constexpr std::uint32_t kIntegralMask = UINT32_C(0x00010000);
constexpr std::uint32_t kLayerMask = UINT32_C(0x01fe0000);
constexpr std::uint32_t kFinalMask = UINT32_C(0x02000000);
constexpr std::uint32_t kMovingMask = UINT32_C(0x04000000);
constexpr std::uint32_t kDirectionMask = UINT32_C(0x38000000);
constexpr std::uint32_t kPoseMask = UINT32_C(0xc0000000);

inline std::uint32_t SetField(std::uint32_t bits, std::uint32_t mask,
                             unsigned shift, std::uint32_t value) {
  return (bits & ~mask) | ((value << shift) & mask);
}

inline std::uint32_t MakePathPlace(std::uint32_t x, std::uint32_t y,
                                   std::uint32_t layer, std::uint32_t direction,
                                   std::uint32_t pose, std::uint32_t moving) {
  std::uint32_t bits = 0;
  bits = SetField(bits, kXMask, 0, x);
  bits = SetField(bits, kYMask, 8, y);
  bits = SetField(bits, kIntegralMask, 16, 1);
  bits = SetField(bits, kLayerMask, 17, layer);
  bits = SetField(bits, kMovingMask, 26, moving);
  bits = SetField(bits, kDirectionMask, 27, direction);
  bits = SetField(bits, kPoseMask, 30, pose);
  return bits;
}

inline std::int32_t PathPlaceSigned(std::uint32_t bits) {
  std::int32_t value = 0;
  std::memcpy(&value, &bits, 4);
  return value;
}

inline bool DecodePathPlace(const std::uint8_t* source, std::size_t length,
                            std::uint32_t* bits) {
  if (!source || !bits || length != 4) return false;
  *bits = std::uint32_t(source[0]) | (std::uint32_t(source[1]) << 8) |
          (std::uint32_t(source[2]) << 16) | (std::uint32_t(source[3]) << 24);
  return true;
}

inline bool EncodePathPlace(std::uint32_t bits, std::uint8_t* destination,
                            std::size_t length) {
  if (!destination || length != 4) return false;
  for (unsigned i = 0; i < 4; ++i)
    destination[i] = static_cast<std::uint8_t>(bits >> (8 * i));
  return true;
}

} // namespace S2AI
