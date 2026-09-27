#include "../Main/HeadSeed.h"

#include <cstdint>
#include <cstdio>

int main() {
  // On x86 this is the original address-as-seed rule.
  if (NRPG::FoldHeadSeedAddress(0x55667788ULL) != 0x55667788u) {
    std::fputs("32-bit address seed mismatch\n", stderr);
    return 1;
  }
  // On every 64-bit target both halves participate, including ARM64/Linux.
  const std::uint32_t folded =
    NRPG::FoldHeadSeedAddress(0x1122334455667788ULL);
  if (folded != (0x11223344u ^ 0x55667788u) ||
      folded == NRPG::FoldHeadSeedAddress(0x55667788ULL)) {
    std::fputs("64-bit address seed mismatch\n", stderr);
    return 1;
  }
  return 0;
}
