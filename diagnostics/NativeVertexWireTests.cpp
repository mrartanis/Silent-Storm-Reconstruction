#include "../Main/StdAfx.h"
#include "../Main/GGeometry.h"

#include <cstdint>

int main() {
  using VertexCodec = S2FileIO::StructureFieldCodec<NGScene::SVertex>;
  using WeightCodec = S2FileIO::StructureFieldCodec<NGScene::SVertexWeight>;
  if (!VertexCodec::kPortable || VertexCodec::kWireSize != 32 ||
      !WeightCodec::kPortable || WeightCodec::kWireSize != 20) return 1;
  NGScene::SVertex vertex = {};
  vertex.pos = CVec3(1.0f, 2.0f, 3.0f);
  vertex.normal.z = 4; vertex.normal.y = 5; vertex.normal.x = 6; vertex.normal.w = 7;
  vertex.texU.z = 8; vertex.texU.y = 9; vertex.texU.x = 10; vertex.texU.w = 11;
  vertex.texV.z = 12; vertex.texV.y = 13; vertex.texV.x = 14; vertex.texV.w = 15;
  vertex.tex = CVec2(0.5f, -0.5f);
  std::uint8_t wire[32] = {};
  NGScene::SVertex decoded = {};
  if (!VertexCodec::Encode(vertex, wire, sizeof(wire)) ||
      wire[12] != 4 || wire[13] != 5 || wire[14] != 6 || wire[15] != 7 ||
      wire[20] != 12 || wire[23] != 15 ||
      !VertexCodec::Decode(wire, sizeof(wire), &decoded) ||
      decoded.pos.x != 1.0f || decoded.pos.y != 2.0f || decoded.pos.z != 3.0f ||
      decoded.normal.z != 4 || decoded.normal.x != 6 || decoded.texU.w != 11 ||
      decoded.texV.y != 13 || decoded.tex.x != 0.5f || decoded.tex.y != -0.5f) return 1;
  NGScene::SVertexWeight weight = {};
  weight.fWeights[0] = 1.0f;
  weight.fWeights[1] = 0.5f;
  weight.cBoneIndices[0] = 17;
  weight.cBoneIndices[3] = 255;
  std::uint8_t weightWire[20] = {};
  NGScene::SVertexWeight decodedWeight = {};
  if (!WeightCodec::Encode(weight, weightWire, sizeof(weightWire)) ||
      weightWire[16] != 17 || weightWire[19] != 255 ||
      !WeightCodec::Decode(weightWire, sizeof(weightWire), &decodedWeight) ||
      decodedWeight.fWeights[0] != 1.0f || decodedWeight.fWeights[1] != 0.5f ||
      decodedWeight.cBoneIndices[0] != 17 || decodedWeight.cBoneIndices[3] != 255) return 1;
  return 0;
}
