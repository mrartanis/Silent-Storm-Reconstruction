#include "../Main/StdAfx.h"
#include "../Main/GAnimParticles.h"

#include <cstdint>
#include <cstring>

int main() {
  using Restore = S2FileIO::StructureFieldCodec<NAnimation::CAParticle::SRestoreBone>;
  using Stick = S2FileIO::StructureFieldCodec<NAnimation::CAParticle::SStick>;
  if (!Restore::kPortable || Restore::kWireSize != 12 ||
      !Stick::kPortable || Stick::kWireSize != 20) return 1;
  const NAnimation::CAParticle::SRestoreBone restore(-1, 0x12345678, 42);
  NAnimation::CAParticle::SRestoreBone decodedRestore;
  std::uint8_t restoreBytes[12] = {};
  if (!Restore::Encode(restore, restoreBytes, sizeof(restoreBytes)) ||
      restoreBytes[0] != 0xff || restoreBytes[4] != 0x78 || restoreBytes[8] != 42 ||
      !Restore::Decode(restoreBytes, sizeof(restoreBytes), &decodedRestore) ||
      decodedRestore.nP1 != -1 || decodedRestore.nP2 != 0x12345678 ||
      decodedRestore.nP3 != 42 || Restore::Decode(restoreBytes, 11, &decodedRestore)) return 1;
  const NAnimation::CAParticle::SStick stick = {13, -2, 0.25f, -0.5f, 0x12345678};
  NAnimation::CAParticle::SStick decodedStick = {};
  std::uint8_t stickBytes[20] = {};
  if (!Stick::Encode(stick, stickBytes, sizeof(stickBytes)) ||
      stickBytes[0] != 13 || stickBytes[4] != 0xfe || stickBytes[10] != 0x80 ||
      stickBytes[15] != 0xbf || stickBytes[16] != 0x78 ||
      !Stick::Decode(stickBytes, sizeof(stickBytes), &decodedStick) ||
      decodedStick.nP1 != 13 || decodedStick.nP2 != -2 ||
      decodedStick.fRest != 0.25f || decodedStick.fSumInvMasses != -0.5f ||
      decodedStick.nMax != 0x12345678 || Stick::Decode(stickBytes, 19, &decodedStick)) return 1;
  return 0;
}
