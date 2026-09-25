#ifndef S2_FILEIO_PORTABLE_AI_TILE_WIRE_H
#define S2_FILEIO_PORTABLE_AI_TILE_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct AITileFields {
  std::uint8_t move[4];
  std::uint16_t height;
  std::int16_t floor;
  std::uint8_t locks, dynamicLocks, passable, displacement;
  std::uint8_t flags, flipper, reserved1, reserved2;
};

inline bool DecodeAITile(const std::uint8_t* source, std::size_t length,
                         AITileFields* value) {
  if (!source || !value || length != 16) return false;
  AITileFields result{};
  for (std::size_t i = 0; i < 4; ++i) result.move[i] = source[i];
  if (!DecodeStructureScalar(source + 4, 2, &result.height) ||
      !DecodeStructureScalar(source + 6, 2, &result.floor)) return false;
  result.locks = source[8]; result.dynamicLocks = source[9];
  result.passable = source[10]; result.displacement = source[11];
  result.flags = source[12]; result.flipper = source[13];
  result.reserved1 = source[14]; result.reserved2 = source[15];
  *value = result;
  return true;
}

inline bool EncodeAITile(const AITileFields& value,
                         std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 16) return false;
  for (std::size_t i = 0; i < 4; ++i) destination[i] = value.move[i];
  if (!EncodeStructureScalar(value.height, destination + 4, 2) ||
      !EncodeStructureScalar(value.floor, destination + 6, 2)) return false;
  destination[8] = value.locks; destination[9] = value.dynamicLocks;
  destination[10] = value.passable; destination[11] = value.displacement;
  destination[12] = value.flags; destination[13] = value.flipper;
  destination[14] = value.reserved1; destination[15] = value.reserved2;
  return true;
}

struct AILinkFields { std::uint32_t destination; std::int32_t height; };

inline bool DecodeAILink(const std::uint8_t* source, std::size_t length,
                         AILinkFields* value) {
  if (!source || !value || length != 8) return false;
  AILinkFields result{};
  if (!DecodeStructureScalar(source, 4, &result.destination) ||
      !DecodeStructureScalar(source + 4, 4, &result.height)) return false;
  *value = result;
  return true;
}

inline bool EncodeAILink(const AILinkFields& value,
                         std::uint8_t* destination, std::size_t length) {
  return destination && length == 8 &&
      EncodeStructureScalar(value.destination, destination, 4) &&
      EncodeStructureScalar(value.height, destination + 4, 4);
}

} // namespace S2FileIO

#endif
