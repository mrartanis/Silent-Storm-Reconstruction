#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiColourer.h"

#include <cstdint>
#include <cstring>
#include <limits>

int main() {
  using RectCodec = S2FileIO::StructureFieldCodec<CTRect<unsigned char>>;
  if (!RectCodec::kPortable || RectCodec::kWireSize != 4) return 3;
  CTRect<unsigned char> rect(1, 2, 253, 254), decodedRect;
  std::uint8_t rectWire[4] = {};
  const std::uint8_t expectedRect[4] = {1, 2, 253, 254};
  if (!RectCodec::Encode(rect, rectWire, sizeof(rectWire)) ||
      std::memcmp(rectWire, expectedRect, sizeof(rectWire)) != 0 ||
      !RectCodec::Decode(rectWire, sizeof(rectWire), &decodedRect) ||
      decodedRect.left != rect.left || decodedRect.top != rect.top ||
      decodedRect.right != rect.right || decodedRect.bottom != rect.bottom ||
      RectCodec::Decode(rectWire, 3, &decodedRect) ||
      RectCodec::Encode(rect, rectWire, 3)) return 3;
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
