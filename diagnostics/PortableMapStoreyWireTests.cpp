#include "../FileIO/PortableMapStoreyWire.h"

#include <cstdint>
#include <cstring>
#include <limits>

int main() {
  const S2FileIO::MapStoreyFields cases[] = {
      {0, 0}, {-3, -1}, {7, 12},
      {std::numeric_limits<std::int32_t>::min(),
       std::numeric_limits<std::int32_t>::max()}};
  const std::uint8_t expected[][8] = {
      {0}, {0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
      {7, 0, 0, 0, 12, 0, 0, 0},
      {0, 0, 0, 0x80, 0xff, 0xff, 0xff, 0x7f}};
  for (unsigned i = 0; i < 4; ++i) {
    std::uint8_t wire[8] = {};
    S2FileIO::MapStoreyFields decoded{};
    if (!S2FileIO::EncodeMapStorey(cases[i], wire, 8) ||
        std::memcmp(wire, expected[i], 8) != 0 ||
        !S2FileIO::DecodeMapStorey(wire, 8, &decoded) ||
        decoded.localFloor != cases[i].localFloor ||
        decoded.globalFloor != cases[i].globalFloor) return 1;
  }
  std::uint8_t wire[8] = {};
  S2FileIO::MapStoreyFields decoded{};
  if (S2FileIO::DecodeMapStorey(wire, 7, &decoded) ||
      S2FileIO::DecodeMapStorey(nullptr, 8, &decoded) ||
      S2FileIO::DecodeMapStorey(wire, 8, nullptr) ||
      S2FileIO::EncodeMapStorey(cases[0], wire, 7) ||
      S2FileIO::EncodeMapStorey(cases[0], nullptr, 8)) return 2;
  return 0;
}
