#ifndef S2_FILEIO_GEOMETRY_WIRE_H
#define S2_FILEIO_GEOMETRY_WIRE_H

#include "PortableStructureChunks.h"
#include "..\Misc\Geom.h"

namespace S2FileIO {
template<>
struct StructureFieldCodec<CVec2, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 8;
  static bool Decode(const std::uint8_t* source, std::size_t length, CVec2* value) {
    if (!value) return false;
    float fields[2];
    if (!DecodeStructureFloatFields(source, length, fields, 2)) return false;
    value->x = fields[0]; value->y = fields[1];
    return true;
  }
  static bool Encode(const CVec2& value, std::uint8_t* destination, std::size_t length) {
    const float fields[2] = {value.x, value.y};
    return EncodeStructureFloatFields(fields, 2, destination, length);
  }
};

template<>
struct StructureFieldCodec<CVec3, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 12;
  static bool Decode(const std::uint8_t* source, std::size_t length, CVec3* value) {
    if (!value) return false;
    float fields[3];
    if (!DecodeStructureFloatFields(source, length, fields, 3)) return false;
    value->x = fields[0]; value->y = fields[1]; value->z = fields[2];
    return true;
  }
  static bool Encode(const CVec3& value, std::uint8_t* destination, std::size_t length) {
    const float fields[3] = {value.x, value.y, value.z};
    return EncodeStructureFloatFields(fields, 3, destination, length);
  }
};

template<>
struct StructureFieldCodec<CVec4, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 16;
  static bool Decode(const std::uint8_t* source, std::size_t length, CVec4* value) {
    if (!value) return false;
    float fields[4];
    if (!DecodeStructureFloatFields(source, length, fields, 4)) return false;
    value->x = fields[0]; value->y = fields[1]; value->z = fields[2]; value->w = fields[3];
    return true;
  }
  static bool Encode(const CVec4& value, std::uint8_t* destination, std::size_t length) {
    const float fields[4] = {value.x, value.y, value.z, value.w};
    return EncodeStructureFloatFields(fields, 4, destination, length);
  }
};

template<>
struct StructureFieldCodec<CQuat, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 16;
  static bool Decode(const std::uint8_t* source, std::size_t length, CQuat* value) {
    if (!value) return false;
    float fields[4];
    if (!DecodeStructureFloatFields(source, length, fields, 4)) return false;
    *value = CQuat(fields[0], fields[1], fields[2], fields[3]);
    return true;
  }
  static bool Encode(const CQuat& value, std::uint8_t* destination, std::size_t length) {
    float fields[4];
    value.GetComponentsForWire(fields);
    return EncodeStructureFloatFields(fields, 4, destination, length);
  }
};

template<>
struct StructureFieldCodec<SHMatrix, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 64;
  static bool Decode(const std::uint8_t* source, std::size_t length, SHMatrix* value) {
    if (!value) return false;
    float fields[16];
    if (!DecodeStructureFloatFields(source, length, fields, 16)) return false;
    value->_11 = fields[0]; value->_12 = fields[1]; value->_13 = fields[2]; value->_14 = fields[3];
    value->_21 = fields[4]; value->_22 = fields[5]; value->_23 = fields[6]; value->_24 = fields[7];
    value->_31 = fields[8]; value->_32 = fields[9]; value->_33 = fields[10]; value->_34 = fields[11];
    value->_41 = fields[12]; value->_42 = fields[13]; value->_43 = fields[14]; value->_44 = fields[15];
    return true;
  }
  static bool Encode(const SHMatrix& value, std::uint8_t* destination, std::size_t length) {
    const float fields[16] = {
        value._11, value._12, value._13, value._14,
        value._21, value._22, value._23, value._24,
        value._31, value._32, value._33, value._34,
        value._41, value._42, value._43, value._44};
    return EncodeStructureFloatFields(fields, 16, destination, length);
  }
};
} // namespace S2FileIO

#endif
