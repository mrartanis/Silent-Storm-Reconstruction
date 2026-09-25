#include "../FileIO/PortableLuaStateWire.h"

#include <cstdint>
#include <cstring>
#include <initializer_list>

int main() {
  const std::int32_t values[15] =
      {-17, 0x12345678, 0, 1, -1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  std::uint8_t wire[60] = {};
  std::int32_t loaded[15] = {};
  const std::uint8_t firstTwo[8] =
      {0xef, 0xff, 0xff, 0xff, 0x78, 0x56, 0x34, 0x12};
  for (std::size_t count : {std::size_t(5), std::size_t(15)}) {
    if (!S2FileIO::EncodeLuaStateIntegers(values, count, wire, count * 4) ||
        std::memcmp(wire, firstTwo, 8) != 0 ||
        !S2FileIO::DecodeLuaStateIntegers(wire, count * 4, loaded, count) ||
        std::memcmp(values, loaded, count * 4) != 0) return 1;
  }
  if (S2FileIO::DecodeLuaStateIntegers(wire, 59, loaded, 15) ||
      S2FileIO::DecodeLuaStateIntegers(nullptr, 60, loaded, 15) ||
      S2FileIO::DecodeLuaStateIntegers(wire, 60, nullptr, 15) ||
      S2FileIO::EncodeLuaStateIntegers(values, 15, nullptr, 60) ||
      S2FileIO::EncodeLuaStateIntegers(values, 16, wire, 64)) return 2;
  return 0;
}
