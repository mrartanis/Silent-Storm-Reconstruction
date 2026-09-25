#include "../Main/StdAfx.h"
#include "../Main/aiGrid.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

static_assert(offsetof(NAI::STile, nHeight) == 4 &&
              offsetof(NAI::STile, nFloor) == 6 &&
              offsetof(NAI::STile, nLocks) == 8 &&
              offsetof(NAI::STile, nFlipper) == 13 &&
              offsetof(NAI::STile, nFake2) == 15,
              "AI tile field offsets");
static_assert(offsetof(NAI::CNodesLayer::SLink, dst) == 0 &&
              offsetof(NAI::CNodesLayer::SLink, nHeight) == 4,
              "AI link field offsets");

int main() {
  using TileCodec = S2FileIO::StructureFieldCodec<NAI::STile>;
  using LinkCodec = S2FileIO::StructureFieldCodec<NAI::CNodesLayer::SLink>;
  if (!TileCodec::kPortable || TileCodec::kWireSize != 16 ||
      !LinkCodec::kPortable || LinkCodec::kWireSize != 8) return 1;
  NAI::STile tile{};
  tile.nMove[0] = static_cast<char>(0x80);
  tile.nMove[1] = 1; tile.nMove[2] = 2; tile.nMove[3] = 3;
  tile.nHeight = 0x1234; tile.nFloor = -2;
  tile.nLocks = 4; tile.nDynLocks = 5; tile.nPassable = 6;
  tile.nDisplacement = 7; tile.nFlags = 8; tile.nFlipper = 9;
  tile.nFake1 = 10; tile.nFake2 = 11;
  std::uint8_t tileWire[16] = {};
  NAI::STile decodedTile{};
  if (!TileCodec::Encode(tile, tileWire, sizeof(tileWire)) ||
      std::memcmp(tileWire, &tile, sizeof(tileWire)) != 0 ||
      !TileCodec::Decode(tileWire, sizeof(tileWire), &decodedTile) ||
      std::memcmp(&tile, &decodedTile, sizeof(tile)) != 0 ||
      TileCodec::Decode(tileWire, 15, &decodedTile) ||
      TileCodec::Encode(tile, tileWire, 15)) return 1;
  NAI::CNodesLayer::SLink link(NAI::SPathPlace::FromBits(0x12345678u), -9);
  NAI::CNodesLayer::SLink decodedLink;
  std::uint8_t linkWire[8] = {};
  if (!LinkCodec::Encode(link, linkWire, sizeof(linkWire)) ||
      std::memcmp(linkWire, &link, sizeof(linkWire)) != 0 ||
      !LinkCodec::Decode(linkWire, sizeof(linkWire), &decodedLink) ||
      decodedLink.dst.GetBits() != link.dst.GetBits() ||
      decodedLink.nHeight != link.nHeight ||
      LinkCodec::Decode(linkWire, 7, &decodedLink) ||
      LinkCodec::Encode(link, linkWire, 7)) return 1;
  return 0;
}
