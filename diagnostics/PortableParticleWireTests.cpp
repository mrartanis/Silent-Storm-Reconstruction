#include "../FileIO/PortableParticleWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const std::uint8_t restoreExpected[12] = {
      0xff, 0xff, 0xff, 0xff, 0x78, 0x56, 0x34, 0x12, 0x2a, 0, 0, 0};
  const S2FileIO::StructureRestoreBoneFields restore = {-1, 0x12345678, 42};
  S2FileIO::StructureRestoreBoneFields decodedRestore;
  std::uint8_t restoreBytes[12] = {};
  if (!S2FileIO::EncodeStructureRestoreBone(restore, restoreBytes, sizeof(restoreBytes)) ||
      std::memcmp(restoreBytes, restoreExpected, sizeof(restoreBytes)) ||
      !S2FileIO::DecodeStructureRestoreBone(restoreExpected, sizeof(restoreExpected), &decodedRestore) ||
      decodedRestore.p1 != -1 || decodedRestore.p2 != 0x12345678 || decodedRestore.p3 != 42 ||
      S2FileIO::DecodeStructureRestoreBone(restoreExpected, 11, &decodedRestore) ||
      S2FileIO::EncodeStructureRestoreBone(restore, restoreBytes, 11)) return 1;
  const std::uint8_t stickExpected[20] = {
      0x0d, 0, 0, 0, 0xfe, 0xff, 0xff, 0xff,
      0, 0, 0x80, 0x3e, 0, 0, 0, 0xbf,
      0x78, 0x56, 0x34, 0x12};
  const S2FileIO::StructureStickFields stick = {13, -2, 0.25f, -0.5f, 0x12345678};
  S2FileIO::StructureStickFields decodedStick;
  std::uint8_t stickBytes[20] = {};
  if (!S2FileIO::EncodeStructureStick(stick, stickBytes, sizeof(stickBytes)) ||
      std::memcmp(stickBytes, stickExpected, sizeof(stickBytes)) ||
      !S2FileIO::DecodeStructureStick(stickExpected, sizeof(stickExpected), &decodedStick) ||
      decodedStick.p1 != 13 || decodedStick.p2 != -2 || decodedStick.rest != 0.25f ||
      decodedStick.sumInvMasses != -0.5f || decodedStick.max != 0x12345678 ||
      S2FileIO::DecodeStructureStick(stickExpected, 19, &decodedStick) ||
      S2FileIO::EncodeStructureStick(stick, stickBytes, 19)) return 1;
  return 0;
}
