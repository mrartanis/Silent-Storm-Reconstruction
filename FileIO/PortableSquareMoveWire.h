#ifndef S2_FILEIO_PORTABLE_SQUARE_MOVE_WIRE_H
#define S2_FILEIO_PORTABLE_SQUARE_MOVE_WIRE_H

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct SquareMoveFields {
  std::uint8_t parentX;
  std::uint8_t parentY;
  std::uint16_t cost;
};

inline bool DecodeSquareMove(const std::uint8_t* source, std::size_t length,
                             SquareMoveFields* value) {
  if (!source || !value || length != 4) return false;
  value->parentX = source[0];
  value->parentY = source[1];
  value->cost = std::uint16_t(source[2]) |
                (std::uint16_t(source[3]) << 8);
  return true;
}

inline bool EncodeSquareMove(const SquareMoveFields& value,
                             std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 4) return false;
  destination[0] = value.parentX;
  destination[1] = value.parentY;
  destination[2] = static_cast<std::uint8_t>(value.cost);
  destination[3] = static_cast<std::uint8_t>(value.cost >> 8);
  return true;
}

} // namespace S2FileIO

#endif
