#include "../Main/StdAfx.h"
#include "../Main/aiColourer.h"

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
  return 0;
}
