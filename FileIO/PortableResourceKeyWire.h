#ifndef S2_FILEIO_PORTABLE_RESOURCE_KEY_WIRE_H
#define S2_FILEIO_PORTABLE_RESOURCE_KEY_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

// Both game resource keys are pairs of signed 32-bit integers. Their second
// fields have different meanings (part number or texture flags).
struct ResourceKeyFields {
  std::int32_t id, option;
};

inline bool DecodeResourceKey(const std::uint8_t* source, std::size_t length,
                              ResourceKeyFields* value) {
  if (!source || !value || length != 8) return false;
  ResourceKeyFields result{};
  if (!DecodeStructureScalar(source, 4, &result.id) ||
      !DecodeStructureScalar(source + 4, 4, &result.option)) return false;
  *value = result;
  return true;
}

inline bool EncodeResourceKey(const ResourceKeyFields& value,
                              std::uint8_t* destination, std::size_t length) {
  return destination && length == 8 &&
      EncodeStructureScalar(value.id, destination, 4) &&
      EncodeStructureScalar(value.option, destination + 4, 4);
}

} // namespace S2FileIO

#endif
