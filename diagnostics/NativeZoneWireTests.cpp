#include "../Main/StdAfx.h"
#include "../Main/aiColourer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

struct LegacyZone { WORD color; int layer; };
static_assert(sizeof(LegacyZone) == 8, "historical zone size");
static_assert(offsetof(LegacyZone, layer) == 4, "historical zone layer offset");
static_assert(offsetof(NAI::SZone, nLayer) == 4, "native zone layer offset");

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
  return 0;
}
