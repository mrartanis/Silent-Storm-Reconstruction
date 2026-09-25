#include "../Main/StdAfx.h"
#include "../Main/BuildingPart.h"

#include <cstdint>

struct LegacyPart {
  union {
    std::uint32_t nID;
    struct { int nFloor : 6; int x : 13; int y : 13; };
  };
};
static_assert(sizeof(LegacyPart) == 4, "legacy building part word");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NBuilding::SPart>;
  if (!Codec::kPortable || Codec::kWireSize != 4 ||
      sizeof(NBuilding::SPart) != 4) return 1;
  const int floors[] = {-32, -16, -1, 0, 1, 31};
  const int coords[] = {-4096, -1, 0, 1, 4095};
  for (int floor : floors) for (int x : coords) for (int y : coords) {
    LegacyPart legacy{};
    legacy.nFloor = floor;
    legacy.x = x;
    legacy.y = y;
    NBuilding::SPart part(floor, x, y);
    if (part.nID != legacy.nID || part.GetFloor() != legacy.nFloor ||
        part.GetX() != legacy.x || part.GetY() != legacy.y) return 2;
    std::uint8_t wire[4] = {};
    NBuilding::SPart loaded;
    if (!Codec::Encode(part, wire, 4) || !Codec::Decode(wire, 4, &loaded) ||
        loaded.nID != legacy.nID) return 3;
  }
  return 0;
}
