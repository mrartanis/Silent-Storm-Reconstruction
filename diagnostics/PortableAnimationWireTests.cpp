#include "../FileIO/PortableAnimationWire.h"

#include <cstdint>
#include <cstring>
#include <initializer_list>

int main() {
  const float fields[10] = {1.0f, -2.5f, 0.0f, 0.25f, -1.0f,
                            3.0f, 10.0f, 0.5f, -4.0f, 2.0f};
  for (std::size_t count : {std::size_t(4), std::size_t(7), std::size_t(10)}) {
    std::uint8_t wire[40] = {};
    float loaded[10] = {};
    if (!S2FileIO::EncodeAnimationFloats(fields, count, wire, count * 4) ||
        !S2FileIO::DecodeAnimationFloats(wire, count * 4, loaded, count) ||
        std::memcmp(fields, loaded, count * 4) != 0) return 1;
  }
  const std::uint8_t firstTwo[] = {0, 0, 0x80, 0x3f, 0, 0, 0x20, 0xc0};
  std::uint8_t wire[44] = {};
  if (!S2FileIO::EncodeAnimationFloats(fields, 10, wire, 40) ||
      std::memcmp(wire, firstTwo, 8) != 0) return 2;
  for (std::size_t count : {std::size_t(7), std::size_t(10)}) {
    float loaded[10] = {};
    std::int32_t parent = 0;
    if (!S2FileIO::EncodeAnimationParentFloats(-17, fields, count,
                                                wire, 4 + count * 4) ||
        !S2FileIO::DecodeAnimationParentFloats(wire, 4 + count * 4,
                                                &parent, loaded, count) ||
        parent != -17 || std::memcmp(fields, loaded, count * 4) != 0 ||
        wire[0] != 0xef || wire[1] != 0xff || wire[2] != 0xff ||
        wire[3] != 0xff || std::memcmp(wire + 4, firstTwo, 8) != 0)
      return 3;
  }
  float loaded[10] = {};
  std::int32_t parent = 0;
  if (S2FileIO::DecodeAnimationFloats(wire, 39, loaded, 10) ||
      S2FileIO::DecodeAnimationFloats(nullptr, 40, loaded, 10) ||
      S2FileIO::EncodeAnimationFloats(fields, 10, nullptr, 40) ||
      S2FileIO::DecodeAnimationParentFloats(wire, 43, &parent, loaded, 10) ||
      S2FileIO::DecodeAnimationParentFloats(wire, 44, nullptr, loaded, 10) ||
      S2FileIO::EncodeAnimationParentFloats(0, fields, 10, wire, 43) ||
      S2FileIO::EncodeAnimationParentFloats(0, nullptr, 10, wire, 44))
    return 4;
  return 0;
}
