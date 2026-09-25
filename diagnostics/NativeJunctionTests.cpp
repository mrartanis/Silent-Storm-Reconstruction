#include "../Main/StdAfx.h"
#include "../Main/aiObject.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

static_assert(sizeof(NAI::SJunction) == 16, "historical junction size");
static_assert(offsetof(NAI::SJunction, pt) == 0, "junction position offset");
static_assert(offsetof(NAI::SJunction, bGround) == 12, "junction ground offset");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NAI::SJunction>;
  if (!Codec::kPortable || Codec::kWireSize != 16) return 1;
  const float coords[] = {0.0f, 1.0f, -2.0f, 0.5f, -0.0f};
  for (float x : coords) for (float y : coords) for (float z : coords)
    for (int ground = 0; ground < 2; ++ground) {
      NAI::SJunction original(CVec3(x, y, z), ground != 0);
      std::uint8_t wire[16] = {};
      NAI::SJunction loaded;
      if (!Codec::Encode(original, wire, 16) ||
          std::memcmp(wire, &original, 13) != 0 ||
          wire[13] || wire[14] || wire[15] ||
          !Codec::Decode(wire, 16, &loaded) ||
          loaded.pt.x != x || loaded.pt.y != y || loaded.pt.z != z ||
          loaded.bGround != (ground != 0)) return 2;
    }
  return 0;
}
