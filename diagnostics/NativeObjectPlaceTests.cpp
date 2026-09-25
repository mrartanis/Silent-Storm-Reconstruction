#include "../Main/StdAfx.h"
#include "../Main/wOSBase.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

static_assert(sizeof(NWorld::SObjectPlace) == 32, "historical object place size");
static_assert(offsetof(NWorld::SObjectPlace, ptPos) == 0, "position offset");
static_assert(offsetof(NWorld::SObjectPlace, ptScale) == 12, "scale offset");
static_assert(offsetof(NWorld::SObjectPlace, fAngle) == 24, "angle offset");
static_assert(offsetof(NWorld::SObjectPlace, nFloor) == 28, "floor offset");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NWorld::SObjectPlace>;
  if (!Codec::kPortable || Codec::kWireSize != 32) return 1;
  const int floors[] = {0, 1, -1, 255, INT32_MIN, INT32_MAX};
  for (int floor : floors) {
    NWorld::SObjectPlace place{};
    place.ptPos = CVec3(1.0f, -2.0f, 0.5f);
    place.ptScale = CVec3(3.0f, 4.0f, 5.0f);
    place.fAngle = 1.5f;
    place.nFloor = floor;
    std::uint8_t wire[32] = {};
    NWorld::SObjectPlace loaded{};
    if (!Codec::Encode(place, wire, 32) ||
        std::memcmp(wire, &place, 32) != 0 ||
        !Codec::Decode(wire, 32, &loaded) ||
        std::memcmp(&loaded, &place, 32) != 0) return 2;
  }
  return 0;
}
