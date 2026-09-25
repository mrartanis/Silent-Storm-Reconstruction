#ifndef S2_FILEIO_PORTABLE_MAP_STOREY_WIRE_H
#define S2_FILEIO_PORTABLE_MAP_STOREY_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct MapStoreyFields {
  std::int32_t localFloor;
  std::int32_t globalFloor;
};

inline bool DecodeMapStorey(const std::uint8_t* source, std::size_t length,
                            MapStoreyFields* value) {
  if (!source || !value || length != 8) return false;
  return DecodeStructureScalar(source, 4, &value->localFloor) &&
         DecodeStructureScalar(source + 4, 4, &value->globalFloor);
}

inline bool EncodeMapStorey(const MapStoreyFields& value,
                            std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 8) return false;
  return EncodeStructureScalar(value.localFloor, destination, 4) &&
         EncodeStructureScalar(value.globalFloor, destination + 4, 4);
}

} // namespace S2FileIO

#endif
