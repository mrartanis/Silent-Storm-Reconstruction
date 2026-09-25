#include "../FileIO/PortableAITileWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::AITileFields tile{{0x80, 0x01, 0xff, 0x40},
      0x1234, -2, 3, 4, 0x80, 0xff, 5, 6, 7, 8};
  const std::uint8_t expectedTile[16] = {
      0x80, 0x01, 0xff, 0x40, 0x34, 0x12, 0xfe, 0xff,
      3, 4, 0x80, 0xff, 5, 6, 7, 8};
  std::uint8_t wireTile[16] = {};
  S2FileIO::AITileFields decodedTile{};
  if (!S2FileIO::EncodeAITile(tile, wireTile, sizeof(wireTile)) ||
      std::memcmp(wireTile, expectedTile, sizeof(wireTile)) != 0 ||
      !S2FileIO::DecodeAITile(expectedTile, sizeof(expectedTile), &decodedTile) ||
      decodedTile.move[0] != 0x80 || decodedTile.move[2] != 0xff ||
      decodedTile.height != 0x1234 || decodedTile.floor != -2 ||
      decodedTile.passable != 0x80 || decodedTile.displacement != 0xff ||
      decodedTile.reserved2 != 8 ||
      S2FileIO::DecodeAITile(expectedTile, 15, &decodedTile) ||
      S2FileIO::EncodeAITile(tile, wireTile, 15)) return 1;

  const S2FileIO::AILinkFields link{0x12345678u, -9};
  const std::uint8_t expectedLink[8] = {
      0x78, 0x56, 0x34, 0x12, 0xf7, 0xff, 0xff, 0xff};
  std::uint8_t wireLink[8] = {};
  S2FileIO::AILinkFields decodedLink{};
  if (!S2FileIO::EncodeAILink(link, wireLink, sizeof(wireLink)) ||
      std::memcmp(wireLink, expectedLink, sizeof(wireLink)) != 0 ||
      !S2FileIO::DecodeAILink(expectedLink, sizeof(expectedLink), &decodedLink) ||
      decodedLink.destination != 0x12345678u || decodedLink.height != -9 ||
      S2FileIO::DecodeAILink(expectedLink, 7, &decodedLink) ||
      S2FileIO::EncodeAILink(link, wireLink, 7)) return 1;
  return 0;
}
