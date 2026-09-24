#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstring>

int main() {
  S2FileIO::StructureExplosionPerkFields fields;
  fields.structureDamage = 1.5f;
  fields.areaDamage = -2.0f;
  fields.alwaysHumanCritical = true;
  const std::uint8_t expected[12] = {
      0x00, 0x00, 0xc0, 0x3f, 0x00, 0x00, 0x00, 0xc0,
      0x01, 0x00, 0x00, 0x00};
  std::uint8_t encoded[12];
  std::memset(encoded, 0xa5, sizeof(encoded));
  if (!S2FileIO::EncodeStructureExplosionPerks(fields, encoded, sizeof(encoded)) ||
      std::memcmp(encoded, expected, sizeof(expected)) != 0 ||
      S2FileIO::EncodeStructureExplosionPerks(fields, encoded, 11)) return 1;
  std::uint8_t legacy[12];
  std::memcpy(legacy, expected, sizeof(legacy));
  legacy[8] = 0xff; legacy[9] = 0xa5; legacy[10] = 0x5a; legacy[11] = 0x7f;
  S2FileIO::StructureExplosionPerkFields decoded;
  if (!S2FileIO::DecodeStructureExplosionPerks(legacy, sizeof(legacy), &decoded) ||
      decoded.structureDamage != fields.structureDamage ||
      decoded.areaDamage != fields.areaDamage ||
      !decoded.alwaysHumanCritical ||
      S2FileIO::DecodeStructureExplosionPerks(legacy, 11, &decoded)) return 1;
  return 0;
}
