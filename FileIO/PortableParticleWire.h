#pragma once

#include "PortableStructureChunks.h"

namespace S2FileIO {

struct StructureRestoreBoneFields {
  std::int32_t p1 = 0, p2 = 0, p3 = 0;
};

inline bool DecodeStructureRestoreBone(const std::uint8_t* source,
                                       std::size_t length,
                                       StructureRestoreBoneFields* value) {
  if (!source || !value || length != 12) return false;
  StructureRestoreBoneFields decoded;
  if (!DecodeStructureScalar(source, 4, &decoded.p1) ||
      !DecodeStructureScalar(source + 4, 4, &decoded.p2) ||
      !DecodeStructureScalar(source + 8, 4, &decoded.p3)) return false;
  *value = decoded;
  return true;
}

inline bool EncodeStructureRestoreBone(const StructureRestoreBoneFields& value,
                                       std::uint8_t* destination,
                                       std::size_t length) {
  return destination && length == 12 &&
         EncodeStructureScalar(value.p1, destination, 4) &&
         EncodeStructureScalar(value.p2, destination + 4, 4) &&
         EncodeStructureScalar(value.p3, destination + 8, 4);
}

struct StructureStickFields {
  std::int32_t p1 = 0, p2 = 0;
  float rest = 0, sumInvMasses = 0;
  std::int32_t max = 0;
};

inline bool DecodeStructureStick(const std::uint8_t* source,
                                 std::size_t length, StructureStickFields* value) {
  if (!source || !value || length != 20) return false;
  StructureStickFields decoded;
  if (!DecodeStructureScalar(source, 4, &decoded.p1) ||
      !DecodeStructureScalar(source + 4, 4, &decoded.p2) ||
      !DecodeStructureScalar(source + 8, 4, &decoded.rest) ||
      !DecodeStructureScalar(source + 12, 4, &decoded.sumInvMasses) ||
      !DecodeStructureScalar(source + 16, 4, &decoded.max)) return false;
  *value = decoded;
  return true;
}

inline bool EncodeStructureStick(const StructureStickFields& value,
                                 std::uint8_t* destination,
                                 std::size_t length) {
  return destination && length == 20 &&
         EncodeStructureScalar(value.p1, destination, 4) &&
         EncodeStructureScalar(value.p2, destination + 4, 4) &&
         EncodeStructureScalar(value.rest, destination + 8, 4) &&
         EncodeStructureScalar(value.sumInvMasses, destination + 12, 4) &&
         EncodeStructureScalar(value.max, destination + 16, 4);
}

} // namespace S2FileIO
