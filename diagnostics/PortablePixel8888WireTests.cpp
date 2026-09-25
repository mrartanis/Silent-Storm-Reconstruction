#include "../FileIO/PortablePixel8888Wire.h"

#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::Pixel8888Channels pixels[] = {
      {0x12, 0x34, 0x56, 0x78}, {0, 0, 0, 0},
      {0xff, 0xff, 0xff, 0xff}, {0x80, 0x01, 0xfe, 0x00}};
  const std::uint8_t expected[][4] = {
      {0x56, 0x34, 0x12, 0x78}, {0, 0, 0, 0},
      {0xff, 0xff, 0xff, 0xff}, {0xfe, 0x01, 0x80, 0x00}};
  for (unsigned i = 0; i < 4; ++i) {
    std::uint8_t wire[4] = {};
    S2FileIO::Pixel8888Channels decoded{};
    if (!S2FileIO::EncodePixel8888(pixels[i], wire, sizeof(wire)) ||
        std::memcmp(wire, expected[i], sizeof(wire)) != 0 ||
        !S2FileIO::DecodePixel8888(wire, sizeof(wire), &decoded) ||
        std::memcmp(&decoded, &pixels[i], sizeof(decoded)) != 0) return 1;
  }
  std::uint8_t wire[4] = {};
  S2FileIO::Pixel8888Channels decoded{};
  if (S2FileIO::DecodePixel8888(wire, 3, &decoded) ||
      S2FileIO::EncodePixel8888(pixels[0], wire, 3) ||
      S2FileIO::DecodePixel8888(nullptr, 4, &decoded) ||
      S2FileIO::EncodePixel8888(pixels[0], nullptr, 4)) return 2;
  return 0;
}
