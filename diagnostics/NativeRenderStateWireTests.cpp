#include "../Main/StdAfx.h"
#include "../Main/GDecalInfo.h"
#include "../Main/GRenderCore.h"
#include "../Main/GLightmapStateWire.h"

#include <cstdint>
#include <cstring>

template<class T>
bool CheckPlain(const T& value) {
  using Codec = S2FileIO::StructureFieldCodec<T>;
  if (!Codec::kPortable || Codec::kWireSize != sizeof(T)) return false;
  std::uint8_t wire[96] = {};
  T loaded;
  return Codec::Encode(value, wire, sizeof(T)) &&
         std::memcmp(wire, &value, sizeof(T)) == 0 &&
         Codec::Decode(wire, sizeof(T), &loaded) &&
         std::memcmp(&loaded, &value, sizeof(T)) == 0 &&
         !Codec::Decode(wire, sizeof(T) - 1, &loaded) &&
         !Codec::Encode(value, wire, sizeof(T) - 1);
}

int main() {
  NGScene::SDecalMappingInfo decal{};
  decal.vCenter = CVec3(1, 2, 3);
  decal.vNormal = CVec3(4, 5, 6);
  decal.fRadius = 7; decal.fRotation = 8;
  NGScene::SFogParams fog{};
  fog.fDist = 1; fog.vFogColor = CVec3(2, 3, 4);
  fog.fHeight = 5; fog.fDensity = 6;
  fog.vWaterColor = CVec3(7, 8, 9); fog.fCameraHeight = 10;
  fog.fVapourNoiseParam = 11; fog.fVapourSpeed = 12;
  fog.fVapourSwitchTime = 13; fog.fTime = 14;
  fog.fDistStart = 15; fog.fVapourHeightStart = 16;
  NGScene::SDirectionalDepthInfo depth{};
  depth.vChannelSelect = CVec4(1, 2, 3, 4);
  depth.vDepth = CVec4(5, 6, 7, 8);
  depth.vVecU = CVec4(9, 10, 11, 12);
  depth.vVecV = CVec4(13, 14, 15, 16);
  NGScene::SDynamicAmbientInfo ambient{};
  NGScene::SDynamicAmbientInfo::SPad* pads[6] = {
      &ambient.vXPos, &ambient.vXNeg, &ambient.vYPos,
      &ambient.vYNeg, &ambient.vZPos, &ambient.vZNeg};
  for (int i = 0; i < 6; ++i) {
    pads[i]->v = CVec3(float(i), float(i + 1), float(i + 2));
    pads[i]->f = float(i + 3);
  }
  NGScene::SLightStateCalcSeed seed;
  seed.nSeed = 0x12345678;
  if (!CheckPlain(decal) || !CheckPlain(fog) || !CheckPlain(depth) ||
      !CheckPlain(ambient) || !CheckPlain(seed)) return 1;

  using Directional = NGScene::SGlobalIlluminationInfo::SDirectional;
  using Codec = S2FileIO::StructureFieldCodec<Directional>;
  Directional dir(CVec3(1, 2, 3), CVec3(4, 5, 6), true);
  std::uint8_t wire[28] = {};
  Directional loaded;
  if (!Codec::kPortable || Codec::kWireSize != 28 ||
      !Codec::Encode(dir, wire, sizeof(wire)) ||
      std::memcmp(wire, &dir, 24) != 0 || wire[24] != 1 ||
      wire[25] != 0 || wire[26] != 0 || wire[27] != 0 ||
      !Codec::Decode(wire, sizeof(wire), &loaded) ||
      std::memcmp(&loaded, &dir, 24) != 0 || !loaded.bIsRendered ||
      Codec::Decode(wire, 27, &loaded) || Codec::Encode(dir, wire, 27))
    return 2;
  return 0;
}
