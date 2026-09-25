#ifndef S2_MAIN_GLIGHTMAP_STATE_WIRE_H
#define S2_MAIN_GLIGHTMAP_STATE_WIRE_H

#include "GLightmapCalc.h"
#include "../FileIO/PortableRenderStateWire.h"

static_assert(sizeof(NGScene::SGlobalIlluminationInfo::SDirectional) == 28,
              "global illumination directional wire size");
static_assert(sizeof(NGScene::SLightStateCalcSeed) == 4,
              "light state seed wire size");

namespace S2FileIO {
template<>
struct StructureFieldCodec<NGScene::SGlobalIlluminationInfo::SDirectional,
                           void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 28;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NGScene::SGlobalIlluminationInfo::SDirectional* value) {
    if (!value) return false;
    RenderDirectionalFields f{};
    if (!DecodeRenderDirectional(source, length, &f)) return false;
    value->vColor = CVec3(f.color[0], f.color[1], f.color[2]);
    value->vDir = CVec3(f.direction[0], f.direction[1], f.direction[2]);
    value->bIsRendered = f.rendered;
    return true;
  }
  static bool Encode(
      const NGScene::SGlobalIlluminationInfo::SDirectional& value,
      std::uint8_t* destination, std::size_t length) {
    const RenderDirectionalFields f{{value.vColor.x, value.vColor.y,
                                     value.vColor.z},
                                    {value.vDir.x, value.vDir.y,
                                     value.vDir.z},
                                    value.bIsRendered};
    return EncodeRenderDirectional(f, destination, length);
  }
};
template<>
struct StructureFieldCodec<NGScene::SLightStateCalcSeed, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 4;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NGScene::SLightStateCalcSeed* value) {
    return value && DecodeStructureScalar(source, length, &value->nSeed);
  }
  static bool Encode(const NGScene::SLightStateCalcSeed& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeStructureScalar(value.nSeed, destination, length);
  }
};
} // namespace S2FileIO

#endif
