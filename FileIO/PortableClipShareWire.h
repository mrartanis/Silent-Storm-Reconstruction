#ifndef S2_FILEIO_PORTABLE_CLIP_SHARE_WIRE_H
#define S2_FILEIO_PORTABLE_CLIP_SHARE_WIRE_H

#include "PortableResourceKeyWire.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

// The building clipper cache key contains an eight-byte resource part key,
// followed by three 32-bit clip parameters. No host pointer is serialized.
struct ClipShareFields {
  ResourceKeyFields source;
  std::uint32_t parts;
  std::int32_t subBlockId;
  std::int32_t clip;
};

inline bool DecodeClipShare(const std::uint8_t* source, std::size_t length,
                            ClipShareFields* value) {
  if (!source || !value || length != 20) return false;
  ClipShareFields result{};
  if (!DecodeResourceKey(source, 8, &result.source) ||
      !DecodeStructureScalar(source + 8, 4, &result.parts) ||
      !DecodeStructureScalar(source + 12, 4, &result.subBlockId) ||
      !DecodeStructureScalar(source + 16, 4, &result.clip)) return false;
  *value = result;
  return true;
}

inline bool EncodeClipShare(const ClipShareFields& value,
                            std::uint8_t* destination, std::size_t length) {
  return destination && length == 20 &&
      EncodeResourceKey(value.source, destination, 8) &&
      EncodeStructureScalar(value.parts, destination + 8, 4) &&
      EncodeStructureScalar(value.subBlockId, destination + 12, 4) &&
      EncodeStructureScalar(value.clip, destination + 16, 4);
}

} // namespace S2FileIO

#endif
