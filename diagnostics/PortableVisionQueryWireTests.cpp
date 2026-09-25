#include "../FileIO/PortableVisionQueryWire.h"

#include <cmath>
#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::VisionQueryFields query{{1.0f, -2.0f, 0.5f},
      {0.0f, -0.0f, 3.0f}, 40.0f, -1.0f};
  const std::uint8_t expected[32] = {
      0, 0, 0x80, 0x3f, 0, 0, 0, 0xc0,
      0, 0, 0, 0x3f, 0, 0, 0, 0,
      0, 0, 0, 0x80, 0, 0, 0x40, 0x40,
      0, 0, 0x20, 0x42, 0, 0, 0x80, 0xbf};
  std::uint8_t wire[32] = {};
  S2FileIO::VisionQueryFields decoded{};
  if (!S2FileIO::EncodeVisionQuery(query, wire, sizeof(wire)) ||
      std::memcmp(wire, expected, sizeof(wire)) != 0 ||
      !S2FileIO::DecodeVisionQuery(expected, sizeof(expected), &decoded) ||
      decoded.from[0] != 1.0f || decoded.from[1] != -2.0f ||
      decoded.from[2] != 0.5f || decoded.target[0] != 0.0f ||
      !std::signbit(decoded.target[1]) || decoded.target[2] != 3.0f ||
      decoded.range != 40.0f || decoded.cosFov != -1.0f ||
      S2FileIO::DecodeVisionQuery(expected, 31, &decoded) ||
      S2FileIO::EncodeVisionQuery(query, wire, 31)) return 1;
  return 0;
}
