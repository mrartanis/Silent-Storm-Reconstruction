#include "../Main/StdAfx.h"
#include "../Main/Camera.h"

#include <cstdint>
#include <cstring>

int main() {
  using PosCodec = S2FileIO::StructureFieldCodec<ICamera::SCameraPos>;
  using LimitsCodec = S2FileIO::StructureFieldCodec<ICamera::SCameraLimits>;
  if (!PosCodec::kPortable || PosCodec::kWireSize != 32 ||
      !LimitsCodec::kPortable || LimitsCodec::kWireSize != 56) return 1;
  ICamera::SCameraPos pos(CVec3(3.0f, 4.0f, 5.0f), 1.0f, -2.0f, 0.5f, 0.0f, 35.0f);
  std::uint8_t posWire[32] = {};
  ICamera::SCameraPos decodedPos;
  if (!PosCodec::Encode(pos, posWire, sizeof(posWire)) ||
      posWire[0] != 0x00 || posWire[2] != 0x80 || posWire[3] != 0x3f ||
      !PosCodec::Decode(posWire, sizeof(posWire), &decodedPos) ||
      decodedPos.fRod != pos.fRod || decodedPos.fPitch != pos.fPitch ||
      decodedPos.fYaw != pos.fYaw || decodedPos.fRoll != pos.fRoll ||
      decodedPos.fFOV != pos.fFOV || decodedPos.ptAnchor.x != pos.ptAnchor.x ||
      decodedPos.ptAnchor.y != pos.ptAnchor.y || decodedPos.ptAnchor.z != pos.ptAnchor.z) return 1;
  ICamera::SCameraLimits limits;
  limits.bMovie = true;
  limits.fMinRod = -11.0f;
  limits.fPitchSpeed = 8.0f;
  std::uint8_t limitsWire[56] = {};
  ICamera::SCameraLimits decodedLimits;
  if (!LimitsCodec::Encode(limits, limitsWire, sizeof(limitsWire)) ||
      limitsWire[16] != 1 || limitsWire[17] != 0 || limitsWire[18] != 0 || limitsWire[19] != 0 ||
      !LimitsCodec::Decode(limitsWire, sizeof(limitsWire), &decodedLimits) ||
      !decodedLimits.bMovie || decodedLimits.fMinRod != -11.0f ||
      decodedLimits.fPitchSpeed != 8.0f ||
      decodedLimits.fYawSpeed != limits.fYawSpeed) return 1;
  return 0;
}
