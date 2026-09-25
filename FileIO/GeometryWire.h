#ifndef S2_FILEIO_GEOMETRY_WIRE_H
#define S2_FILEIO_GEOMETRY_WIRE_H

#include "PortableStructureChunks.h"
#include "PortableGameGeometry.h"
#include "..\Misc\Geom.h"

static_assert(sizeof(STriangle) == 6, "triangle wire size");
static_assert(sizeof(SSphere) == 16, "sphere wire size");
static_assert(sizeof(SMassSphere) == 20, "mass sphere wire size");
static_assert(sizeof(SBound) == 28, "bound wire size");

namespace S2FileIO {
template<>
struct StructureFieldCodec<STriangle, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 6;
  static bool Decode(const std::uint8_t* source, std::size_t length, STriangle* value) {
    if (!value) return false;
    TriangleFields fields{};
    if (!DecodeTriangle(source, length, &fields)) return false;
    value->i1 = fields.first; value->i2 = fields.second; value->i3 = fields.third;
    return true;
  }
  static bool Encode(const STriangle& value, std::uint8_t* destination, std::size_t length) {
    return EncodeTriangle(TriangleFields{value.i1, value.i2, value.i3}, destination, length);
  }
};

template<>
struct StructureFieldCodec<SSphere, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 16;
  static bool Decode(const std::uint8_t* source, std::size_t length, SSphere* value) {
    if (!value) return false;
    SphereFields fields{};
    if (!DecodeSphere(source, length, &fields)) return false;
    value->ptCenter = CVec3(fields.center[0], fields.center[1], fields.center[2]);
    value->fRadius = fields.radius;
    return true;
  }
  static bool Encode(const SSphere& value, std::uint8_t* destination, std::size_t length) {
    return EncodeSphere(SphereFields{{value.ptCenter.x, value.ptCenter.y, value.ptCenter.z},
                                      value.fRadius}, destination, length);
  }
};

template<>
struct StructureFieldCodec<SMassSphere, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 20;
  static bool Decode(const std::uint8_t* source, std::size_t length, SMassSphere* value) {
    if (!value) return false;
    MassSphereFields fields{};
    if (!DecodeMassSphere(source, length, &fields)) return false;
    value->ptCenter = CVec3(fields.center[0], fields.center[1], fields.center[2]);
    value->fRadius = fields.radius; value->fMass = fields.mass;
    return true;
  }
  static bool Encode(const SMassSphere& value, std::uint8_t* destination, std::size_t length) {
    return EncodeMassSphere(MassSphereFields{{value.ptCenter.x, value.ptCenter.y, value.ptCenter.z},
                                              value.fRadius, value.fMass}, destination, length);
  }
};

template<>
struct StructureFieldCodec<SBound, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 28;
  static bool Decode(const std::uint8_t* source, std::size_t length, SBound* value) {
    if (!value) return false;
    BoundFields fields{};
    if (!DecodeBound(source, length, &fields)) return false;
    value->s.ptCenter = CVec3(fields.center[0], fields.center[1], fields.center[2]);
    value->s.fRadius = fields.radius;
    value->ptHalfBox = CVec3(fields.halfBox[0], fields.halfBox[1], fields.halfBox[2]);
    return true;
  }
  static bool Encode(const SBound& value, std::uint8_t* destination, std::size_t length) {
    return EncodeBound(BoundFields{{value.s.ptCenter.x, value.s.ptCenter.y, value.s.ptCenter.z},
                                    value.s.fRadius,
                                    {value.ptHalfBox.x, value.ptHalfBox.y, value.ptHalfBox.z}},
                       destination, length);
  }
};

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
struct StructureFieldCodec<SPlane, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 16;
  static bool Decode(const std::uint8_t* source, std::size_t length, SPlane* value) {
    if (!value) return false;
    float fields[4];
    if (!DecodeStructureFloatFields(source, length, fields, 4)) return false;
    value->n.x = fields[0]; value->n.y = fields[1]; value->n.z = fields[2]; value->d = fields[3];
    return true;
  }
  static bool Encode(const SPlane& value, std::uint8_t* destination, std::size_t length) {
    const float fields[4] = {value.n.x, value.n.y, value.n.z, value.d};
    return EncodeStructureFloatFields(fields, 4, destination, length);
  }
};

template<>
struct StructureFieldCodec<SPlane[6], void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 96;
  static bool Decode(const std::uint8_t* source, std::size_t length, SPlane (*value)[6]) {
    if (!value) return false;
    float fields[24];
    if (!DecodeStructurePlaneBox(source, length, fields)) return false;
    for (std::size_t i = 0; i < 6; ++i)
      (*value)[i] = SPlane(CVec3(fields[i * 4], fields[i * 4 + 1], fields[i * 4 + 2]),
                           fields[i * 4 + 3]);
    return true;
  }
  static bool Encode(const SPlane (&value)[6], std::uint8_t* destination, std::size_t length) {
    float fields[24];
    for (std::size_t i = 0; i < 6; ++i) {
      fields[i * 4] = value[i].n.x; fields[i * 4 + 1] = value[i].n.y;
      fields[i * 4 + 2] = value[i].n.z; fields[i * 4 + 3] = value[i].d;
    }
    return EncodeStructurePlaneBox(fields, destination, length);
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
