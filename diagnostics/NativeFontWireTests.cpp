#include "../Main/StdAfx.h"
#include "../Main/FontFormat.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

static_assert(offsetof(STFCharacter, x1) == 0 &&
              offsetof(STFCharacter, y1) == 4 &&
              offsetof(STFCharacter, x2) == 8 &&
              offsetof(STFCharacter, y2) == 12 &&
              offsetof(STFCharacter, nA) == 16 &&
              offsetof(STFCharacter, nBC) == 20 &&
              offsetof(STFCharacter, nWidth) == 24,
              "game font character field offsets");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<STFCharacter>;
  if (!Codec::kPortable || Codec::kWireSize != 28) return 1;
  STFCharacter source{-1, 2, 3, -4, 5, -6, 7}, decoded{};
  std::uint8_t wire[28] = {};
  if (!Codec::Encode(source, wire, sizeof(wire)) ||
      std::memcmp(wire, &source, sizeof(wire)) != 0 ||
      !Codec::Decode(wire, sizeof(wire), &decoded) ||
      std::memcmp(&source, &decoded, sizeof(source)) != 0 ||
      Codec::Decode(wire, 27, &decoded) ||
      Codec::Encode(source, wire, 27)) return 1;
  return 0;
}
