#include "../Misc/PortableVectorHash.h"

#include <cstdint>
#include <limits>

int main() {
  if (S2Math::FloatBits(-0.0f) != 0x80000000u ||
      S2Math::Vec3HashBits(1.0f, -2.0f, 0.5f) != 0xc0800000u ||
      S2Math::VisionHashTerm(-1.0f, 1024.0f) != 0xfffffc00u ||
      S2Math::VisionQueryHashBits(1.0f, -2.0f, 3.0f,
          -4.0f, 5.0f, -0.0f, 40.0f, 0.5f) != 0xbfe00480u ||
      S2Math::SignedHash(0xbfe00480u) != -1075837824 ||
      S2Math::VisionHashTerm(std::numeric_limits<float>::quiet_NaN(),
          16.0f) != 0 ||
      S2Math::VisionHashTerm(std::numeric_limits<float>::infinity(),
          16.0f) != 0 ||
      S2Math::VisionHashTerm(2147483648.0f, 1.0f) != 0) return 1;
  return 0;
}
