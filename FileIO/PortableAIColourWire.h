#ifndef S2_FILEIO_PORTABLE_AI_COLOUR_WIRE_H
#define S2_FILEIO_PORTABLE_AI_COLOUR_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct NeighbourFields {
  std::uint16_t node, distance;
  std::int32_t layer;
};

inline bool DecodeNeighbour(const std::uint8_t* source, std::size_t length,
                            NeighbourFields* value) {
  if (!source || !value || length != 8) return false;
  NeighbourFields result{};
  if (!DecodeStructureScalar(source, 2, &result.node) ||
      !DecodeStructureScalar(source + 2, 2, &result.distance) ||
      !DecodeStructureScalar(source + 4, 4, &result.layer)) return false;
  *value = result;
  return true;
}

inline bool EncodeNeighbour(const NeighbourFields& value,
                            std::uint8_t* destination, std::size_t length) {
  return destination && length == 8 &&
      EncodeStructureScalar(value.node, destination, 2) &&
      EncodeStructureScalar(value.distance, destination + 2, 2) &&
      EncodeStructureScalar(value.layer, destination + 4, 4);
}

struct LocalColorFields { std::uint16_t averageX, averageY; };

inline bool DecodeLocalColor(const std::uint8_t* source, std::size_t length,
                             LocalColorFields* value) {
  if (!source || !value || length != 4) return false;
  LocalColorFields result{};
  if (!DecodeStructureScalar(source, 2, &result.averageX) ||
      !DecodeStructureScalar(source + 2, 2, &result.averageY)) return false;
  *value = result;
  return true;
}

inline bool EncodeLocalColor(const LocalColorFields& value,
                             std::uint8_t* destination, std::size_t length) {
  return destination && length == 4 &&
      EncodeStructureScalar(value.averageX, destination, 2) &&
      EncodeStructureScalar(value.averageY, destination + 2, 2);
}

} // namespace S2FileIO

#endif
