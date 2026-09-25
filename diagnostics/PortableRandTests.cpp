#include "../Misc/PortableRand.h"
#include "../Misc/PortableClockSeed.h"

#include <cstdint>
#include <cstring>

int main() {
  if (S2Random::ClockSeedFromMilliseconds(0) != 0 ||
      S2Random::ClockSeedFromMilliseconds(UINT64_C(0x100000001)) != 1)
    return 1;
  const std::int32_t wrapped = S2Random::ClockSeedFromMilliseconds(UINT64_C(0x80000000));
  std::uint32_t wrappedBits = 0;
  std::memcpy(&wrappedBits, &wrapped, sizeof(wrappedBits));
  if (wrappedBits != UINT32_C(0x80000000)) return 1;
  if (S2Random::SeedFromBits(UINT32_C(0x80000000)) != wrapped) return 1;
  (void)S2Random::ClockSeed32();

  std::int32_t seed = 0;
  const std::int32_t expectedSeeds[] = {
      2531011, 505908858, -755606699, 159719620, -1567142793,
      773150046, 548247209, 2115878600, -1462599061, 2006221698};
  const std::int32_t expectedValues[] = {0, 23, 64, 7, 27, 36, 25, 98, 31, 93};
  for (unsigned i = 0; i < 10; ++i) {
    if (S2Random::Next(&seed, 100) != expectedValues[i] || seed != expectedSeeds[i])
      return 1;
  }
  seed = 7645;
  if (S2Random::Next(&seed, 100000) != -54768 || seed != 1638660396)
    return 1; // 32-bit intermediate product wraps, too.
  seed = -1;
  if (S2Random::Next(&seed, 0) != 0 || seed != 2316998) return 1;
  return 0;
}
