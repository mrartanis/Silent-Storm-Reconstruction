#include "../FileIO/PortableFBTransformWire.h"

#include <cstdint>
#include <cstring>

int main() {
  S2FileIO::FBTransformFields source{};
  for (int i = 0; i < 16; ++i) {
    source.forward[i] = static_cast<float>(i + 1);
    source.backward[i] = static_cast<float>(-i - 1);
  }
  source.forward[5] = -0.0f;
  std::uint8_t wire[128] = {};
  S2FileIO::FBTransformFields decoded{};
  const std::uint8_t one[4] = {0, 0, 0x80, 0x3f};
  const std::uint8_t minusZero[4] = {0, 0, 0, 0x80};
  const std::uint8_t minusOne[4] = {0, 0, 0x80, 0xbf};
  const std::uint8_t minusSixteen[4] = {0, 0, 0x80, 0xc1};
  if (!S2FileIO::EncodeFBTransform(source, wire, sizeof(wire)) ||
      std::memcmp(wire, one, 4) != 0 ||
      std::memcmp(wire + 20, minusZero, 4) != 0 ||
      std::memcmp(wire + 64, minusOne, 4) != 0 ||
      std::memcmp(wire + 124, minusSixteen, 4) != 0 ||
      !S2FileIO::DecodeFBTransform(wire, sizeof(wire), &decoded) ||
      std::memcmp(&source, &decoded, sizeof(source)) != 0 ||
      S2FileIO::DecodeFBTransform(wire, 127, &decoded) ||
      S2FileIO::EncodeFBTransform(source, wire, 127) ||
      S2FileIO::DecodeFBTransform(nullptr, 128, &decoded) ||
      S2FileIO::EncodeFBTransform(source, nullptr, 128)) return 1;
  return 0;
}
