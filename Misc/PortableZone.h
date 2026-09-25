#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace S2AI {

// Historical SZone wire layout: uint16 color, two padding bytes, int32 layer.
// The padding is ignored on read and canonicalized to zero on write.
inline bool DecodeZone(const std::uint8_t* source, std::size_t length,
                       std::uint16_t* color, std::int32_t* layer) {
  if (!source || !color || !layer || length != 8) return false;
  *color = std::uint16_t(source[0]) | (std::uint16_t(source[1]) << 8);
  const std::uint32_t bits = std::uint32_t(source[4]) |
      (std::uint32_t(source[5]) << 8) | (std::uint32_t(source[6]) << 16) |
      (std::uint32_t(source[7]) << 24);
  std::memcpy(layer, &bits, 4);
  return true;
}

inline bool EncodeZone(std::uint16_t color, std::int32_t layer,
                       std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 8) return false;
  destination[0] = static_cast<std::uint8_t>(color);
  destination[1] = static_cast<std::uint8_t>(color >> 8);
  destination[2] = destination[3] = 0;
  std::uint32_t bits = 0;
  std::memcpy(&bits, &layer, 4);
  for (unsigned i = 0; i < 4; ++i)
    destination[4 + i] = static_cast<std::uint8_t>(bits >> (8 * i));
  return true;
}

} // namespace S2AI
