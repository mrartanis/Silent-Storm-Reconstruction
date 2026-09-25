#include "../Main/StdAfx.h"
#include "../Main/aiPosition.h"

#include <cstdint>
#include <cstring>

struct LegacyPathPlace {
  union {
    struct {
      union { struct { unsigned short x:8; unsigned short y:8; }; unsigned short point:16; };
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
static_assert(sizeof(LegacyPathPlace) == 4, "historical path place is one word");

static std::uint32_t Bits(const LegacyPathPlace& value) {
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, 4);
  return bits;
}

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NAI::SPathPlace>;
  if (!Codec::kPortable || Codec::kWireSize != 4 || sizeof(NAI::SPathPlace) != 4) return 1;
  const unsigned values[] = {0, 1, 0x7f, 0xff};
  for (unsigned x : values) for (unsigned y : values) for (unsigned layer : values)
    for (unsigned direction = 0; direction < 8; ++direction)
      for (unsigned pose = 0; pose < 4; ++pose) {
        LegacyPathPlace old{};
        old.x = static_cast<unsigned short>(x);
        old.y = static_cast<unsigned short>(y);
        old.integral = 1;
        old.layer = static_cast<unsigned short>(layer);
        old.direction = static_cast<unsigned short>(direction);
        old.pose = static_cast<unsigned short>(pose);
        NAI::SPathPlace now(x, y, layer, direction, pose, 0);
        if (now.GetBits() != Bits(old) || now.GetX() != old.x || now.GetY() != old.y ||
            now.GetLayer() != old.layer || now.GetDirection() != old.direction ||
            now.GetPose() != old.pose || !now.IsIntegral()) return 2;
        old.final = 1;
        old.moving = 1;
        now.SetFinal(1);
        now.SetMoving(1);
        if (now.GetBits() != Bits(old) || !now.IsFinal() || !now.IsMoving()) return 3;
        old.x = 0x42; old.y = 0x95; old.integral = 0; old.layer = 0x4a;
        old.pose = 0; old.final = 0;
        now.SetOnLayer(0x4a, 0x42, 0x95, 0);
        if (now.GetBits() != Bits(old) || now.GetLadderStep() != 0x95) return 4;
        old.layer = 0x52; old.integral = 1;
        now.SetLayer(0x52);
        now.SetIntegral(1);
        if (now.GetBits() != Bits(old)) return 7;
        std::uint8_t wire[4] = {};
        NAI::SPathPlace loaded;
        if (!Codec::Encode(now, wire, 4) || !Codec::Decode(wire, 4, &loaded) ||
            loaded.GetBits() != now.GetBits()) return 5;
      }
  NAI::SPathPlace sentinel;
  if (sentinel.GetBits() != UINT32_C(0xfdffffff)) return 6;
  return 0;
}
