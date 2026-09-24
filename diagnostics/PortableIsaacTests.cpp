#include "../Misc/PortableIsaac.h"

#include <cstdint>

int main() {
  const std::uint32_t seeds[] = {0, 1, UINT32_C(0x12345678), UINT32_C(0xffffffff)};
  std::uint64_t hash = UINT64_C(14695981039346656037);
  for (const auto seed : seeds) {
    S2Random::IsaacState state;
    S2Random::IsaacSeedHarness(&state, seed);
    if (state.count != S2Random::kIsaacSize) return 1;
    for (int i = 0; i < 8192; ++i) {
      const std::uint32_t value = S2Random::IsaacNext(&state);
      hash = (hash ^ value) * UINT64_C(1099511628211);
      if (state.count >= S2Random::kIsaacSize) return 1;
    }
  }
  return hash == UINT64_C(0xc9963b4a52430ac5) ? 0 : 1;
}
