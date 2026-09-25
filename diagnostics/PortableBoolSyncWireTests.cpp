#include "../FileIO/PortableBoolSyncWire.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

struct TestBoolSyncValue {
  using S2BoolSyncObjectInfoWire = void;
  int nMask, nTrackID;
};
static_assert(sizeof(TestBoolSyncValue) == 8 &&
              offsetof(TestBoolSyncValue, nTrackID) == 4,
              "native bool-sync value layout");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<TestBoolSyncValue>;
  if (!Codec::kPortable || Codec::kWireSize != 8) return 1;
  const TestBoolSyncValue values[] = {{0, -1}, {1, 0}, {2, 123}, {3, -2}};
  const std::uint8_t expected[][8] = {
      {0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff},
      {1, 0, 0, 0, 0, 0, 0, 0},
      {2, 0, 0, 0, 123, 0, 0, 0},
      {3, 0, 0, 0, 0xfe, 0xff, 0xff, 0xff}};
  for (unsigned i = 0; i < 4; ++i) {
    std::uint8_t wire[8] = {};
    TestBoolSyncValue decoded{};
    if (!Codec::Encode(values[i], wire, sizeof(wire)) ||
        std::memcmp(wire, expected[i], sizeof(wire)) != 0 ||
        !Codec::Decode(expected[i], sizeof(wire), &decoded) ||
        std::memcmp(&decoded, &values[i], sizeof(decoded)) != 0 ||
        Codec::Decode(wire, 7, &decoded) ||
        Codec::Encode(values[i], wire, 7)) return 2;
  }
  return 0;
}
