#include "../FileIO/PortableObjectPlace.h"

#include <cmath>
#include <cstdint>
#include <cstring>

int main() {
  S2FileIO::ObjectPlaceFields value{{1.0f, -2.0f, 0.5f},
                                     {3.0f, 4.0f, -0.0f}, 1.5f, -3};
  const std::uint8_t expected[32] = {
      0, 0, 0x80, 0x3f, 0, 0, 0, 0xc0, 0, 0, 0, 0x3f,
      0, 0, 0x40, 0x40, 0, 0, 0x80, 0x40, 0, 0, 0, 0x80,
      0, 0, 0xc0, 0x3f, 0xfd, 0xff, 0xff, 0xff};
  std::uint8_t encoded[32] = {};
  S2FileIO::ObjectPlaceFields decoded{};
  if (!S2FileIO::EncodeObjectPlace(value, encoded, 32) ||
      std::memcmp(encoded, expected, 32) != 0 ||
      !S2FileIO::DecodeObjectPlace(expected, 32, &decoded) ||
      decoded.position[0] != 1.0f || decoded.position[1] != -2.0f ||
      decoded.position[2] != 0.5f || decoded.scale[0] != 3.0f ||
      decoded.scale[1] != 4.0f || !std::signbit(decoded.scale[2]) ||
      decoded.angle != 1.5f || decoded.floor != -3) return 1;
  if (S2FileIO::DecodeObjectPlace(expected, 31, &decoded) ||
      S2FileIO::DecodeObjectPlace(nullptr, 32, &decoded) ||
      S2FileIO::EncodeObjectPlace(value, encoded, 31) ||
      S2FileIO::EncodeObjectPlace(value, nullptr, 32)) return 2;
  return 0;
}
