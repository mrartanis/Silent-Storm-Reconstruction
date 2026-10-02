#include "../Main/SWTextureFilter.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>

int main() {
  using namespace S2TerrainFilter;
  // Center of four distinct BGRA colors: every channel must be mixed, including alpha.
  if (BilinearColor(0x00000000, 0x000000ff, 0x0000ff00, 0xffff0000, 128, 128) !=
      0x40404040) return 1;
  if (BilinearColor(0x12345678, 0xffffffff, 0, 0, 0, 0) != 0x12345678) return 2;
  // Constant textures remain constant, including the largest possible channel values.
  for (unsigned u : {0u, 128u, 255u})
    for (unsigned v : {0u, 128u, 255u}) {
      if (BilinearColor(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, u, v) !=
          0xffffffff) return 3;
      if (BilinearColor(0x12345678, 0x12345678, 0x12345678, 0x12345678, u, v) !=
          0x12345678) return 4;
    }
  // Slopes cross zero without quantization or a bias in either direction.
  if (std::fabs(BilinearSlope(-2, 2, -4, 4, 128, 128)) > 1e-6f) return 5;
  if (std::fabs(BilinearSlope(0, 4, 8, 12, 64, 192) - 7.0f) > 1e-6f) return 6;
  std::puts("terrain filter: BGRA, alpha, constants and signed slopes passed");
  return 0;
}
