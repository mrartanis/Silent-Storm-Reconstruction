#include "../Main/StdAfx.h"
#include "../Misc/2DArray.h"
#include "../Main/aiWaveSearch.h"

#include <cstdint>
#include <cstring>

int main() {
  using Move = NAI::CSquareMapCosts::SMove;
  using Codec = S2FileIO::StructureFieldCodec<Move>;
  if (!Codec::kPortable || Codec::kWireSize != 4 || sizeof(Move) != 4 ||
      sizeof(CTPoint<unsigned char>) != 2 || sizeof(WORD) != 2) return 1;
  const std::uint8_t coords[] = {0, 1, 127, 255};
  const std::uint16_t costs[] = {0, 1, 0x1234, 65535};
  for (std::uint8_t x : coords) for (std::uint8_t y : coords)
    for (std::uint16_t cost : costs) {
      Move move(CTPoint<unsigned char>(x, y), cost);
      std::uint8_t wire[4] = {};
      Move loaded;
      if (!Codec::Encode(move, wire, 4) ||
          std::memcmp(wire, &move, 4) != 0 ||
          !Codec::Decode(wire, 4, &loaded) ||
          loaded.parentPos.x != x || loaded.parentPos.y != y ||
          loaded.cost != cost) return 2;
    }
  return 0;
}
