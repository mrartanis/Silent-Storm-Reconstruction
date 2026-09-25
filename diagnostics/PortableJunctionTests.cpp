#include "../FileIO/PortableJunction.h"

#include <cmath>
#include <cstdint>
#include <cstring>

int main() {
  const std::uint8_t old[16] = {0, 0, 0x80, 0x3f, 0, 0, 0, 0xc0,
                                0, 0, 0, 0x80, 2, 0xaa, 0xbb, 0xcc};
  S2FileIO::JunctionFields decoded{};
  if (!S2FileIO::DecodeJunction(old, 16, &decoded) ||
      decoded.x != 1.0f || decoded.y != -2.0f ||
      !std::signbit(decoded.z) || !decoded.ground) return 1;
  std::uint8_t encoded[16] = {};
  const std::uint8_t canonical[16] = {0, 0, 0x80, 0x3f, 0, 0, 0, 0xc0,
                                      0, 0, 0, 0x80, 1, 0, 0, 0};
  if (!S2FileIO::EncodeJunction(decoded, encoded, 16) ||
      std::memcmp(encoded, canonical, 16) != 0) return 2;
  decoded.ground = false;
  if (!S2FileIO::EncodeJunction(decoded, encoded, 16) || encoded[12] != 0 ||
      S2FileIO::DecodeJunction(old, 15, &decoded) ||
      S2FileIO::DecodeJunction(nullptr, 16, &decoded) ||
      S2FileIO::EncodeJunction(decoded, encoded, 15) ||
      S2FileIO::EncodeJunction(decoded, nullptr, 16)) return 3;
  return 0;
}
