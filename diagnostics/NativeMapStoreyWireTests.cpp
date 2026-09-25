#include "../Main/StdAfx.h"
#include "../Main/MapBuildingInfo.h"

#include <cstdint>
#include <cstring>

int main() {
  using Storey = SMapBuilding::SStorey;
  using Codec = S2FileIO::StructureFieldCodec<Storey>;
  if (!Codec::kPortable || Codec::kWireSize != 8 || sizeof(Storey) != 8)
    return 1;
  const int values[] = {-32, -7, -1, 0, 1, 12, 31};
  for (int local : values) for (int global : values) {
    Storey storey(local, global);
    std::uint8_t wire[8] = {};
    Storey loaded;
    if (!Codec::Encode(storey, wire, 8) ||
        std::memcmp(wire, &storey, 8) != 0 ||
        !Codec::Decode(wire, 8, &loaded) ||
        loaded.nFloor != local || loaded.nRealFloor != global)
      return 2;
  }
  return 0;
}
