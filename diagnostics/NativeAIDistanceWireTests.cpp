#include "../Main/StdAfx.h"
#include "../Main/aiColourer.h"

#include <cstdint>
#include <cstring>
#include <limits>

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NAI::SDistanceInfo>;
  if (!Codec::kPortable || Codec::kWireSize != 32) return 1;
  NAI::SDistanceInfo value, decoded;
  value.distance = 0x1234;
  value.parent.wColor = 0x5678;
  value.parent.nLayer = -2;
  value.isProceeded = true;
  value.isInList = false;
  value.prev.wColor = 0xffff;
  value.prev.nLayer = 3;
  value.next.wColor = 0xabcd;
  value.next.nLayer = (std::numeric_limits<std::int32_t>::min)();
  std::uint8_t wire[32] = {};
  if (!Codec::Encode(value, wire, sizeof(wire)) ||
      !Codec::Decode(wire, sizeof(wire), &decoded) ||
      decoded.distance != value.distance ||
      decoded.parent.wColor != value.parent.wColor ||
      decoded.parent.nLayer != value.parent.nLayer ||
      decoded.isProceeded != value.isProceeded ||
      decoded.isInList != value.isInList ||
      decoded.prev.wColor != value.prev.wColor ||
      decoded.prev.nLayer != value.prev.nLayer ||
      decoded.next.wColor != value.next.wColor ||
      decoded.next.nLayer != value.next.nLayer ||
      Codec::Decode(wire, 31, &decoded) ||
      Codec::Encode(value, wire, 31)) return 2;
  return 0;
}
