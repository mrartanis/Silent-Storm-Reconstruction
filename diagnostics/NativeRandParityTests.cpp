#include "../Misc/PortableRand.h"

#include <cstdint>
#include <cstdio>

// Intentionally reproduce the old MSVC expression in this x86/x64 oracle.
// It is not compiled into the game or the portable Linux/ARM64 test.
__declspec(noinline) int LegacyNext(int* seed, int maximum) {
  return (((*seed = *seed * 214013L + 2531011L) >> 16) & 0x7fff) * maximum / 0x8000;
}

int main() {
  const int startingSeeds[] = {0, 1, -1, 0x12345678, 0x7fffffff, (-2147483647 - 1), 7645};
  const int maxima[] = {0, 1, 17, 100, 32767, 65535, 100000};
  std::uint64_t hash = UINT64_C(14695981039346656037);
  for (const int startingSeed : startingSeeds) {
    for (const int maximum : maxima) {
      int legacySeed = startingSeed;
      std::int32_t portableSeed = startingSeed;
      for (int i = 0; i < 1000; ++i) {
        const int legacy = LegacyNext(&legacySeed, maximum);
        const std::int32_t portable = S2Random::Next(&portableSeed, maximum);
        if (legacySeed != portableSeed || legacy != portable) return 1;
        hash = (hash ^ static_cast<std::uint32_t>(portableSeed)) * UINT64_C(1099511628211);
        hash = (hash ^ static_cast<std::uint32_t>(portable)) * UINT64_C(1099511628211);
      }
    }
  }
  std::printf("rand-parity %016llx\n", static_cast<unsigned long long>(hash));
  return 0;
}
