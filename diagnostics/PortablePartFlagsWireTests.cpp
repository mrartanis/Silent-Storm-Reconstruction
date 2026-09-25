#include "../FileIO/PortablePartFlagsWire.h"

#include <cstdint>
#include <cstring>
#include <limits>

int main() {
  const S2FileIO::PartFlagsFields values[] = {
      {{0, 0, 0, 0, 0, 0, 0, 0}},
      {{0x12345678, -1, 0, -2, 0x10203040, 1, -3, 0x76543210}},
      {{std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max(), 0, 0, 0, 0, 0, 0}}};
  const std::uint8_t expected[][32] = {
      {0},
      {0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff,
       0, 0, 0, 0, 0xfe, 0xff, 0xff, 0xff,
       0x40, 0x30, 0x20, 0x10, 1, 0, 0, 0,
       0xfd, 0xff, 0xff, 0xff, 0x10, 0x32, 0x54, 0x76},
      {0, 0, 0, 0x80, 0xff, 0xff, 0xff, 0x7f}};
  for (unsigned i = 0; i < 3; ++i) {
    std::uint8_t wire[32] = {};
    S2FileIO::PartFlagsFields decoded{};
    if (!S2FileIO::EncodePartFlags(values[i], wire, sizeof(wire)) ||
        std::memcmp(wire, expected[i], sizeof(wire)) != 0 ||
        !S2FileIO::DecodePartFlags(expected[i], sizeof(wire), &decoded) ||
        std::memcmp(&values[i], &decoded, sizeof(decoded)) != 0) return 1;
  }
  S2FileIO::PartFlagsFields decoded{};
  std::uint8_t wire[32] = {};
  if (S2FileIO::DecodePartFlags(wire, 31, &decoded) ||
      S2FileIO::EncodePartFlags(values[0], wire, 31) ||
      S2FileIO::DecodePartFlags(nullptr, 32, &decoded) ||
      S2FileIO::DecodePartFlags(wire, 32, nullptr) ||
      S2FileIO::EncodePartFlags(values[0], nullptr, 32)) return 2;
  return 0;
}
