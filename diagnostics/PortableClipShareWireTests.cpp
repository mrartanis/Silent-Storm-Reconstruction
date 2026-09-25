#include "../FileIO/PortableClipShareWire.h"

#include <cstdint>
#include <cstring>
#include <limits>

int main() {
  const S2FileIO::ClipShareFields values[] = {
      {{0, 0}, 0, 0, 0},
      {{0x12345678, -2}, 0xaabbccddu, -3, 0x10203040},
      {{std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()},
       std::numeric_limits<std::uint32_t>::max(),
       std::numeric_limits<std::int32_t>::min(),
       std::numeric_limits<std::int32_t>::max()}};
  const std::uint8_t expected[][20] = {
      {0},
      {0x78, 0x56, 0x34, 0x12, 0xfe, 0xff, 0xff, 0xff,
       0xdd, 0xcc, 0xbb, 0xaa, 0xfd, 0xff, 0xff, 0xff,
       0x40, 0x30, 0x20, 0x10},
      {0, 0, 0, 0x80, 0xff, 0xff, 0xff, 0x7f,
       0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0x80,
       0xff, 0xff, 0xff, 0x7f}};
  for (unsigned i = 0; i < 3; ++i) {
    std::uint8_t wire[20] = {};
    S2FileIO::ClipShareFields decoded{};
    if (!S2FileIO::EncodeClipShare(values[i], wire, sizeof(wire)) ||
        std::memcmp(wire, expected[i], sizeof(wire)) != 0 ||
        !S2FileIO::DecodeClipShare(expected[i], sizeof(wire), &decoded) ||
        decoded.source.id != values[i].source.id ||
        decoded.source.option != values[i].source.option ||
        decoded.parts != values[i].parts ||
        decoded.subBlockId != values[i].subBlockId ||
        decoded.clip != values[i].clip) return 1;
  }
  S2FileIO::ClipShareFields decoded{};
  std::uint8_t wire[20] = {};
  if (S2FileIO::DecodeClipShare(wire, 19, &decoded) ||
      S2FileIO::EncodeClipShare(values[0], wire, 19) ||
      S2FileIO::DecodeClipShare(nullptr, 20, &decoded) ||
      S2FileIO::DecodeClipShare(wire, 20, nullptr) ||
      S2FileIO::EncodeClipShare(values[0], nullptr, 20)) return 2;
  return 0;
}
