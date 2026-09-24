#include "../Misc/PortableIsaac.h"

#include <cstdint>
#include <cstdio>

// Test-only transcription of the previous MSVC implementation, including
// the byte-offset lookup and reverse result consumption.
namespace {
constexpr int size = 256;
#define OLD_IND(mm, x) (*(std::uint32_t*)((unsigned char*)(mm) + ((x) & ((size - 1) << 2))))
#define OLD_STEP(mix, a, b, mm, m, m2, r, x) { \
  x = *m; a = (a ^ (mix)) + *(m2++); \
  *(m++) = y = OLD_IND(mm, x) + a + b; \
  *(r++) = b = OLD_IND(mm, y >> 8) + x; }
#define OLD_MIX(a,b,c,d,e,f,g,h) { \
  a ^= b << 11; d += a; b += c; \
  b ^= c >> 2; e += b; c += d; \
  c ^= d << 8; f += c; d += e; \
  d ^= e >> 16; g += d; e += f; \
  e ^= f << 10; h += e; f += g; \
  f ^= g >> 4; a += f; g += h; \
  g ^= h << 8; b += g; h += a; \
  h ^= a >> 9; c += h; a += b; }

struct LegacyIsaac {
  std::uint32_t count = 0, results[size] = {}, memory[size] = {};
  std::uint32_t a = 0, b = 0, c = 0;

  void Generate() {
    std::uint32_t x, y, *m, *m2, *r, *end;
    std::uint32_t* mm = memory;
    r = results;
    std::uint32_t localA = a;
    std::uint32_t localB = b + (++c);
    for (m = mm, end = m2 = m + size / 2; m < end;) {
      OLD_STEP(localA << 13, localA, localB, mm, m, m2, r, x);
      OLD_STEP(localA >> 6, localA, localB, mm, m, m2, r, x);
      OLD_STEP(localA << 2, localA, localB, mm, m, m2, r, x);
      OLD_STEP(localA >> 16, localA, localB, mm, m, m2, r, x);
    }
    for (m2 = mm; m2 < end;) {
      OLD_STEP(localA << 13, localA, localB, mm, m, m2, r, x);
      OLD_STEP(localA >> 6, localA, localB, mm, m, m2, r, x);
      OLD_STEP(localA << 2, localA, localB, mm, m, m2, r, x);
      OLD_STEP(localA >> 16, localA, localB, mm, m, m2, r, x);
    }
    a = localA; b = localB;
  }

  void Seed(std::uint32_t seed) {
    for (int i = 0; i < size; ++i) {
      seed = seed * 1664525u + 1013904223u;
      results[i] = seed;
    }
    a = b = c = 0;
    std::uint32_t aa, bb, cc, dd, ee, ff, gg, hh;
    aa = bb = cc = dd = ee = ff = gg = hh = 0x9e3779b9u;
    for (int i = 0; i < 4; ++i) OLD_MIX(aa, bb, cc, dd, ee, ff, gg, hh);
    for (int i = 0; i < size; i += 8) {
      aa += results[i]; bb += results[i + 1]; cc += results[i + 2]; dd += results[i + 3];
      ee += results[i + 4]; ff += results[i + 5]; gg += results[i + 6]; hh += results[i + 7];
      OLD_MIX(aa, bb, cc, dd, ee, ff, gg, hh);
      memory[i] = aa; memory[i + 1] = bb; memory[i + 2] = cc; memory[i + 3] = dd;
      memory[i + 4] = ee; memory[i + 5] = ff; memory[i + 6] = gg; memory[i + 7] = hh;
    }
    for (int i = 0; i < size; i += 8) {
      aa += memory[i]; bb += memory[i + 1]; cc += memory[i + 2]; dd += memory[i + 3];
      ee += memory[i + 4]; ff += memory[i + 5]; gg += memory[i + 6]; hh += memory[i + 7];
      OLD_MIX(aa, bb, cc, dd, ee, ff, gg, hh);
      memory[i] = aa; memory[i + 1] = bb; memory[i + 2] = cc; memory[i + 3] = dd;
      memory[i + 4] = ee; memory[i + 5] = ff; memory[i + 6] = gg; memory[i + 7] = hh;
    }
    Generate(); count = size;
  }

  std::uint32_t Next() {
    if (count-- == 0) { Generate(); count = size - 1; }
    return results[count];
  }
};
}

int main() {
  const std::uint32_t seeds[] = {0, 1, UINT32_C(0x12345678), UINT32_C(0xffffffff)};
  std::uint64_t hash = UINT64_C(14695981039346656037);
  for (const auto seed : seeds) {
    LegacyIsaac old;
    S2Random::IsaacState portable;
    old.Seed(seed);
    S2Random::IsaacSeedHarness(&portable, seed);
    for (int i = 0; i < 8192; ++i) {
      const std::uint32_t a = old.Next();
      const std::uint32_t b = S2Random::IsaacNext(&portable);
      if (a != b) { std::printf("mismatch seed=%u index=%d old=%u new=%u\n", seed, i, a, b); return 1; }
      hash = (hash ^ b) * UINT64_C(1099511628211);
    }
  }
  std::printf("isaac-parity %016llx\n", static_cast<unsigned long long>(hash));
  return 0;
}
