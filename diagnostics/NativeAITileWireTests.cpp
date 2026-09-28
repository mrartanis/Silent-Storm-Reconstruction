#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiGrid.h"

#include <cstddef>
#include <cmath>
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
  using LadderCodec = S2FileIO::StructureFieldCodec<NAI::CNodesLayer::SLadderTransition>;
  if (!LadderCodec::kPortable || LadderCodec::kWireSize != 3) return 1;
  NAI::CNodesLayer::SLadderTransition ladder{true, 0, 0};
  const std::uint8_t ladderIndex = 0x80, layerGroup = 0xfe;
  std::memcpy(&ladder.nLadder, &ladderIndex, 1);
  std::memcpy(&ladder.nLayerGroup, &layerGroup, 1);
  std::uint8_t ladderWire[3] = {};
  if (!LadderCodec::Encode(ladder, ladderWire, sizeof(ladderWire)) ||
      ladderWire[0] != 1 || ladderWire[1] != ladderIndex ||
      ladderWire[2] != layerGroup) return 1;
  NAI::CNodesLayer::SLadderTransition decodedLadder{};
  ladderWire[0] = 2; // legacy nonzero bool values normalize on decode.
  if (!LadderCodec::Decode(ladderWire, sizeof(ladderWire), &decodedLadder) ||
      !decodedLadder.bUpper ||
      std::memcmp(&decodedLadder.nLadder, &ladder.nLadder, 1) != 0 ||
      std::memcmp(&decodedLadder.nLayerGroup, &ladder.nLayerGroup, 1) != 0 ||
      LadderCodec::Decode(ladderWire, 2, &decodedLadder) ||
      LadderCodec::Encode(ladder, ladderWire, 2)) return 1;
  using IgnoreCodec = S2FileIO::StructureFieldCodec<NAI::CLayersGroup::SIgnoreRect>;
  if (!IgnoreCodec::kPortable || IgnoreCodec::kWireSize != 24) return 1;
  NAI::CLayersGroup::SIgnoreRect ignored(CVec2(1.0f, 2.0f),
    CVec2(3.0f, 4.0f), CVec2(5.0f, -0.0f));
  NAI::CLayersGroup::SIgnoreRect decodedIgnored;
  std::uint8_t ignoredWire[24] = {};
  if (!IgnoreCodec::Encode(ignored, ignoredWire, sizeof(ignoredWire)) ||
      ignoredWire[0] != 0 || ignoredWire[1] != 0 ||
      ignoredWire[2] != 0x80 || ignoredWire[3] != 0x3f ||
      ignoredWire[23] != 0x80 ||
      !IgnoreCodec::Decode(ignoredWire, sizeof(ignoredWire), &decodedIgnored) ||
      std::memcmp(&ignored, &decodedIgnored, sizeof(ignored)) != 0 ||
      !std::signbit(decodedIgnored.ptSize.y) ||
      IgnoreCodec::Decode(ignoredWire, 23, &decodedIgnored) ||
      IgnoreCodec::Encode(ignored, ignoredWire, 23)) return 1;
  return 0;
}
