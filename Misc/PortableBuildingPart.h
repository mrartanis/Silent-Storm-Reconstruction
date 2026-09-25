#pragma once

#include <cstddef>
#include <cstdint>

namespace S2Building {

inline std::uint32_t MakePart(int floor, int x, int y) {
  return (static_cast<std::uint32_t>(floor) & UINT32_C(0x3f)) |
         ((static_cast<std::uint32_t>(x) & UINT32_C(0x1fff)) << 6) |
         ((static_cast<std::uint32_t>(y) & UINT32_C(0x1fff)) << 19);
}

inline int SignedField(std::uint32_t bits, unsigned shift, unsigned width) {
  const std::uint32_t mask = (UINT32_C(1) << width) - 1;
  const int value = static_cast<int>((bits >> shift) & mask);
  return value >= (1 << (width - 1)) ? value - (1 << width) : value;
}

inline int PartFloor(std::uint32_t bits) { return SignedField(bits, 0, 6); }
inline int PartX(std::uint32_t bits) { return SignedField(bits, 6, 13); }
inline int PartY(std::uint32_t bits) { return SignedField(bits, 19, 13); }

inline bool DecodePart(const std::uint8_t* source, std::size_t length,
                       std::uint32_t* bits) {
  if (!source || !bits || length != 4) return false;
  *bits = std::uint32_t(source[0]) | (std::uint32_t(source[1]) << 8) |
          (std::uint32_t(source[2]) << 16) | (std::uint32_t(source[3]) << 24);
  return true;
}

inline bool EncodePart(std::uint32_t bits, std::uint8_t* destination,
                       std::size_t length) {
  if (!destination || length != 4) return false;
  for (unsigned i = 0; i < 4; ++i)
    destination[i] = static_cast<std::uint8_t>(bits >> (8 * i));
  return true;
}

} // namespace S2Building
