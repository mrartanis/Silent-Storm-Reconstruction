#include "../Main/StdAfx.h"
#include "../Main/SelectionInfoWire.h"

#include <cstdint>
#include <cstring>

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NRender::SSelectionInfo>;
  if (!Codec::kPortable || Codec::kWireSize != 20) return 1;
  NRender::SSelectionInfo value(CVec4(1.0f, -2.0f, 0.5f, 0.0f), true);
  std::uint8_t wire[20] = {};
  if (!Codec::Encode(value, wire, sizeof(wire)) ||
      std::memcmp(wire, &value, 16) != 0 || wire[16] != 1 ||
      wire[17] != 0 || wire[18] != 0 || wire[19] != 0) return 2;
  NRender::SSelectionInfo loaded;
  if (!Codec::Decode(wire, sizeof(wire), &loaded) ||
      std::memcmp(&loaded.vColor, &value.vColor, 16) != 0 ||
      !loaded.bIgnoreFloorMask ||
      Codec::Decode(wire, 19, &loaded) ||
      Codec::Encode(value, wire, 19)) return 3;
  return 0;
}
