#include "../Misc/PortableZone.h"

#include <cstdint>
#include <cstring>

int main() {
  const std::uint8_t historical[8] = {0x34, 0x12, 0xaa, 0xbb,
                                      0xfe, 0xff, 0xff, 0xff};
  std::uint16_t color = 0;
  std::int32_t layer = 0;
  if (!S2AI::DecodeZone(historical, 8, &color, &layer) ||
      color != 0x1234 || layer != -2) return 1;
  const std::uint8_t canonical[8] = {0x34, 0x12, 0, 0,
                                     0xfe, 0xff, 0xff, 0xff};
  std::uint8_t encoded[8] = {};
  if (!S2AI::EncodeZone(color, layer, encoded, 8) ||
      std::memcmp(encoded, canonical, 8) != 0) return 2;
  const std::uint16_t colors[] = {0, 1, 0x7fff, 0xffff};
  const std::int32_t layers[] = {0, 1, -1, 255, -256, INT32_MIN, INT32_MAX};
  for (std::uint16_t c : colors) for (std::int32_t l : layers) {
    std::uint8_t wire[8] = {};
    std::uint16_t decodedColor = 0;
    std::int32_t decodedLayer = 0;
    if (!S2AI::EncodeZone(c, l, wire, 8) ||
        !S2AI::DecodeZone(wire, 8, &decodedColor, &decodedLayer) ||
        decodedColor != c || decodedLayer != l || wire[2] || wire[3]) return 3;
  }
  if (S2AI::DecodeZone(historical, 7, &color, &layer) ||
      S2AI::DecodeZone(nullptr, 8, &color, &layer) ||
      S2AI::EncodeZone(color, layer, encoded, 7) ||
      S2AI::EncodeZone(color, layer, nullptr, 8)) return 4;
  return 0;
}
