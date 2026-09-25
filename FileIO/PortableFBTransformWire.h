#ifndef S2_FILEIO_PORTABLE_FB_TRANSFORM_WIRE_H
#define S2_FILEIO_PORTABLE_FB_TRANSFORM_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct FBTransformFields {
  float forward[16];
  float backward[16];
};

inline bool DecodeFBTransform(const std::uint8_t* source, std::size_t length,
                              FBTransformFields* value) {
  if (!source || !value || length != 128) return false;
  FBTransformFields result{};
  if (!DecodeStructureFloatFields(source, 64, result.forward, 16) ||
      !DecodeStructureFloatFields(source + 64, 64, result.backward, 16))
    return false;
  *value = result;
  return true;
}

inline bool EncodeFBTransform(const FBTransformFields& value,
                              std::uint8_t* destination, std::size_t length) {
  return destination && length == 128 &&
      EncodeStructureFloatFields(value.forward, 16, destination, 64) &&
      EncodeStructureFloatFields(value.backward, 16, destination + 64, 64);
}

} // namespace S2FileIO

#endif
