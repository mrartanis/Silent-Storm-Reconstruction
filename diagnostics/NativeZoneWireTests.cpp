#include "../Main/StdAfx.h"
#include "../Main/aiColourer.h"
#include "../Main/GScene.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

struct LegacyZone { WORD color; int layer; };
static_assert(sizeof(LegacyZone) == 8, "historical zone size");
static_assert(offsetof(LegacyZone, layer) == 4, "historical zone layer offset");
static_assert(offsetof(NAI::SZone, nLayer) == 4, "native zone layer offset");
static_assert(offsetof(NAI::SNeighbour, wNodeNumber) == 0 &&
              offsetof(NAI::SNeighbour, wDistance) == 2 &&
              offsetof(NAI::SNeighbour, nLayer) == 4,
              "native neighbour offsets");
static_assert(offsetof(NAI::SLocalColorInfo, wAverageX) == 0 &&
              offsetof(NAI::SLocalColorInfo, wAverageY) == 2,
              "native local colour offsets");
static_assert(offsetof(NGScene::SGroupInfo, nLightGroup) == 0 &&
              offsetof(NGScene::SGroupInfo, nObjectGroup) == 2 &&
              offsetof(NGScene::SGroupSelect, nMaskAny) == 0 &&
              offsetof(NGScene::SGroupSelect, nMaskEvery) == 2,
              "native scene group offsets");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NAI::SZone>;
  if (!Codec::kPortable || Codec::kWireSize != 8) return 1;
  const WORD colors[] = {0, 1, 0x7fff, 0xffff};
  const int layers[] = {0, 1, -1, 255, -256, INT32_MIN, INT32_MAX};
  for (WORD color : colors) for (int layer : layers) {
    NAI::SZone zone(color, layer);
    LegacyZone old{};
    old.color = color;
    old.layer = layer;
    std::uint8_t wire[8] = {};
    NAI::SZone loaded;
    if (!Codec::Encode(zone, wire, 8) ||
        std::memcmp(wire, &old, 8) != 0 ||
        !Codec::Decode(wire, 8, &loaded) ||
        loaded.wColor != color || loaded.nLayer != layer) return 2;
  }
  NAI::SZone empty;
  if (!empty.IsNull() || empty.nLayer != 0) return 3;
  using NeighbourCodec = S2FileIO::StructureFieldCodec<NAI::SNeighbour>;
  using ColorCodec = S2FileIO::StructureFieldCodec<NAI::SLocalColorInfo>;
  if (!NeighbourCodec::kPortable || NeighbourCodec::kWireSize != 8 ||
      !ColorCodec::kPortable || ColorCodec::kWireSize != 4) return 4;
  NAI::SNeighbour neighbour(0x1234, 0xabcd, -2), decodedNeighbour;
  std::uint8_t neighbourWire[8] = {};
  if (!NeighbourCodec::Encode(neighbour, neighbourWire, sizeof(neighbourWire)) ||
      std::memcmp(neighbourWire, &neighbour, sizeof(neighbourWire)) != 0 ||
      !NeighbourCodec::Decode(neighbourWire, sizeof(neighbourWire),
                              &decodedNeighbour) ||
      std::memcmp(&neighbour, &decodedNeighbour, sizeof(neighbour)) != 0) return 5;
  NAI::SLocalColorInfo color, decodedColor;
  color.wAverageX = 0x1234;
  color.wAverageY = 0xabcd;
  std::uint8_t colorWire[4] = {};
  if (!ColorCodec::Encode(color, colorWire, sizeof(colorWire)) ||
      std::memcmp(colorWire, &color, sizeof(colorWire)) != 0 ||
      !ColorCodec::Decode(colorWire, sizeof(colorWire), &decodedColor) ||
      std::memcmp(&color, &decodedColor, sizeof(color)) != 0) return 6;
  using GroupCodec = S2FileIO::StructureFieldCodec<NGScene::SGroupInfo>;
  using SelectCodec = S2FileIO::StructureFieldCodec<NGScene::SGroupSelect>;
  if (!GroupCodec::kPortable || GroupCodec::kWireSize != 4 ||
      !SelectCodec::kPortable || SelectCodec::kWireSize != 4) return 7;
  NGScene::SGroupInfo group(0x1234, 0x80ff), decodedGroup;
  std::uint8_t groupWire[4] = {};
  if (!GroupCodec::Encode(group, groupWire, sizeof(groupWire)) ||
      std::memcmp(groupWire, &group, sizeof(groupWire)) != 0 ||
      !GroupCodec::Decode(groupWire, sizeof(groupWire), &decodedGroup) ||
      std::memcmp(&group, &decodedGroup, sizeof(group)) != 0) return 8;
  NGScene::SGroupSelect select(0x8000, 0x0fff), decodedSelect(0, 0);
  std::uint8_t selectWire[4] = {};
  if (!SelectCodec::Encode(select, selectWire, sizeof(selectWire)) ||
      std::memcmp(selectWire, &select, sizeof(selectWire)) != 0 ||
      !SelectCodec::Decode(selectWire, sizeof(selectWire), &decodedSelect) ||
      std::memcmp(&select, &decodedSelect, sizeof(select)) != 0) return 9;
  return 0;
}
