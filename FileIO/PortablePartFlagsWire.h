#ifndef S2_FILEIO_PORTABLE_PART_FLAGS_WIRE_H
#define S2_FILEIO_PORTABLE_PART_FLAGS_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct PartFlagsFields {
  std::int32_t blocks[8];
};

inline bool DecodePartFlags(const std::uint8_t* source, std::size_t length,
                            PartFlagsFields* value) {
  if (!source || !value || length != 32) return false;
  PartFlagsFields result{};
  for (std::size_t i = 0; i < 8; ++i) {
    if (!DecodeStructureScalar(source + 4 * i, 4, &result.blocks[i]))
      return false;
  }
  *value = result;
  return true;
}

inline bool EncodePartFlags(const PartFlagsFields& value,
                            std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 32) return false;
  for (std::size_t i = 0; i < 8; ++i) {
    if (!EncodeStructureScalar(value.blocks[i], destination + 4 * i, 4))
      return false;
  }
  return true;
}

} // namespace S2FileIO

#endif
