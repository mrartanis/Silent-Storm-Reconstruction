#include "../FileIO/PortableMeshWire.h"

#include <cmath>
#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::LoadVertexFields vertex{
      {1.0f, -2.0f, 0.0f}, {0.5f, 0.0f, 0.0f},
      {0.0f, -0.0f}, {0.0f, 0.0f, 4.0f},
      {0.0f, 0.0f, -1.0f}, 0xa1b2c3d4u};
  const std::uint8_t expected[60] = {
      0, 0, 0x80, 0x3f, 0, 0, 0, 0xc0, 0, 0, 0, 0,
      0, 0, 0, 0x3f, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0x80,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0x40,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0xbf,
      0xd4, 0xc3, 0xb2, 0xa1};
  std::uint8_t wire[60] = {};
  S2FileIO::LoadVertexFields decoded{};
  if (!S2FileIO::EncodeLoadVertex(vertex, wire, sizeof(wire)) ||
      std::memcmp(wire, expected, sizeof(wire)) != 0 ||
      !S2FileIO::DecodeLoadVertex(expected, sizeof(expected), &decoded) ||
      decoded.position[0] != 1.0f || decoded.position[1] != -2.0f ||
      decoded.normal[0] != 0.5f || !std::signbit(decoded.texcoord[1]) ||
      decoded.tangentU[2] != 4.0f || decoded.tangentV[2] != -1.0f ||
      decoded.color != 0xa1b2c3d4u ||
      S2FileIO::DecodeLoadVertex(expected, 59, &decoded) ||
      S2FileIO::EncodeLoadVertex(vertex, wire, 59)) return 1;

  const S2FileIO::LoadVertexWeightFields weight{0.5f, -2, 0x12345678};
  const std::uint8_t expectedWeight[12] = {
      0, 0, 0, 0x3f, 0xfe, 0xff, 0xff, 0xff,
      0x78, 0x56, 0x34, 0x12};
  std::uint8_t wireWeight[12] = {};
  S2FileIO::LoadVertexWeightFields decodedWeight{};
  if (!S2FileIO::EncodeLoadVertexWeight(weight, wireWeight, sizeof(wireWeight)) ||
      std::memcmp(wireWeight, expectedWeight, sizeof(wireWeight)) != 0 ||
      !S2FileIO::DecodeLoadVertexWeight(expectedWeight, sizeof(expectedWeight),
                                       &decodedWeight) ||
      decodedWeight.weight != 0.5f || decodedWeight.vertex != -2 ||
      decodedWeight.bone != 0x12345678 ||
      S2FileIO::DecodeLoadVertexWeight(expectedWeight, 11, &decodedWeight) ||
      S2FileIO::EncodeLoadVertexWeight(weight, wireWeight, 11)) return 2;
  return 0;
}
