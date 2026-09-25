#include "../FileIO/PortableFontWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::FontCharacterFields fields{
      -1, 2, 0x12345678, -3, 4, -5, 6};
  const std::uint8_t expected[28] = {
      0xff, 0xff, 0xff, 0xff, 0x02, 0, 0, 0,
      0x78, 0x56, 0x34, 0x12, 0xfd, 0xff, 0xff, 0xff,
      0x04, 0, 0, 0, 0xfb, 0xff, 0xff, 0xff, 0x06, 0, 0, 0};
  std::uint8_t wire[28] = {};
  S2FileIO::FontCharacterFields decoded{};
  if (!S2FileIO::EncodeFontCharacter(fields, wire, sizeof(wire)) ||
      std::memcmp(wire, expected, sizeof(wire)) != 0 ||
      !S2FileIO::DecodeFontCharacter(expected, sizeof(expected), &decoded) ||
      decoded.x1 != -1 || decoded.y1 != 2 || decoded.x2 != 0x12345678 ||
      decoded.y2 != -3 || decoded.advanceA != 4 ||
      decoded.advanceBC != -5 || decoded.width != 6 ||
      S2FileIO::DecodeFontCharacter(expected, 27, &decoded) ||
      S2FileIO::EncodeFontCharacter(fields, wire, 27) ||
      S2FileIO::DecodeFontCharacter(nullptr, 28, &decoded) ||
      S2FileIO::EncodeFontCharacter(fields, nullptr, 28)) return 1;
  return 0;
}
