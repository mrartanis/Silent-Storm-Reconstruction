#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cmath>
#include <cstring>

int main() {
  const float fields[4] = {1.0f, -2.0f, 0.5f, -0.0f};
  const std::uint8_t expected[16] = {
      0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0xc0,
      0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x80};
  std::uint8_t encoded[16] = {};
  float decoded[4] = {};
  if (!S2FileIO::EncodeStructureFloatFields(fields, 4, encoded, sizeof(encoded)) ||
      std::memcmp(encoded, expected, sizeof(expected)) != 0 ||
      !S2FileIO::DecodeStructureFloatFields(expected, sizeof(expected), decoded, 4) ||
      decoded[0] != 1.0f || decoded[1] != -2.0f || decoded[2] != 0.5f ||
      !std::signbit(decoded[3]) ||
      S2FileIO::DecodeStructureFloatFields(expected, 15, decoded, 4) ||
      S2FileIO::EncodeStructureFloatFields(fields, 4, encoded, 15) ||
      S2FileIO::DecodeStructureFloatFields(expected, sizeof(expected), decoded, 0) ||
      S2FileIO::EncodeStructureFloatFields(fields, 17, encoded, 68)) return 1;
  return 0;
}
