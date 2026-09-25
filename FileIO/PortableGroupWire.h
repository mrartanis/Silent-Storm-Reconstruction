#ifndef S2_FILEIO_PORTABLE_GROUP_WIRE_H
#define S2_FILEIO_PORTABLE_GROUP_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct GroupInfoFields {
  std::uint16_t lightGroup, objectGroup;
};

inline bool DecodeGroupInfo(const std::uint8_t* source, std::size_t length,
                            GroupInfoFields* value) {
  if (!source || !value || length != 4) return false;
  GroupInfoFields result{};
  if (!DecodeStructureScalar(source, 2, &result.lightGroup) ||
      !DecodeStructureScalar(source + 2, 2, &result.objectGroup)) return false;
  *value = result;
  return true;
}

inline bool EncodeGroupInfo(const GroupInfoFields& value,
                            std::uint8_t* destination, std::size_t length) {
  return destination && length == 4 &&
      EncodeStructureScalar(value.lightGroup, destination, 2) &&
      EncodeStructureScalar(value.objectGroup, destination + 2, 2);
}

struct GroupSelectFields {
  std::uint16_t maskAny, maskEvery;
};

inline bool DecodeGroupSelect(const std::uint8_t* source, std::size_t length,
                              GroupSelectFields* value) {
  if (!source || !value || length != 4) return false;
  GroupSelectFields result{};
  if (!DecodeStructureScalar(source, 2, &result.maskAny) ||
      !DecodeStructureScalar(source + 2, 2, &result.maskEvery)) return false;
  *value = result;
  return true;
}

inline bool EncodeGroupSelect(const GroupSelectFields& value,
                              std::uint8_t* destination, std::size_t length) {
  return destination && length == 4 &&
      EncodeStructureScalar(value.maskAny, destination, 2) &&
      EncodeStructureScalar(value.maskEvery, destination + 2, 2);
}

} // namespace S2FileIO

#endif
