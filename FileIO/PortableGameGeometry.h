#ifndef S2_FILEIO_PORTABLE_GAME_GEOMETRY_H
#define S2_FILEIO_PORTABLE_GAME_GEOMETRY_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace S2FileIO {

template<class T> struct PointFields { T x, y; };
template<class T> struct RectFields { T x1, y1, x2, y2; };

template<class T>
inline bool DecodePoint(const std::uint8_t* source, std::size_t length,
                        PointFields<T>* value) {
  static_assert(std::is_same<T, std::int32_t>::value || std::is_same<T, float>::value,
                "only game int32 and float points use this wire format");
  if (!source || !value || length != 8) return false;
  PointFields<T> result{};
  if (!DecodeStructureScalar(source, 4, &result.x) ||
      !DecodeStructureScalar(source + 4, 4, &result.y)) return false;
  *value = result;
  return true;
}

template<class T>
inline bool EncodePoint(const PointFields<T>& value,
                        std::uint8_t* destination, std::size_t length) {
  static_assert(std::is_same<T, std::int32_t>::value || std::is_same<T, float>::value,
                "only game int32 and float points use this wire format");
  return destination && length == 8 &&
      EncodeStructureScalar(value.x, destination, 4) &&
      EncodeStructureScalar(value.y, destination + 4, 4);
}

template<class T>
inline bool DecodeRect(const std::uint8_t* source, std::size_t length,
                       RectFields<T>* value) {
  static_assert(std::is_same<T, std::int32_t>::value || std::is_same<T, float>::value,
                "only game int32 and float rects use this wire format");
  if (!source || !value || length != 16) return false;
  RectFields<T> result{};
  if (!DecodeStructureScalar(source, 4, &result.x1) ||
      !DecodeStructureScalar(source + 4, 4, &result.y1) ||
      !DecodeStructureScalar(source + 8, 4, &result.x2) ||
      !DecodeStructureScalar(source + 12, 4, &result.y2)) return false;
  *value = result;
  return true;
}

template<class T>
inline bool EncodeRect(const RectFields<T>& value,
                       std::uint8_t* destination, std::size_t length) {
  static_assert(std::is_same<T, std::int32_t>::value || std::is_same<T, float>::value,
                "only game int32 and float rects use this wire format");
  return destination && length == 16 &&
      EncodeStructureScalar(value.x1, destination, 4) &&
      EncodeStructureScalar(value.y1, destination + 4, 4) &&
      EncodeStructureScalar(value.x2, destination + 8, 4) &&
      EncodeStructureScalar(value.y2, destination + 12, 4);
}

struct TriangleFields { std::uint16_t first, second, third; };
struct SphereFields { float center[3]; float radius; };
struct MassSphereFields { float center[3]; float radius; float mass; };
struct BoundFields { float center[3]; float radius; float halfBox[3]; };
struct RayFields { float origin[3]; float direction[3]; };
struct BoundCalcerFields { float minimum[3]; float maximum[3]; };

inline bool DecodeRay(const std::uint8_t* source, std::size_t length,
                      RayFields* value) {
  if (!value) return false;
  float f[6] = {};
  if (!DecodeStructureFloatFields(source, length, f, 6)) return false;
  *value = RayFields{{f[0], f[1], f[2]}, {f[3], f[4], f[5]}};
  return true;
}

inline bool EncodeRay(const RayFields& value, std::uint8_t* destination,
                      std::size_t length) {
  const float f[6] = {value.origin[0], value.origin[1], value.origin[2],
                      value.direction[0], value.direction[1], value.direction[2]};
  return EncodeStructureFloatFields(f, 6, destination, length);
}

inline bool DecodeBoundCalcer(const std::uint8_t* source, std::size_t length,
                              BoundCalcerFields* value) {
  if (!value) return false;
  float f[6] = {};
  if (!DecodeStructureFloatFields(source, length, f, 6)) return false;
  *value = BoundCalcerFields{{f[0], f[1], f[2]}, {f[3], f[4], f[5]}};
  return true;
}

inline bool EncodeBoundCalcer(const BoundCalcerFields& value,
                              std::uint8_t* destination,
                              std::size_t length) {
  const float f[6] = {value.minimum[0], value.minimum[1], value.minimum[2],
                      value.maximum[0], value.maximum[1], value.maximum[2]};
  return EncodeStructureFloatFields(f, 6, destination, length);
}

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
