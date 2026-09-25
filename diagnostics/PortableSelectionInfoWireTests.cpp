#include "../FileIO/PortableSelectionInfoWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::SelectionInfoFields value{{1.0f, -2.0f, 0.5f, 0.0f}, true};
  const std::uint8_t expected[20] = {
      0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0xc0,
      0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00};
  std::uint8_t wire[20] = {};
  if (!S2FileIO::EncodeSelectionInfo(value, wire, sizeof(wire)) ||
      std::memcmp(wire, expected, sizeof(wire)) != 0) return 1;
  S2FileIO::SelectionInfoFields loaded{};
  if (!S2FileIO::DecodeSelectionInfo(wire, sizeof(wire), &loaded) ||
      std::memcmp(loaded.color, value.color, sizeof(value.color)) != 0 ||
      !loaded.ignoreFloorMask) return 2;
  // Old saves may carry arbitrary padding and any nonzero true byte.
  wire[16] = 0xff;
  wire[17] = 0xaa;
  if (!S2FileIO::DecodeSelectionInfo(wire, sizeof(wire), &loaded) ||
      !loaded.ignoreFloorMask) return 3;
  if (S2FileIO::DecodeSelectionInfo(wire, 19, &loaded) ||
      S2FileIO::DecodeSelectionInfo(nullptr, 20, &loaded) ||
      S2FileIO::DecodeSelectionInfo(wire, 20, nullptr) ||
      S2FileIO::EncodeSelectionInfo(value, nullptr, 20) ||
      S2FileIO::EncodeSelectionInfo(value, wire, 19)) return 4;
  return 0;
}
