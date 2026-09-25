#include "../Misc/PortableBuildingPart.h"

#include <cstdint>
#include <cstring>

int main() {
  const int floors[] = {-32, -16, -1, 0, 1, 31};
  const int coords[] = {-4096, -1, 0, 1, 4095};
  for (int floor : floors) for (int x : coords) for (int y : coords) {
    const std::uint32_t bits = S2Building::MakePart(floor, x, y);
    if (S2Building::PartFloor(bits) != floor ||
        S2Building::PartX(bits) != x || S2Building::PartY(bits) != y)
      return 1;
    std::uint8_t wire[4] = {};
    std::uint32_t decoded = 0;
    if (!S2Building::EncodePart(bits, wire, 4) ||
        !S2Building::DecodePart(wire, 4, &decoded) || decoded != bits)
      return 2;
  }
  const std::uint32_t bits = S2Building::MakePart(-1, 1, 2);
  const std::uint8_t expected[] = {0x7f, 0x00, 0x10, 0x00};
  std::uint8_t wire[4] = {};
  std::uint32_t decoded = 0;
  if (!S2Building::EncodePart(bits, wire, 4) ||
      std::memcmp(wire, expected, 4) != 0 ||
      S2Building::DecodePart(wire, 3, &decoded) ||
      S2Building::DecodePart(nullptr, 4, &decoded) ||
      S2Building::DecodePart(wire, 4, nullptr) ||
      S2Building::EncodePart(bits, wire, 3) ||
      S2Building::EncodePart(bits, nullptr, 4)) return 3;
  return 0;
}
