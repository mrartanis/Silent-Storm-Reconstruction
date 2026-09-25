#ifndef S2_FILEIO_PORTABLE_SAVE_HEADER_H
#define S2_FILEIO_PORTABLE_SAVE_HEADER_H

#include "PortablePixel8888Wire.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace S2FileIO {

constexpr std::size_t kSaveScreenshotWidth = 320;
constexpr std::size_t kSaveScreenshotHeight = 200;
constexpr std::size_t kSaveScreenshotPixels =
    kSaveScreenshotWidth * kSaveScreenshotHeight;
constexpr std::size_t kSaveHeaderWireSize = 8 + kSaveScreenshotPixels * 4;

struct SaveHeaderData {
  std::int32_t magic = 0;
  std::int32_t activeMods = 0;
  std::vector<Pixel8888Channels> screenshot;
};

inline bool DecodeSaveHeader(const std::uint8_t* source, std::size_t length,
                             SaveHeaderData* output) {
  if (!source || !output || length != kSaveHeaderWireSize) return false;
  SaveHeaderData decoded;
  if (!DecodeStructureScalar(source, 4, &decoded.magic) ||
      !DecodeStructureScalar(source + 4, 4, &decoded.activeMods) ||
      decoded.activeMods < 0) return false;
  decoded.screenshot.resize(kSaveScreenshotPixels);
  for (std::size_t i = 0; i < decoded.screenshot.size(); ++i)
    if (!DecodePixel8888(source + 8 + i * 4, 4, &decoded.screenshot[i]))
      return false;
  *output = std::move(decoded);
  return true;
}

inline bool EncodeSaveHeader(const SaveHeaderData& value,
                             std::uint8_t* destination,
                             std::size_t length) {
  if (!destination || length != kSaveHeaderWireSize ||
      value.activeMods < 0 || value.screenshot.size() != kSaveScreenshotPixels ||
      !EncodeStructureScalar(value.magic, destination, 4) ||
      !EncodeStructureScalar(value.activeMods, destination + 4, 4)) return false;
  for (std::size_t i = 0; i < value.screenshot.size(); ++i)
    if (!EncodePixel8888(value.screenshot[i], destination + 8 + i * 4, 4))
      return false;
  return true;
}

} // namespace S2FileIO

#endif
