#ifndef S2_MAIN_HEAD_SEED_H
#define S2_MAIN_HEAD_SEED_H

#include <cstdint>

namespace NRPG {
// Preserve the original 32-bit address seed. On a 64-bit host, fold both
// address halves as the Windows x64 path already does, independently of the
// compiler's architecture macros.
inline constexpr std::uint32_t FoldHeadSeedAddress(std::uint64_t address) {
  return static_cast<std::uint32_t>(address) ^
    static_cast<std::uint32_t>(address >> 32);
}
}

#endif
