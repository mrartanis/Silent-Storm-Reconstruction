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
  float boxFields[24] = {}, decodedBox[24] = {};
  for (int i = 0; i < 6; ++i) {
    boxFields[i * 4] = static_cast<float>(i + 1);
    boxFields[i * 4 + 1] = -2.0f;
    boxFields[i * 4 + 2] = 0.5f;
    boxFields[i * 4 + 3] = -0.0f;
  }
  std::uint8_t boxWire[96] = {};
  if (!S2FileIO::EncodeStructurePlaneBox(boxFields, boxWire, sizeof(boxWire)) ||
      boxWire[3] != 0x3f || boxWire[19] != 0x40 ||
      boxWire[15] != 0x80 || boxWire[95] != 0x80 ||
      !S2FileIO::DecodeStructurePlaneBox(boxWire, sizeof(boxWire), decodedBox) ||
      S2FileIO::DecodeStructurePlaneBox(boxWire, 95, decodedBox) ||
      S2FileIO::EncodeStructurePlaneBox(boxFields, boxWire, 95)) return 1;
  for (int i = 0; i < 6; ++i)
    if (decodedBox[i * 4] != i + 1 || decodedBox[i * 4 + 1] != -2.0f ||
        decodedBox[i * 4 + 2] != 0.5f || !std::signbit(decodedBox[i * 4 + 3])) return 1;
  return 0;
}
