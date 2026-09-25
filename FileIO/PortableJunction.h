#ifndef S2_FILEIO_PORTABLE_JUNCTION_H
#define S2_FILEIO_PORTABLE_JUNCTION_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct JunctionFields {
  float x;
  float y;
  float z;
  bool ground;
};

// Historical 16-byte AI junction: three LE floats, one bool, three padding
// bytes. Ignore padding on read and canonicalize it on write.
inline bool DecodeJunction(const std::uint8_t* source, std::size_t length,
                           JunctionFields* value) {
  if (!source || !value || length != 16) return false;
  float coords[3] = {};
  if (!DecodeStructureFloatFields(source, 12, coords, 3)) return false;
  *value = JunctionFields{coords[0], coords[1], coords[2], source[12] != 0};
  return true;
}

inline bool EncodeJunction(const JunctionFields& value,
                           std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 16) return false;
  const float coords[3] = {value.x, value.y, value.z};
  if (!EncodeStructureFloatFields(coords, 3, destination, 12)) return false;
  destination[12] = value.ground ? 1 : 0;
  destination[13] = destination[14] = destination[15] = 0;
  return true;
}

} // namespace S2FileIO

#endif
