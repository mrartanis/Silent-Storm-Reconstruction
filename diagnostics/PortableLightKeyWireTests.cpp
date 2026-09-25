#include "../FileIO/PortableLightKeyWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const float vector[3] = {1.0f, -2.0f, 0.5f};
  const std::uint8_t expected[14] = {
      0x34, 0x12, 0x00, 0x00, 0x80, 0x3f, 0x00,
      0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x3f};
  std::uint8_t wire[14] = {};
  if (!S2FileIO::EncodeLightKey<3>(0x1234, vector, wire, 14) ||
      std::memcmp(wire, expected, 14) != 0) return 1;
  std::int16_t frame = 0;
  float loaded[3] = {};
  if (!S2FileIO::DecodeLightKey<3>(wire, 14, &frame, loaded) ||
      frame != 0x1234 || std::memcmp(vector, loaded, sizeof(vector)) != 0)
    return 2;
  const float scalar = -0.25f;
  const std::uint8_t scalarExpected[6] = {
      0xfe, 0xff, 0x00, 0x00, 0x80, 0xbe};
  if (!S2FileIO::EncodeLightKey<1>(-2, &scalar, wire, 6) ||
      std::memcmp(wire, scalarExpected, 6) != 0 ||
      !S2FileIO::DecodeLightKey<1>(wire, 6, &frame, loaded) ||
      frame != -2 || loaded[0] != scalar) return 3;
  if (S2FileIO::DecodeLightKey<3>(wire, 13, &frame, loaded) ||
      S2FileIO::DecodeLightKey<3>(nullptr, 14, &frame, loaded) ||
      S2FileIO::DecodeLightKey<3>(wire, 14, nullptr, loaded) ||
      S2FileIO::EncodeLightKey<3>(0, vector, wire, 13) ||
      S2FileIO::EncodeLightKey<3>(0, vector, nullptr, 14)) return 4;
  return 0;
}
