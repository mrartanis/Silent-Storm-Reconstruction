#include "../FileIO/PortableCameraEffectWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::CameraSloMoFields slow{-3, 1, 0x87654321u};
  std::uint8_t slowWire[12] = {};
  S2FileIO::CameraSloMoFields slowLoaded{};
  const std::uint8_t slowExpected[12] =
      {0xfd, 0xff, 0xff, 0xff, 1, 0, 0, 0, 0x21, 0x43, 0x65, 0x87};
  if (!S2FileIO::EncodeCameraSloMo(slow, slowWire, 12) ||
      std::memcmp(slowWire, slowExpected, 12) != 0 ||
      !S2FileIO::DecodeCameraSloMo(slowWire, 12, &slowLoaded) ||
      slowLoaded.ratio != slow.ratio || slowLoaded.enabled != slow.enabled ||
      slowLoaded.maxDuration != slow.maxDuration ||
      S2FileIO::DecodeCameraSloMo(slowWire, 11, &slowLoaded) ||
      S2FileIO::EncodeCameraSloMo(slow, nullptr, 12)) return 1;

  S2FileIO::CameraFOVEffectFields fov{};
  fov.ratio = 4;
  fov.enabled = -2;
  fov.springEnabled = 1;
  fov.maxDuration = 0x87654321u;
  fov.fov = 35.0f;
  const float position[8] = {2.0f, -3.0f, 4.0f, 0.5f,
                             50.0f, 100.0f, -20.0f, 7.0f};
  std::memcpy(fov.placement, position, sizeof(position));
  fov.roll = -0.25f;
  std::uint8_t fovWire[56] = {};
  S2FileIO::CameraFOVEffectFields fovLoaded{};
  if (!S2FileIO::EncodeCameraFOVEffect(fov, fovWire, 56) ||
      fovWire[0] != 4 || fovWire[4] != 0xfe || fovWire[8] != 1 ||
      fovWire[12] != 0x21 || fovWire[20] != 0 || fovWire[22] != 0 ||
      fovWire[23] != 0x40 ||
      !S2FileIO::DecodeCameraFOVEffect(fovWire, 56, &fovLoaded) ||
      fovLoaded.ratio != fov.ratio || fovLoaded.enabled != fov.enabled ||
      fovLoaded.springEnabled != fov.springEnabled ||
      fovLoaded.maxDuration != fov.maxDuration || fovLoaded.fov != fov.fov ||
      std::memcmp(fovLoaded.placement, fov.placement, sizeof(position)) != 0 ||
      fovLoaded.roll != fov.roll ||
      S2FileIO::DecodeCameraFOVEffect(fovWire, 55, &fovLoaded) ||
      S2FileIO::EncodeCameraFOVEffect(fov, nullptr, 56)) return 2;
  return 0;
}
