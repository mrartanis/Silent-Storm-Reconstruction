#ifndef S2_FILEIO_PORTABLE_OBJECT_PLACE_H
#define S2_FILEIO_PORTABLE_OBJECT_PLACE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct ObjectPlaceFields {
  float position[3];
  float scale[3];
  float angle;
  std::int32_t floor;
};

// Historical 32-byte record: seven little-endian floats followed by int32.
inline bool DecodeObjectPlace(const std::uint8_t* source, std::size_t length,
                              ObjectPlaceFields* value) {
  if (!source || !value || length != 32) return false;
  ObjectPlaceFields result{};
  float floats[7] = {};
  if (!DecodeStructureFloatFields(source, 28, floats, 7) ||
      !DecodeStructureScalar(source + 28, 4, &result.floor)) return false;
  for (unsigned i = 0; i < 3; ++i) {
    result.position[i] = floats[i];
    result.scale[i] = floats[3 + i];
  }
  result.angle = floats[6];
  *value = result;
  return true;
}

inline bool EncodeObjectPlace(const ObjectPlaceFields& value,
                              std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 32) return false;
  const float floats[7] = {value.position[0], value.position[1], value.position[2],
                           value.scale[0], value.scale[1], value.scale[2], value.angle};
  return EncodeStructureFloatFields(floats, 7, destination, 28) &&
      EncodeStructureScalar(value.floor, destination + 28, 4);
}

} // namespace S2FileIO

#endif
