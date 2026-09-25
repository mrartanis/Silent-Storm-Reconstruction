#ifndef S2_FILEIO_PORTABLE_LIGHT_KEY_WIRE_H
#define S2_FILEIO_PORTABLE_LIGHT_KEY_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

// Animated-light save keys: int16 frame followed by one or three float32
// values, packed to two-byte alignment in the original record.
template<std::size_t N>
inline bool DecodeLightKey(const std::uint8_t* source, std::size_t length,
                           std::int16_t* frame, float* values) {
  if (!source || !frame || !values || (N != 1 && N != 3) ||
      length != 2 + N * 4) return false;
  return DecodeStructureScalar(source, 2, frame) &&
         DecodeStructureFloatFields(source + 2, N * 4, values, N);
}

template<std::size_t N>
inline bool EncodeLightKey(std::int16_t frame, const float* values,
                           std::uint8_t* destination, std::size_t length) {
  if (!values || !destination || (N != 1 && N != 3) ||
      length != 2 + N * 4) return false;
  return EncodeStructureScalar(frame, destination, 2) &&
         EncodeStructureFloatFields(values, N, destination + 2, N * 4);
}

} // namespace S2FileIO

#endif
