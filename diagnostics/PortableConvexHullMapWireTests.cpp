#include "../FileIO/PortableConvexHullMapWire.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

struct TestHullMap {
  using S2ConvexHullMapWire = void;
  int nPieceID, nUserID;
};
static_assert(sizeof(TestHullMap) == 8 &&
              offsetof(TestHullMap, nUserID) == 4,
              "native convex hull map layout");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<TestHullMap>;
  if (!Codec::kPortable || Codec::kWireSize != 8) return 1;
  const TestHullMap values[] = {
      {0, 0}, {0x12345678, -1},
      {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}};
  const std::uint8_t expected[][8] = {
      {0, 0, 0, 0, 0, 0, 0, 0},
      {0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff},
      {0, 0, 0, 0x80, 0xff, 0xff, 0xff, 0x7f}};
  for (unsigned i = 0; i < 3; ++i) {
    std::uint8_t wire[8] = {};
    TestHullMap decoded{};
    if (!Codec::Encode(values[i], wire, sizeof(wire)) ||
        std::memcmp(wire, expected[i], sizeof(wire)) != 0 ||
        !Codec::Decode(expected[i], sizeof(wire), &decoded) ||
        std::memcmp(&decoded, &values[i], sizeof(decoded)) != 0 ||
        Codec::Decode(wire, 7, &decoded) ||
        Codec::Encode(values[i], wire, 7)) return 2;
  }
  return 0;
}
