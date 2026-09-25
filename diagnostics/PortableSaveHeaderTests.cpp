#include "../FileIO/PortableSaveHeader.h"

#include <cstdint>
#include <vector>

int main() {
  S2FileIO::SaveHeaderData input;
  input.magic = static_cast<std::int32_t>(0x828ca022u);
  input.activeMods = 3;
  input.screenshot.assign(S2FileIO::kSaveScreenshotPixels,
                          S2FileIO::Pixel8888Channels{1, 2, 3, 4});
  input.screenshot[0] = {0x12, 0x34, 0x56, 0x78};
  input.screenshot.back() = {0xfe, 0xdc, 0xba, 0x98};
  std::vector<std::uint8_t> wire(S2FileIO::kSaveHeaderWireSize);
  if (!S2FileIO::EncodeSaveHeader(input, wire.data(), wire.size())) return 1;
  const std::uint8_t expected[] = {0x22, 0xa0, 0x8c, 0x82, 3, 0, 0, 0,
                                   0x56, 0x34, 0x12, 0x78};
  for (std::size_t i = 0; i < sizeof(expected); ++i)
    if (wire[i] != expected[i]) return 2;
  const std::size_t last = wire.size() - 4;
  if (wire[last] != 0xba || wire[last + 1] != 0xdc ||
      wire[last + 2] != 0xfe || wire[last + 3] != 0x98) return 3;
  S2FileIO::SaveHeaderData decoded;
  if (!S2FileIO::DecodeSaveHeader(wire.data(), wire.size(), &decoded) ||
      decoded.magic != input.magic || decoded.activeMods != input.activeMods ||
      decoded.screenshot.size() != S2FileIO::kSaveScreenshotPixels) return 4;
  for (std::size_t i = 0; i < input.screenshot.size(); ++i)
    if (decoded.screenshot[i].red != input.screenshot[i].red ||
        decoded.screenshot[i].green != input.screenshot[i].green ||
        decoded.screenshot[i].blue != input.screenshot[i].blue ||
        decoded.screenshot[i].alpha != input.screenshot[i].alpha) return 5;
  if (S2FileIO::DecodeSaveHeader(wire.data(), wire.size() - 1, &decoded) ||
      S2FileIO::EncodeSaveHeader(input, wire.data(), wire.size() - 1) ||
      S2FileIO::DecodeSaveHeader(nullptr, wire.size(), &decoded)) return 6;
  input.activeMods = -1;
  if (S2FileIO::EncodeSaveHeader(input, wire.data(), wire.size())) return 7;
  wire[4] = 0xff;
  wire[5] = 0xff;
  wire[6] = 0xff;
  wire[7] = 0xff;
  if (S2FileIO::DecodeSaveHeader(wire.data(), wire.size(), &decoded)) return 8;
  return 0;
}
