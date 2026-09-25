#ifndef S2_FILEIO_PORTABLE_VISION_QUERY_WIRE_H
#define S2_FILEIO_PORTABLE_VISION_QUERY_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct VisionQueryFields {
  float from[3];
  float target[3];
  float range;
  float cosFov;
};

inline bool DecodeVisionQuery(const std::uint8_t* source, std::size_t length,
                              VisionQueryFields* value) {
  if (!source || !value || length != 32) return false;
  VisionQueryFields result{};
  float* fields[] = {&result.from[0], &result.from[1], &result.from[2],
                     &result.target[0], &result.target[1], &result.target[2],
                     &result.range, &result.cosFov};
  for (std::size_t i = 0; i < 8; ++i)
    if (!DecodeStructureScalar(source + 4 * i, 4, fields[i])) return false;
  *value = result;
  return true;
}

inline bool EncodeVisionQuery(const VisionQueryFields& value,
                              std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 32) return false;
  const float* fields[] = {&value.from[0], &value.from[1], &value.from[2],
                           &value.target[0], &value.target[1], &value.target[2],
                           &value.range, &value.cosFov};
  for (std::size_t i = 0; i < 8; ++i)
    if (!EncodeStructureScalar(*fields[i], destination + 4 * i, 4)) return false;
  return true;
}

} // namespace S2FileIO

#endif
