#include "../Main/StdAfx.h"
#include "../Main/PortableMeshCodecs.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

static_assert(offsetof(NGScene::SLoadVertex, pos) == 0 &&
              offsetof(NGScene::SLoadVertex, normal) == 12 &&
              offsetof(NGScene::SLoadVertex, tex) == 24 &&
              offsetof(NGScene::SLoadVertex, texU) == 32 &&
              offsetof(NGScene::SLoadVertex, texV) == 44 &&
              offsetof(NGScene::SLoadVertex, dwColor) == 56,
              "mesh resource vertex offsets");
static_assert(offsetof(NGScene::SLoadVertexWeight, fWeight) == 0 &&
              offsetof(NGScene::SLoadVertexWeight, nVertex) == 4 &&
              offsetof(NGScene::SLoadVertexWeight, nBone) == 8,
              "mesh resource weight offsets");

int main() {
  using VertexCodec = S2FileIO::StructureFieldCodec<NGScene::SLoadVertex>;
  using WeightCodec = S2FileIO::StructureFieldCodec<NGScene::SLoadVertexWeight>;
  if (!VertexCodec::kPortable || VertexCodec::kWireSize != 60 ||
      !WeightCodec::kPortable || WeightCodec::kWireSize != 12) return 1;
  NGScene::SLoadVertex vertex{}, decodedVertex{};
  vertex.pos = CVec3(1.0f, -2.0f, 3.0f);
  vertex.normal = CVec3(-4.0f, 5.0f, 6.0f);
  vertex.tex = CVec2(0.5f, -0.0f);
  vertex.texU = CVec3(7.0f, 8.0f, 9.0f);
  vertex.texV = CVec3(-10.0f, 11.0f, 12.0f);
  vertex.dwColor = 0xa1b2c3d4u;
  std::uint8_t wire[60] = {};
  if (!VertexCodec::Encode(vertex, wire, sizeof(wire)) ||
      std::memcmp(wire, &vertex, sizeof(wire)) != 0 ||
      !VertexCodec::Decode(wire, sizeof(wire), &decodedVertex) ||
      std::memcmp(&vertex, &decodedVertex, sizeof(vertex)) != 0 ||
      VertexCodec::Decode(wire, 59, &decodedVertex)) return 2;
  NGScene::SLoadVertexWeight weight{0.5f, -2, 0x12345678}, decodedWeight{};
  std::uint8_t weightWire[12] = {};
  if (!WeightCodec::Encode(weight, weightWire, sizeof(weightWire)) ||
      std::memcmp(weightWire, &weight, sizeof(weightWire)) != 0 ||
      !WeightCodec::Decode(weightWire, sizeof(weightWire), &decodedWeight) ||
      std::memcmp(&weight, &decodedWeight, sizeof(weight)) != 0 ||
      WeightCodec::Decode(weightWire, 11, &decodedWeight)) return 3;
  return 0;
}
