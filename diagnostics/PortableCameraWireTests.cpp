#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstring>

int main() {
  const float pos[8] = {1.0f, -2.0f, 0.5f, 0.0f, 35.0f, 3.0f, 4.0f, 5.0f};
  std::uint8_t posWire[32] = {};
  float decodedPos[8] = {};
  const std::uint8_t expectedPosStart[12] = {
      0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0xc0,
      0x00, 0x00, 0x00, 0x3f};
  if (!S2FileIO::EncodeStructureCameraPos(pos, posWire, sizeof(posWire)) ||
      std::memcmp(posWire, expectedPosStart, sizeof(expectedPosStart)) != 0 ||
      !S2FileIO::DecodeStructureCameraPos(posWire, sizeof(posWire), decodedPos) ||
      std::memcmp(pos, decodedPos, sizeof(pos)) != 0 ||
      S2FileIO::DecodeStructureCameraPos(posWire, 31, decodedPos) ||
      S2FileIO::EncodeStructureCameraPos(pos, posWire, 31)) return 1;

  float limits[13] = {};
  limits[0] = 1.0f;
  limits[12] = -2.0f;
  std::uint8_t limitsWire[56];
  std::memset(limitsWire, 0xaa, sizeof(limitsWire));
  float decodedLimits[13] = {};
  bool movie = false;
  if (!S2FileIO::EncodeStructureCameraLimits(limits, true, limitsWire, sizeof(limitsWire)) ||
      limitsWire[0] != 0x00 || limitsWire[2] != 0x80 || limitsWire[3] != 0x3f ||
      limitsWire[16] != 1 || limitsWire[17] != 0 || limitsWire[18] != 0 || limitsWire[19] != 0 ||
      limitsWire[52] != 0x00 || limitsWire[55] != 0xc0 ||
      !S2FileIO::DecodeStructureCameraLimits(limitsWire, sizeof(limitsWire), decodedLimits, &movie) ||
      !movie || std::memcmp(limits, decodedLimits, sizeof(limits)) != 0 ||
      S2FileIO::DecodeStructureCameraLimits(limitsWire, 55, decodedLimits, &movie) ||
      S2FileIO::EncodeStructureCameraLimits(limits, true, limitsWire, 55)) return 1;
  limitsWire[16] = 7;
  limitsWire[17] = 0xff;
  if (!S2FileIO::DecodeStructureCameraLimits(limitsWire, sizeof(limitsWire), decodedLimits, &movie) ||
      !movie || !S2FileIO::EncodeStructureCameraLimits(decodedLimits, movie, limitsWire, sizeof(limitsWire)) ||
      limitsWire[16] != 1 || limitsWire[17] != 0) return 1;
  return 0;
}
