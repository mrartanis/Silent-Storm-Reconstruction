#include "../FileIO/PortableAIDistanceWire.h"

#include <cstdint>
#include <cstring>
#include <limits>

int main() {
  const S2FileIO::AIDistanceFields value{
      0x1234, {0x5678, -2}, 1, 0,
      {0xffff, 3}, {0xabcd, std::numeric_limits<std::int32_t>::min()}};
  const std::uint8_t expected[32] = {
      0x34, 0x12, 0, 0, 0x78, 0x56, 0, 0,
      0xfe, 0xff, 0xff, 0xff, 1, 0, 0, 0,
      0xff, 0xff, 0, 0, 3, 0, 0, 0,
      0xcd, 0xab, 0, 0, 0, 0, 0, 0x80};
  std::uint8_t wire[32] = {};
  S2FileIO::AIDistanceFields decoded{};
  if (!S2FileIO::EncodeAIDistance(value, wire, sizeof(wire)) ||
      std::memcmp(wire, expected, sizeof(wire)) != 0 ||
      !S2FileIO::DecodeAIDistance(expected, sizeof(expected), &decoded) ||
      decoded.distance != value.distance ||
      decoded.parent.color != value.parent.color ||
      decoded.parent.layer != value.parent.layer ||
      decoded.proceeded != value.proceeded || decoded.inList != value.inList ||
      decoded.previous.color != value.previous.color ||
      decoded.previous.layer != value.previous.layer ||
      decoded.next.color != value.next.color ||
      decoded.next.layer != value.next.layer) return 1;
  wire[2] = wire[6] = wire[14] = wire[18] = wire[26] = 0xff;
  wire[12] = 7;
  if (!S2FileIO::DecodeAIDistance(wire, sizeof(wire), &decoded) ||
      decoded.proceeded != 7 || decoded.distance != value.distance ||
      decoded.parent.color != value.parent.color ||
      !S2FileIO::EncodeAIDistance(decoded, wire, sizeof(wire)) ||
      std::memcmp(wire, expected, sizeof(wire)) != 0) return 2;
  if (S2FileIO::DecodeAIDistance(wire, 31, &decoded) ||
      S2FileIO::EncodeAIDistance(value, wire, 31) ||
      S2FileIO::DecodeAIDistance(nullptr, 32, &decoded) ||
      S2FileIO::DecodeAIDistance(wire, 32, nullptr) ||
      S2FileIO::EncodeAIDistance(value, nullptr, 32)) return 3;
  return 0;
}
