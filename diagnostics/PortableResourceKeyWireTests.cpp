#include "../FileIO/PortableResourceKeyWire.h"

#include <cstdint>
#include <cstring>
#include <limits>

int main() {
  const S2FileIO::ResourceKeyFields values[] = {
      {0, 0}, {0x12345678, 3}, {-1, -2},
      {std::numeric_limits<std::int32_t>::min(),
       std::numeric_limits<std::int32_t>::max()}};
  const std::uint8_t expected[][8] = {
      {0, 0, 0, 0, 0, 0, 0, 0},
      {0x78, 0x56, 0x34, 0x12, 3, 0, 0, 0},
      {0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff},
      {0, 0, 0, 0x80, 0xff, 0xff, 0xff, 0x7f}};
  for (unsigned i = 0; i < 4; ++i) {
    std::uint8_t wire[8] = {};
    S2FileIO::ResourceKeyFields decoded{};
    if (!S2FileIO::EncodeResourceKey(values[i], wire, sizeof(wire)) ||
        std::memcmp(wire, expected[i], sizeof(wire)) != 0 ||
        !S2FileIO::DecodeResourceKey(expected[i], sizeof(wire), &decoded) ||
        decoded.id != values[i].id || decoded.option != values[i].option)
      return 1;
  }
  S2FileIO::ResourceKeyFields decoded{};
  std::uint8_t wire[8] = {};
  if (S2FileIO::DecodeResourceKey(wire, 7, &decoded) ||
      S2FileIO::EncodeResourceKey(values[0], wire, 7) ||
      S2FileIO::DecodeResourceKey(nullptr, 8, &decoded) ||
      S2FileIO::EncodeResourceKey(values[0], nullptr, 8)) return 2;
  return 0;
}
