#include "../FileIO/PortableSquareMoveWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::SquareMoveFields cases[] = {
      {0, 0, 0}, {255, 255, 65535}, {42, 193, 0x1234}};
  const std::uint8_t expected[][4] = {
      {0, 0, 0, 0}, {255, 255, 255, 255}, {42, 193, 0x34, 0x12}};
  for (unsigned i = 0; i < 3; ++i) {
    std::uint8_t wire[4] = {};
    S2FileIO::SquareMoveFields decoded{};
    if (!S2FileIO::EncodeSquareMove(cases[i], wire, 4) ||
        std::memcmp(wire, expected[i], 4) != 0 ||
        !S2FileIO::DecodeSquareMove(wire, 4, &decoded) ||
        decoded.parentX != cases[i].parentX ||
        decoded.parentY != cases[i].parentY ||
        decoded.cost != cases[i].cost) return 1;
  }
  std::uint8_t wire[4] = {};
  S2FileIO::SquareMoveFields decoded{};
  if (S2FileIO::DecodeSquareMove(wire, 3, &decoded) ||
      S2FileIO::DecodeSquareMove(nullptr, 4, &decoded) ||
      S2FileIO::DecodeSquareMove(wire, 4, nullptr) ||
      S2FileIO::EncodeSquareMove(cases[0], wire, 3) ||
      S2FileIO::EncodeSquareMove(cases[0], nullptr, 4)) return 2;
  return 0;
}
