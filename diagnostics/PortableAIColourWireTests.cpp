#include "../FileIO/PortableAIColourWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::NeighbourFields neighbour{0x1234, 0xabcd, -2};
  const std::uint8_t expectedNeighbour[8] = {
      0x34, 0x12, 0xcd, 0xab, 0xfe, 0xff, 0xff, 0xff};
  std::uint8_t wireNeighbour[8] = {};
  S2FileIO::NeighbourFields decodedNeighbour{};
  if (!S2FileIO::EncodeNeighbour(neighbour, wireNeighbour, sizeof(wireNeighbour)) ||
      std::memcmp(wireNeighbour, expectedNeighbour, sizeof(wireNeighbour)) != 0 ||
      !S2FileIO::DecodeNeighbour(expectedNeighbour, sizeof(expectedNeighbour),
                                 &decodedNeighbour) ||
      decodedNeighbour.node != 0x1234 ||
      decodedNeighbour.distance != 0xabcd || decodedNeighbour.layer != -2 ||
      S2FileIO::DecodeNeighbour(expectedNeighbour, 7, &decodedNeighbour) ||
      S2FileIO::EncodeNeighbour(neighbour, wireNeighbour, 7)) return 1;

  const S2FileIO::LocalColorFields color{0x1234, 0xabcd};
  const std::uint8_t expectedColor[4] = {0x34, 0x12, 0xcd, 0xab};
  std::uint8_t wireColor[4] = {};
  S2FileIO::LocalColorFields decodedColor{};
  if (!S2FileIO::EncodeLocalColor(color, wireColor, sizeof(wireColor)) ||
      std::memcmp(wireColor, expectedColor, sizeof(wireColor)) != 0 ||
      !S2FileIO::DecodeLocalColor(expectedColor, sizeof(expectedColor),
                                 &decodedColor) ||
      decodedColor.averageX != 0x1234 || decodedColor.averageY != 0xabcd ||
      S2FileIO::DecodeLocalColor(expectedColor, 3, &decodedColor) ||
      S2FileIO::EncodeLocalColor(color, wireColor, 3)) return 2;
  return 0;
}
