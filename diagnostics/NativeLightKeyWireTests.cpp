#include "../Main/StdAfx.h"
#include "../Main/GParticleFormat.h"

#include <cstdint>
#include <cstring>

template<class T>
bool Check(const T& value) {
  using Codec = S2FileIO::StructureFieldCodec<T>;
  if (!Codec::kPortable || Codec::kWireSize != sizeof(T)) return false;
  std::uint8_t wire[14] = {};
  T loaded{};
  return Codec::Encode(value, wire, sizeof(T)) &&
         std::memcmp(wire, &value, sizeof(T)) == 0 &&
         Codec::Decode(wire, sizeof(T), &loaded) &&
         std::memcmp(&loaded, &value, sizeof(T)) == 0 &&
         !Codec::Decode(wire, sizeof(T) - 1, &loaded) &&
         !Codec::Encode(value, wire, sizeof(T) - 1);
}

int main() {
  NGScene::TKey<CVec3> vector{};
  vector.nT = 0x1234;
  vector.value = CVec3(1.0f, -2.0f, 0.5f);
  NGScene::TKey<float> scalar{};
  scalar.nT = -2;
  scalar.value = -0.25f;
  return Check(vector) && Check(scalar) ? 0 : 1;
}
