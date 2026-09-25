#ifndef S2_FILEIO_PORTABLE_SELECTION_INFO_WIRE_H
#define S2_FILEIO_PORTABLE_SELECTION_INFO_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct SelectionInfoFields {
  float color[4];
  bool ignoreFloorMask;
};

inline bool DecodeSelectionInfo(const std::uint8_t* source,
                                std::size_t length,
                                SelectionInfoFields* value) {
  if (!source || !value || length != 20 ||
      !DecodeStructureFloatFields(source, 16, value->color, 4)) return false;
  value->ignoreFloorMask = source[16] != 0;
  return true;
}

inline bool EncodeSelectionInfo(const SelectionInfoFields& value,
                                std::uint8_t* destination,
                                std::size_t length) {
  if (!destination || length != 20 ||
      !EncodeStructureFloatFields(value.color, 4, destination, 16)) return false;
  destination[16] = value.ignoreFloorMask ? 1 : 0;
  destination[17] = destination[18] = destination[19] = 0;
  return true;
}

} // namespace S2FileIO

#endif
