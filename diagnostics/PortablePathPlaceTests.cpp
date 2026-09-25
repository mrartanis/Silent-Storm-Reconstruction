#include "../Misc/PortablePathPlace.h"

#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
struct LegacyPathPlace {
  union {
    struct {
      union {
        struct { unsigned short x:8; unsigned short y:8; };
        unsigned short point:16;
      };
      unsigned short integral:1;
      unsigned short layer:8;
      unsigned short final:1;
      unsigned short moving:1;
      unsigned short direction:3;
      unsigned short pose:2;
    };
    std::int32_t data;
  };
};
static_assert(sizeof(LegacyPathPlace) == 4, "legacy path place is a word");
#endif

int main() {
  if (S2AI::kXMask != 0xff || S2AI::kYMask != 0xff00 ||
      S2AI::kIntegralMask != 0x10000 || S2AI::kLayerMask != 0x1fe0000 ||
      S2AI::kFinalMask != 0x2000000 || S2AI::kMovingMask != 0x4000000 ||
      S2AI::kDirectionMask != 0x38000000 || S2AI::kPoseMask != 0xc0000000)
    return 1;
  if ((UINT32_C(0xffffffff) & ~S2AI::kFinalMask) != UINT32_C(0xfdffffff) ||
      S2AI::MakePathPlace(0x12, 0x34, 0x56, 5, 2, 1) != UINT32_C(0xacad3412))
    return 2;
  const std::uint32_t samples[] = {0, 1, 0x7f, 0xff};
  for (std::uint32_t x : samples) for (std::uint32_t y : samples)
    for (std::uint32_t layer : samples) for (std::uint32_t direction = 0; direction < 8; ++direction)
      for (std::uint32_t pose = 0; pose < 4; ++pose)
        for (std::uint32_t moving = 0; moving < 2; ++moving) {
          const std::uint32_t bits = S2AI::MakePathPlace(x, y, layer, direction, pose, moving);
          if ((bits & S2AI::kXMask) != x || ((bits & S2AI::kYMask) >> 8) != y ||
              ((bits & S2AI::kLayerMask) >> 17) != layer ||
              ((bits & S2AI::kDirectionMask) >> 27) != direction ||
              ((bits & S2AI::kPoseMask) >> 30) != pose ||
              ((bits & S2AI::kMovingMask) >> 26) != moving ||
              (bits & S2AI::kIntegralMask) == 0 || (bits & S2AI::kFinalMask) != 0)
            return 3;
#if defined(_MSC_VER)
          LegacyPathPlace legacy{};
          legacy.x = static_cast<unsigned short>(x);
          legacy.y = static_cast<unsigned short>(y);
          legacy.layer = static_cast<unsigned short>(layer);
          legacy.direction = static_cast<unsigned short>(direction);
          legacy.pose = static_cast<unsigned short>(pose);
          legacy.moving = static_cast<unsigned short>(moving);
          legacy.final = 0;
          legacy.integral = 1;
          std::uint32_t oldBits = 0;
          std::memcpy(&oldBits, &legacy, 4);
          if (bits != oldBits) return 4;
#endif
          std::uint8_t wire[4] = {};
          std::uint32_t decoded = 0;
          if (!S2AI::EncodePathPlace(bits, wire, 4) ||
              !S2AI::DecodePathPlace(wire, 4, &decoded) || decoded != bits)
            return 5;
        }
  const std::uint8_t wire[] = {0x12, 0x34, 0xad, 0xac};
  std::uint32_t decoded = 0;
  if (!S2AI::DecodePathPlace(wire, 4, &decoded) || decoded != UINT32_C(0xacad3412) ||
      S2AI::DecodePathPlace(wire, 3, &decoded) || S2AI::EncodePathPlace(decoded, nullptr, 4) ||
      S2AI::PathPlaceSigned(UINT32_C(0xffffffff)) != -1)
    return 6;
  return 0;
}
