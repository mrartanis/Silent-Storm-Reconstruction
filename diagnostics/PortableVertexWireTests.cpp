#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstring>

int main() {
  S2FileIO::StructureVertexFields vertex;
  vertex.position[0] = 1.0f;
  vertex.position[1] = -2.0f;
  vertex.position[2] = 0.5f;
  for (std::size_t i = 0; i < 12; ++i) vertex.basis[i] = static_cast<std::uint8_t>(i + 1);
  vertex.texture[0] = 3.0f;
  vertex.texture[1] = -4.0f;
  std::uint8_t wire[32] = {};
  S2FileIO::StructureVertexFields decoded;
  const std::uint8_t start[] = {0, 0, 0x80, 0x3f, 0, 0, 0, 0xc0, 0, 0, 0, 0x3f};
  if (!S2FileIO::EncodeStructureVertex(vertex, wire, sizeof(wire)) ||
      std::memcmp(wire, start, sizeof(start)) != 0 ||
      wire[12] != 1 || wire[23] != 12 || wire[24] != 0 || wire[26] != 0x40 ||
      wire[28] != 0 || wire[31] != 0xc0 ||
      !S2FileIO::DecodeStructureVertex(wire, sizeof(wire), &decoded) ||
      decoded.position[0] != 1.0f || decoded.position[1] != -2.0f ||
      decoded.position[2] != 0.5f || decoded.texture[0] != 3.0f ||
      decoded.texture[1] != -4.0f || decoded.basis[0] != 1 || decoded.basis[11] != 12 ||
      S2FileIO::DecodeStructureVertex(wire, 31, &decoded) ||
      S2FileIO::EncodeStructureVertex(vertex, wire, 31)) return 1;

  S2FileIO::StructureVertexWeightFields weight;
  weight.weights[0] = 1.0f;
  weight.weights[1] = 0.5f;
  weight.weights[2] = 0.25f;
  weight.weights[3] = 0.0f;
  weight.boneIndices[0] = 3;
  weight.boneIndices[1] = 7;
  weight.boneIndices[2] = 11;
  weight.boneIndices[3] = 255;
  std::uint8_t weightWire[20] = {};
  S2FileIO::StructureVertexWeightFields decodedWeight;
  if (!S2FileIO::EncodeStructureVertexWeight(weight, weightWire, sizeof(weightWire)) ||
      weightWire[2] != 0x80 || weightWire[3] != 0x3f || weightWire[7] != 0x3f ||
      weightWire[16] != 3 || weightWire[17] != 7 || weightWire[18] != 11 || weightWire[19] != 255 ||
      !S2FileIO::DecodeStructureVertexWeight(weightWire, sizeof(weightWire), &decodedWeight) ||
      decodedWeight.weights[0] != 1.0f || decodedWeight.weights[1] != 0.5f ||
      decodedWeight.weights[2] != 0.25f || decodedWeight.weights[3] != 0.0f ||
      decodedWeight.boneIndices[3] != 255 ||
      S2FileIO::DecodeStructureVertexWeight(weightWire, 19, &decodedWeight) ||
      S2FileIO::EncodeStructureVertexWeight(weight, weightWire, 19)) return 1;
  return 0;
}
