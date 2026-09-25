#include "../Main/StdAfx.h"
#include "../Main/GRenderCore.h"

#include <cstdint>
#include <cstring>

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NGScene::CPartFlags>;
  if (!Codec::kPortable || Codec::kWireSize != 32) return 1;
  NGScene::CPartFlags value, decoded;
  const int blocks[8] = {0x12345678, -1, 0, -2, 0x10203040, 1, -3, 0x76543210};
  for (int i = 0; i < 8; ++i) value.SetBlock(i, blocks[i]);
  std::uint8_t wire[32] = {};
  if (!Codec::Encode(value, wire, sizeof(wire)) ||
      std::memcmp(wire, &value, sizeof(wire)) != 0 ||
      !Codec::Decode(wire, sizeof(wire), &decoded) ||
      std::memcmp(&value, &decoded, sizeof(value)) != 0 ||
      Codec::Decode(wire, 31, &decoded) ||
      Codec::Encode(value, wire, 31)) return 2;
  return 0;
}
