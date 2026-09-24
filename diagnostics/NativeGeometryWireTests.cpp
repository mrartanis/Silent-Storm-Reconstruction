#include "../Main/StdAfx.h"
#include "../FileIO/GeometryWire.h"

#include <cstdint>
#include <cmath>

int main() {
  using Vec2 = S2FileIO::StructureFieldCodec<CVec2>;
  using Vec3 = S2FileIO::StructureFieldCodec<CVec3>;
  using Vec4 = S2FileIO::StructureFieldCodec<CVec4>;
  using Quat = S2FileIO::StructureFieldCodec<CQuat>;
  using Plane = S2FileIO::StructureFieldCodec<SPlane>;
  using PlaneBox = S2FileIO::StructureFieldCodec<SPlane[6]>;
  using Matrix = S2FileIO::StructureFieldCodec<SHMatrix>;
  if (!Vec2::kPortable || Vec2::kWireSize != 8 ||
      !Vec3::kPortable || Vec3::kWireSize != 12 ||
      !Vec4::kPortable || Vec4::kWireSize != 16 ||
      !Quat::kPortable || Quat::kWireSize != 16 ||
      !Plane::kPortable || Plane::kWireSize != 16 ||
      !PlaneBox::kPortable || PlaneBox::kWireSize != 96 ||
      !Matrix::kPortable || Matrix::kWireSize != 64) return 1;
  CVec2 a(1.0f, -2.0f), decodedA;
  CVec3 b(3.0f, 4.0f, 5.0f), decodedB;
  CVec4 c(6.0f, 7.0f, 8.0f, 9.0f), decodedC;
  std::uint8_t wire2[8] = {}, wire3[12] = {}, wire4[16] = {};
  if (!Vec2::Encode(a, wire2, sizeof(wire2)) ||
      !Vec3::Encode(b, wire3, sizeof(wire3)) ||
      !Vec4::Encode(c, wire4, sizeof(wire4)) ||
      wire2[3] != 0x3f || wire3[11] != 0x40 || wire4[15] != 0x41 ||
      !Vec2::Decode(wire2, sizeof(wire2), &decodedA) ||
      !Vec3::Decode(wire3, sizeof(wire3), &decodedB) ||
      !Vec4::Decode(wire4, sizeof(wire4), &decodedC) ||
      decodedA.x != a.x || decodedA.y != a.y ||
      decodedB.x != b.x || decodedB.y != b.y || decodedB.z != b.z ||
      decodedC.x != c.x || decodedC.y != c.y || decodedC.z != c.z || decodedC.w != c.w)
    return 1;
  CQuat quat(1.0f, -2.0f, 0.5f, -0.0f), decodedQuat;
  std::uint8_t wireQuat[16] = {};
  float quatFields[4] = {};
  if (!Quat::Encode(quat, wireQuat, sizeof(wireQuat)) ||
      wireQuat[3] != 0x3f || wireQuat[7] != 0xc0 ||
      wireQuat[11] != 0x3f || wireQuat[15] != 0x80 ||
      !Quat::Decode(wireQuat, sizeof(wireQuat), &decodedQuat) ||
      Quat::Decode(wireQuat, 15, &decodedQuat)) return 1;
  decodedQuat.GetComponentsForWire(quatFields);
  if (quatFields[0] != 1.0f || quatFields[1] != -2.0f || quatFields[2] != 0.5f ||
      !std::signbit(quatFields[3])) return 1;
  SPlane planes[6], decodedPlanes[6];
  for (int i = 0; i < 6; ++i)
    planes[i] = SPlane(CVec3(static_cast<float>(i + 1), -2.0f, 0.5f), -0.0f);
  std::uint8_t wirePlanes[96] = {};
  if (!PlaneBox::Encode(planes, wirePlanes, sizeof(wirePlanes)) ||
      wirePlanes[3] != 0x3f || wirePlanes[19] != 0x40 ||
      wirePlanes[15] != 0x80 || wirePlanes[95] != 0x80 ||
      !PlaneBox::Decode(wirePlanes, sizeof(wirePlanes), &decodedPlanes) ||
      PlaneBox::Decode(wirePlanes, 95, &decodedPlanes)) return 1;
  for (int i = 0; i < 6; ++i)
    if (decodedPlanes[i].n.x != i + 1 || decodedPlanes[i].n.y != -2.0f ||
        decodedPlanes[i].n.z != 0.5f || !std::signbit(decodedPlanes[i].d)) return 1;
  SHMatrix matrix;
  matrix._11 = 1; matrix._12 = 2; matrix._13 = 3; matrix._14 = 4;
  matrix._21 = 5; matrix._22 = 6; matrix._23 = 7; matrix._24 = 8;
  matrix._31 = 9; matrix._32 = 10; matrix._33 = 11; matrix._34 = 12;
  matrix._41 = 13; matrix._42 = 14; matrix._43 = 15; matrix._44 = 16;
  SHMatrix decodedMatrix;
  std::uint8_t wireMatrix[64] = {};
  if (!Matrix::Encode(matrix, wireMatrix, sizeof(wireMatrix)) ||
      wireMatrix[3] != 0x3f || wireMatrix[63] != 0x41 ||
      !Matrix::Decode(wireMatrix, sizeof(wireMatrix), &decodedMatrix) ||
      decodedMatrix._11 != 1 || decodedMatrix._14 != 4 ||
      decodedMatrix._21 != 5 || decodedMatrix._34 != 12 ||
      decodedMatrix._44 != 16 || Matrix::Decode(wireMatrix, 63, &decodedMatrix)) return 1;
  return 0;
}
