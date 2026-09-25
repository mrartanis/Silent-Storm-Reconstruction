#ifndef S2_FILEIO_PORTABLE_CRITICAL_H
#define S2_FILEIO_PORTABLE_CRITICAL_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct CriticalFields {
  std::int32_t difficulty;
  std::int32_t duration;
  float value;
  std::int32_t critical;
  std::int32_t location;
};

// Historical record: int32, int32, float32, int32 enum, int32 enum.
inline bool DecodeCritical(const std::uint8_t* source, std::size_t length,
                           CriticalFields* value) {
  if (!source || !value || length != 20) return false;
  CriticalFields result{};
  if (!DecodeStructureScalar(source, 4, &result.difficulty) ||
      !DecodeStructureScalar(source + 4, 4, &result.duration) ||
      !DecodeStructureScalar(source + 8, 4, &result.value) ||
      !DecodeStructureScalar(source + 12, 4, &result.critical) ||
      !DecodeStructureScalar(source + 16, 4, &result.location)) return false;
  *value = result;
  return true;
}

inline bool EncodeCritical(const CriticalFields& value,
                           std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 20) return false;
  return EncodeStructureScalar(value.difficulty, destination, 4) &&
      EncodeStructureScalar(value.duration, destination + 4, 4) &&
      EncodeStructureScalar(value.value, destination + 8, 4) &&
      EncodeStructureScalar(value.critical, destination + 12, 4) &&
      EncodeStructureScalar(value.location, destination + 16, 4);
}

} // namespace S2FileIO

#endif
