#include "../Main/StdAfx.h"
#include "../Main/wExplosionPerks.h"

#include <cstdint>
#include <cstring>

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NWorld::SPerkMineModifiers>;
  if (!Codec::kPortable || Codec::kWireSize != 12) return 1;
  NWorld::SPerkMineModifiers value;
  value.fStructureDmgModifier = 1.5f;
  value.fAEDmgModifier = -2.0f;
  value.bAlwaysHumanCritical = true;
  const std::uint8_t expected[12] = {
      0x00, 0x00, 0xc0, 0x3f, 0x00, 0x00, 0x00, 0xc0,
      0x01, 0x00, 0x00, 0x00};
  std::uint8_t encoded[12];
  std::memset(encoded, 0xa5, sizeof(encoded));
  if (!Codec::Encode(value, encoded, sizeof(encoded)) ||
      std::memcmp(encoded, expected, sizeof(expected)) != 0 ||
      Codec::Encode(value, encoded, 11)) return 1;
  NWorld::SPerkMineModifiers decoded;
  if (!Codec::Decode(expected, sizeof(expected), &decoded) ||
      decoded.fStructureDmgModifier != value.fStructureDmgModifier ||
      decoded.fAEDmgModifier != value.fAEDmgModifier ||
      !decoded.bAlwaysHumanCritical || Codec::Decode(expected, 11, &decoded)) return 1;
  return 0;
}
