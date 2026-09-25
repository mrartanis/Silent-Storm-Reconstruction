#include "../FileIO/PortableCritical.h"

#include <cmath>
#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::CriticalFields value{15, -2, -0.0f, 14, 3};
  const std::uint8_t expected[20] = {
      0x0f, 0, 0, 0, 0xfe, 0xff, 0xff, 0xff, 0, 0, 0, 0x80,
      0x0e, 0, 0, 0, 3, 0, 0, 0};
  std::uint8_t encoded[20] = {};
  S2FileIO::CriticalFields decoded{};
  if (!S2FileIO::EncodeCritical(value, encoded, 20) ||
      std::memcmp(encoded, expected, 20) != 0 ||
      !S2FileIO::DecodeCritical(expected, 20, &decoded) ||
      decoded.difficulty != 15 || decoded.duration != -2 ||
      !std::signbit(decoded.value) || decoded.critical != 14 ||
      decoded.location != 3) return 1;
  const std::int32_t values[] = {0, 1, -1, INT32_MIN, INT32_MAX};
  for (std::int32_t number : values) {
    const S2FileIO::CriticalFields item{number, number, 1.5f, number, number};
    S2FileIO::CriticalFields roundtrip{};
    if (!S2FileIO::EncodeCritical(item, encoded, 20) ||
        !S2FileIO::DecodeCritical(encoded, 20, &roundtrip) ||
        roundtrip.difficulty != number || roundtrip.duration != number ||
        roundtrip.value != 1.5f || roundtrip.critical != number ||
        roundtrip.location != number) return 2;
  }
  if (S2FileIO::DecodeCritical(expected, 19, &decoded) ||
      S2FileIO::DecodeCritical(nullptr, 20, &decoded) ||
      S2FileIO::EncodeCritical(value, encoded, 19) ||
      S2FileIO::EncodeCritical(value, nullptr, 20)) return 3;
  return 0;
}
