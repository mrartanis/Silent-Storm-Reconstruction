#ifndef S2_FILEIO_PORTABLE_ANIMATION_WIRE_H
#define S2_FILEIO_PORTABLE_ANIMATION_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

// Animation keys are consecutive IEEE-754 float32 fields, optionally
// preceded by a signed parent-bone index. No host C++ layout is wire data.
inline bool DecodeAnimationFloats(const std::uint8_t* source,
                                  std::size_t length, float* values,
                                  std::size_t count) {
  return DecodeStructureFloatFields(source, length, values, count);
}

inline bool EncodeAnimationFloats(const float* values, std::size_t count,
                                  std::uint8_t* destination,
                                  std::size_t length) {
  return EncodeStructureFloatFields(values, count, destination, length);
}

inline bool DecodeAnimationParentFloats(const std::uint8_t* source,
                                        std::size_t length,
                                        std::int32_t* parent, float* values,
                                        std::size_t count) {
  if (!source || !parent || !values || count > 15 ||
      length != 4 + count * 4) return false;
  std::int32_t decodedParent = 0;
  if (!DecodeStructureScalar(source, 4, &decodedParent) ||
      !DecodeAnimationFloats(source + 4, count * 4, values, count))
    return false;
  *parent = decodedParent;
  return true;
}

inline bool EncodeAnimationParentFloats(std::int32_t parent,
                                        const float* values,
                                        std::size_t count,
                                        std::uint8_t* destination,
                                        std::size_t length) {
  if (!destination || !values || count > 15 ||
      length != 4 + count * 4) return false;
  return EncodeStructureScalar(parent, destination, 4) &&
         EncodeAnimationFloats(values, count, destination + 4, count * 4);
}

} // namespace S2FileIO

#endif
