#include "../FileIO/PortableGameGeometry.h"

#include <cstdint>
#include <cmath>
#include <cstring>

int main() {
  const float fields[4] = {1.0f, -2.0f, 0.5f, -0.0f};
  const std::uint8_t expected[16] = {
      0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0xc0,
      0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x80};
  std::uint8_t encoded[16] = {};
  float decoded[4] = {};
  if (!S2FileIO::EncodeStructureFloatFields(fields, 4, encoded, sizeof(encoded)) ||
      std::memcmp(encoded, expected, sizeof(expected)) != 0 ||
      !S2FileIO::DecodeStructureFloatFields(expected, sizeof(expected), decoded, 4) ||
      decoded[0] != 1.0f || decoded[1] != -2.0f || decoded[2] != 0.5f ||
      !std::signbit(decoded[3]) ||
      S2FileIO::DecodeStructureFloatFields(expected, 15, decoded, 4) ||
      S2FileIO::EncodeStructureFloatFields(fields, 4, encoded, 15) ||
      S2FileIO::DecodeStructureFloatFields(expected, sizeof(expected), decoded, 0) ||
      S2FileIO::EncodeStructureFloatFields(fields, 17, encoded, 68)) return 1;
  float boxFields[24] = {}, decodedBox[24] = {};
  for (int i = 0; i < 6; ++i) {
    boxFields[i * 4] = static_cast<float>(i + 1);
    boxFields[i * 4 + 1] = -2.0f;
    boxFields[i * 4 + 2] = 0.5f;
    boxFields[i * 4 + 3] = -0.0f;
  }
  std::uint8_t boxWire[96] = {};
  if (!S2FileIO::EncodeStructurePlaneBox(boxFields, boxWire, sizeof(boxWire)) ||
      boxWire[3] != 0x3f || boxWire[19] != 0x40 ||
      boxWire[15] != 0x80 || boxWire[95] != 0x80 ||
      !S2FileIO::DecodeStructurePlaneBox(boxWire, sizeof(boxWire), decodedBox) ||
      S2FileIO::DecodeStructurePlaneBox(boxWire, 95, decodedBox) ||
      S2FileIO::EncodeStructurePlaneBox(boxFields, boxWire, 95)) return 1;
  for (int i = 0; i < 6; ++i)
    if (decodedBox[i * 4] != i + 1 || decodedBox[i * 4 + 1] != -2.0f ||
        decodedBox[i * 4 + 2] != 0.5f || !std::signbit(decodedBox[i * 4 + 3])) return 1;
  const S2FileIO::TriangleFields triangle{0x1234, 0xabcd, 0xffff};
  const std::uint8_t triangleExpected[6] = {0x34, 0x12, 0xcd, 0xab, 0xff, 0xff};
  std::uint8_t triangleWire[6] = {};
  S2FileIO::TriangleFields triangleDecoded{};
  if (!S2FileIO::EncodeTriangle(triangle, triangleWire, 6) ||
      std::memcmp(triangleWire, triangleExpected, 6) != 0 ||
      !S2FileIO::DecodeTriangle(triangleExpected, 6, &triangleDecoded) ||
      triangleDecoded.first != triangle.first ||
      triangleDecoded.second != triangle.second ||
      triangleDecoded.third != triangle.third ||
      S2FileIO::DecodeTriangle(triangleExpected, 5, &triangleDecoded)) return 2;
  const S2FileIO::SphereFields sphere{{1.0f, -2.0f, 0.5f}, -0.0f};
  std::uint8_t sphereWire[16] = {};
  S2FileIO::SphereFields sphereDecoded{};
  if (!S2FileIO::EncodeSphere(sphere, sphereWire, 16) ||
      std::memcmp(sphereWire, expected, 16) != 0 ||
      !S2FileIO::DecodeSphere(expected, 16, &sphereDecoded) ||
      sphereDecoded.center[0] != 1.0f || sphereDecoded.center[1] != -2.0f ||
      sphereDecoded.center[2] != 0.5f || !std::signbit(sphereDecoded.radius)) return 3;
  const S2FileIO::MassSphereFields mass{{1.0f, -2.0f, 0.5f}, -0.0f, 3.0f};
  std::uint8_t massWire[20] = {};
  S2FileIO::MassSphereFields massDecoded{};
  if (!S2FileIO::EncodeMassSphere(mass, massWire, 20) ||
      std::memcmp(massWire, expected, 16) != 0 || massWire[19] != 0x40 ||
      !S2FileIO::DecodeMassSphere(massWire, 20, &massDecoded) ||
      massDecoded.mass != 3.0f || !std::signbit(massDecoded.radius)) return 4;
  const S2FileIO::BoundFields bound{{1.0f, -2.0f, 0.5f}, -0.0f,
                                     {3.0f, 4.0f, 5.0f}};
  std::uint8_t boundWire[28] = {};
  S2FileIO::BoundFields boundDecoded{};
  if (!S2FileIO::EncodeBound(bound, boundWire, 28) ||
      std::memcmp(boundWire, expected, 16) != 0 ||
      boundWire[19] != 0x40 || boundWire[23] != 0x40 || boundWire[27] != 0x40 ||
      !S2FileIO::DecodeBound(boundWire, 28, &boundDecoded) ||
      boundDecoded.center[0] != 1.0f || boundDecoded.center[1] != -2.0f ||
      boundDecoded.center[2] != 0.5f || !std::signbit(boundDecoded.radius) ||
      boundDecoded.halfBox[0] != 3.0f || boundDecoded.halfBox[1] != 4.0f ||
      boundDecoded.halfBox[2] != 5.0f ||
      S2FileIO::DecodeBound(boundWire, 27, &boundDecoded)) return 5;
  return 0;
}
