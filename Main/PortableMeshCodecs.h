#ifndef S2_MAIN_PORTABLE_MESH_CODECS_H
#define S2_MAIN_PORTABLE_MESH_CODECS_H

#include "GGeometry.h"
#include "GObjectInfo.h"
#include "GFileSkin.h"
#include "../FileIO/PortableMeshWire.h"

static_assert(sizeof(NGScene::SLoadVertex) == 60, "mesh resource vertex size");
static_assert(sizeof(NGScene::SLoadVertexWeight) == 12, "mesh resource weight size");

namespace S2FileIO {

template<>
struct StructureFieldCodec<NGScene::SLoadVertex, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 60;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NGScene::SLoadVertex* value) {
    if (!value) return false;
    LoadVertexFields fields{};
    if (!DecodeLoadVertex(source, length, &fields)) return false;
    value->pos = CVec3(fields.position[0], fields.position[1], fields.position[2]);
    value->normal = CVec3(fields.normal[0], fields.normal[1], fields.normal[2]);
    value->tex = CVec2(fields.texcoord[0], fields.texcoord[1]);
    value->texU = CVec3(fields.tangentU[0], fields.tangentU[1], fields.tangentU[2]);
    value->texV = CVec3(fields.tangentV[0], fields.tangentV[1], fields.tangentV[2]);
    value->dwColor = fields.color;
    return true;
  }
  static bool Encode(const NGScene::SLoadVertex& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeLoadVertex(LoadVertexFields{
        {value.pos.x, value.pos.y, value.pos.z},
        {value.normal.x, value.normal.y, value.normal.z},
        {value.tex.x, value.tex.y},
        {value.texU.x, value.texU.y, value.texU.z},
        {value.texV.x, value.texV.y, value.texV.z},
        value.dwColor}, destination, length);
  }
};

template<>
struct StructureFieldCodec<NGScene::SLoadVertexWeight, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 12;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NGScene::SLoadVertexWeight* value) {
    if (!value) return false;
    LoadVertexWeightFields fields{};
    if (!DecodeLoadVertexWeight(source, length, &fields)) return false;
    value->fWeight = fields.weight;
    value->nVertex = fields.vertex;
    value->nBone = fields.bone;
    return true;
  }
  static bool Encode(const NGScene::SLoadVertexWeight& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeLoadVertexWeight(LoadVertexWeightFields{
        value.fWeight, value.nVertex, value.nBone}, destination, length);
  }
};

} // namespace S2FileIO

#endif
