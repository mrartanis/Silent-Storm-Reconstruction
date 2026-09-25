#ifndef S2_FILEIO_PORTABLE_PIXEL8888_WIRE_H
#define S2_FILEIO_PORTABLE_PIXEL8888_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct Pixel8888Channels {
  std::uint8_t red, green, blue, alpha;
};

// Legacy CF_A8R8G8B8 stores a pixel as B, G, R, A bytes on disk.
inline bool DecodePixel8888(const std::uint8_t* source, std::size_t length,
                            Pixel8888Channels* value) {
  if (!source || !value || length != 4) return false;
  *value = Pixel8888Channels{source[2], source[1], source[0], source[3]};
  return true;
}

inline bool EncodePixel8888(const Pixel8888Channels& value,
                            std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 4) return false;
  destination[0] = value.blue;
  destination[1] = value.green;
  destination[2] = value.red;
  destination[3] = value.alpha;
  return true;
}

} // namespace S2FileIO

#endif
