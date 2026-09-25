#include "../Script/StdAfx.h"
#include "../Script/ltm.h"

#include <cstdint>
#include <cstring>

template<class T>
bool Check(const T& value) {
  using Codec = S2FileIO::StructureFieldCodec<T>;
  if (!Codec::kPortable || Codec::kWireSize != sizeof(T)) return false;
  std::uint8_t wire[60] = {};
  T loaded{};
  return Codec::Encode(value, wire, sizeof(T)) &&
         std::memcmp(wire, &value, sizeof(T)) == 0 &&
         Codec::Decode(wire, sizeof(T), &loaded) &&
         std::memcmp(&loaded, &value, sizeof(T)) == 0 &&
         !Codec::Decode(wire, sizeof(T) - 1, &loaded) &&
         !Codec::Encode(value, wire, sizeof(T) - 1);
}

int main() {
  TMinfo methods{};
  for (int i = 0; i < TM_N; ++i) methods.method[i] = i * 17 - 20;
  CallInfo call{};
  call.nFunc = -17;
  call.nProto = 0x12345678;
  call.lastpc = 0;
  call.line = 42;
  call.refi = -1;
  return Check(methods) && Check(call) ? 0 : 1;
}
