#ifndef __GDecalInfo_H_
#define __GDecalInfo_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../FileIO/PortableRenderStateWire.h"

namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDecalMappingInfo
{
	CVec3 vCenter, vNormal;
	float fRadius, fRotation;
};
}

static_assert(sizeof(NGScene::SDecalMappingInfo) == 32,
              "decal mapping wire size");
namespace S2FileIO {
template<>
struct StructureFieldCodec<NGScene::SDecalMappingInfo, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 32;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NGScene::SDecalMappingInfo* value) {
    if (!value) return false;
    float f[8];
    if (!DecodeRenderFloatFields<8>(source, length, f)) return false;
    value->vCenter = CVec3(f[0], f[1], f[2]);
    value->vNormal = CVec3(f[3], f[4], f[5]);
    value->fRadius = f[6]; value->fRotation = f[7];
    return true;
  }
  static bool Encode(const NGScene::SDecalMappingInfo& value,
                     std::uint8_t* destination, std::size_t length) {
    const float f[8] = {value.vCenter.x, value.vCenter.y, value.vCenter.z,
                        value.vNormal.x, value.vNormal.y, value.vNormal.z,
                        value.fRadius, value.fRotation};
    return EncodeRenderFloatFields<8>(f, destination, length);
  }
};
} // namespace S2FileIO
#endif
