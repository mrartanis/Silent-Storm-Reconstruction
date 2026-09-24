#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>

int main() {
  std::uint32_t length = 0;
  const std::uint8_t shortLength[] = {8};
  const std::uint8_t longLength[] = {0xf9, 0x0e, 0x00, 0x00};
  return S2FileIO::DecodeStructureLength(shortLength, 1, 4, &length) && length == 4 &&
         S2FileIO::DecodeStructureLength(longLength, 4, 1916, &length) && length == 1916 &&
         !S2FileIO::DecodeStructureLength(shortLength, 1, 3, &length) &&
         !S2FileIO::DecodeStructureLength(longLength, 1, 1916, &length) &&
         !S2FileIO::DecodeStructureLength(nullptr, 0, 0, &length)
             ? 0 : 1;
}
