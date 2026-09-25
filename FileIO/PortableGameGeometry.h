#ifndef S2_FILEIO_PORTABLE_GAME_GEOMETRY_H
#define S2_FILEIO_PORTABLE_GAME_GEOMETRY_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct TriangleFields { std::uint16_t first, second, third; };
struct SphereFields { float center[3]; float radius; };
struct MassSphereFields { float center[3]; float radius; float mass; };
struct BoundFields { float center[3]; float radius; float halfBox[3]; };

inline bool DecodeTriangle(const std::uint8_t* source, std::size_t length,
                           TriangleFields* value) {
  if (!source || !value || length != 6) return false;
  TriangleFields result{};
  if (!DecodeStructureScalar(source, 2, &result.first) ||
      !DecodeStructureScalar(source + 2, 2, &result.second) ||
      !DecodeStructureScalar(source + 4, 2, &result.third)) return false;
  *value = result;
  return true;
}

inline bool EncodeTriangle(const TriangleFields& value,
                           std::uint8_t* destination, std::size_t length) {
  return destination && length == 6 &&
      EncodeStructureScalar(value.first, destination, 2) &&
      EncodeStructureScalar(value.second, destination + 2, 2) &&
      EncodeStructureScalar(value.third, destination + 4, 2);
}

inline bool DecodeSphere(const std::uint8_t* source, std::size_t length,
                         SphereFields* value) {
  if (!source || !value || length != 16) return false;
  float floats[4] = {};
  if (!DecodeStructureFloatFields(source, length, floats, 4)) return false;
  *value = SphereFields{{floats[0], floats[1], floats[2]}, floats[3]};
  return true;
}

inline bool EncodeSphere(const SphereFields& value,
                         std::uint8_t* destination, std::size_t length) {
  const float floats[4] = {value.center[0], value.center[1], value.center[2], value.radius};
  return EncodeStructureFloatFields(floats, 4, destination, length);
}

inline bool DecodeMassSphere(const std::uint8_t* source, std::size_t length,
                             MassSphereFields* value) {
  if (!source || !value || length != 20) return false;
  float floats[5] = {};
  if (!DecodeStructureFloatFields(source, length, floats, 5)) return false;
  *value = MassSphereFields{{floats[0], floats[1], floats[2]}, floats[3], floats[4]};
  return true;
}

inline bool EncodeMassSphere(const MassSphereFields& value,
                             std::uint8_t* destination, std::size_t length) {
  const float floats[5] = {value.center[0], value.center[1], value.center[2],
                           value.radius, value.mass};
  return EncodeStructureFloatFields(floats, 5, destination, length);
}

inline bool DecodeBound(const std::uint8_t* source, std::size_t length,
                        BoundFields* value) {
  if (!source || !value || length != 28) return false;
  float floats[7] = {};
  if (!DecodeStructureFloatFields(source, length, floats, 7)) return false;
  *value = BoundFields{{floats[0], floats[1], floats[2]}, floats[3],
                       {floats[4], floats[5], floats[6]}};
  return true;
}

inline bool EncodeBound(const BoundFields& value,
                        std::uint8_t* destination, std::size_t length) {
  const float floats[7] = {value.center[0], value.center[1], value.center[2],
                           value.radius, value.halfBox[0], value.halfBox[1], value.halfBox[2]};
  return EncodeStructureFloatFields(floats, 7, destination, length);
}

} // namespace S2FileIO

#endif
