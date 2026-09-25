#include "../Main/StdAfx.h"
#include "../Main/PortableMeshCodecs.h"

#include <cstdint>
#include <cstring>

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NGScene::SClipShare>;
  if (!Codec::kPortable || Codec::kWireSize != 20) return 1;
  NGScene::SClipShare value, decoded;
  value.src.nID = 0x12345678;
  value.src.nPart = -2;
  value.dwParts = 0xaabbccddu;
  value.nSubBlockID = -3;
  value.nClip = 0x10203040;
  std::uint8_t wire[20] = {};
  if (!Codec::Encode(value, wire, sizeof(wire)) ||
      std::memcmp(wire, &value, sizeof(wire)) != 0 ||
      !Codec::Decode(wire, sizeof(wire), &decoded) ||
      std::memcmp(&value, &decoded, sizeof(value)) != 0 ||
      Codec::Decode(wire, 19, &decoded) ||
      Codec::Encode(value, wire, 19)) return 2;
  return 0;
}
