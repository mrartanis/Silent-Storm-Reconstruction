#include "../Main/StdAfx.h"
#include "../Main/Camera.h"

#include <cstdint>
#include <cstring>

template<class T>
bool Check(const T& value) {
  using Codec = S2FileIO::StructureFieldCodec<T>;
  if (!Codec::kPortable || Codec::kWireSize != sizeof(T)) return false;
  std::uint8_t wire[56] = {};
  T loaded;
  if (!Codec::Encode(value, wire, sizeof(T)) ||
      std::memcmp(wire, &value, sizeof(T)) != 0 ||
      !Codec::Decode(wire, sizeof(T), &loaded) ||
      std::memcmp(&loaded, &value, sizeof(T)) != 0 ||
      Codec::Decode(wire, sizeof(T) - 1, &loaded) ||
      Codec::Encode(value, wire, sizeof(T) - 1)) return false;
  return true;
}

int main() {
  S2CameraWire::SCameraSloMo slow;
  slow.nSloMo = 4;
  slow.tOn = -2;
  slow.tMaxLen = 0x87654321u;
  S2CameraWire::SCameraFOVEffect fov;
  fov.nSloMo = 4;
  fov.tOn = -2;
  fov.tOnFOVSpring = 1;
  fov.tMaxLen = 0x87654321u;
  fov.fFOV = 35.0f;
  fov.sDesiredPlacement = ICamera::SCameraPos(
      CVec3(100.0f, -20.0f, 7.0f), 2.0f, -3.0f, 4.0f, 0.5f, 50.0f);
  fov.fRoll = -0.25f;
  return Check(slow) && Check(fov) ? 0 : 1;
}
