#pragma once

#include <cstdint>

namespace S2Random {

constexpr unsigned kIsaacSize = 256;

struct IsaacState {
  std::uint32_t count = 0;
  std::uint32_t results[kIsaacSize] = {};
  std::uint32_t memory[kIsaacSize] = {};
  std::uint32_t a = 0, b = 0, c = 0;
};

inline void IsaacMix(std::uint32_t& a, std::uint32_t& b,
                     std::uint32_t& c, std::uint32_t& d,
                     std::uint32_t& e, std::uint32_t& f,
                     std::uint32_t& g, std::uint32_t& h) {
  a ^= b << 11; d += a; b += c;
  b ^= c >> 2; e += b; c += d;
  c ^= d << 8; f += c; d += e;
  d ^= e >> 16; g += d; e += f;
  e ^= f << 10; h += e; f += g;
  f ^= g >> 4; a += f; g += h;
  g ^= h << 8; b += g; h += a;
  h ^= a >> 9; c += h; a += b;
}

inline void IsaacGenerate(IsaacState* state) {
  std::uint32_t a = state->a;
  std::uint32_t b = state->b + ++state->c;
  for (unsigned i = 0; i < kIsaacSize; ++i) {
    const std::uint32_t x = state->memory[i];
    switch (i & 3) {
      case 0: a ^= a << 13; break;
      case 1: a ^= a >> 6; break;
      case 2: a ^= a << 2; break;
      default: a ^= a >> 16; break;
    }
    a += state->memory[(i + kIsaacSize / 2) & (kIsaacSize - 1)];
    const std::uint32_t y = state->memory[(x >> 2) & (kIsaacSize - 1)] + a + b;
    state->memory[i] = y;
    b = state->memory[(y >> 10) & (kIsaacSize - 1)] + x;
    state->results[i] = b;
  }
  state->a = a;
  state->b = b;
}

inline void IsaacInitialize(IsaacState* state) {
  state->a = state->b = state->c = 0;
  std::uint32_t a = UINT32_C(0x9e3779b9), b = a, c = a, d = a;
  std::uint32_t e = a, f = a, g = a, h = a;
  for (unsigned i = 0; i < 4; ++i)
    IsaacMix(a, b, c, d, e, f, g, h);
  for (unsigned i = 0; i < kIsaacSize; i += 8) {
    a += state->results[i]; b += state->results[i + 1];
    c += state->results[i + 2]; d += state->results[i + 3];
    e += state->results[i + 4]; f += state->results[i + 5];
    g += state->results[i + 6]; h += state->results[i + 7];
    IsaacMix(a, b, c, d, e, f, g, h);
    state->memory[i] = a; state->memory[i + 1] = b;
    state->memory[i + 2] = c; state->memory[i + 3] = d;
    state->memory[i + 4] = e; state->memory[i + 5] = f;
    state->memory[i + 6] = g; state->memory[i + 7] = h;
  }
  for (unsigned i = 0; i < kIsaacSize; i += 8) {
    a += state->memory[i]; b += state->memory[i + 1];
    c += state->memory[i + 2]; d += state->memory[i + 3];
    e += state->memory[i + 4]; f += state->memory[i + 5];
    g += state->memory[i + 6]; h += state->memory[i + 7];
    IsaacMix(a, b, c, d, e, f, g, h);
    state->memory[i] = a; state->memory[i + 1] = b;
    state->memory[i + 2] = c; state->memory[i + 3] = d;
    state->memory[i + 4] = e; state->memory[i + 5] = f;
    state->memory[i + 6] = g; state->memory[i + 7] = h;
  }
  IsaacGenerate(state);
  state->count = kIsaacSize;
}

inline void IsaacSeedHarness(IsaacState* state, std::uint32_t seed) {
  for (unsigned i = 0; i < kIsaacSize; ++i) {
    seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
    state->results[i] = seed;
  }
  IsaacInitialize(state);
}

inline std::uint32_t IsaacNext(IsaacState* state) {
  if (state->count-- == 0) {
    IsaacGenerate(state);
    state->count = kIsaacSize - 1;
  }
  return state->results[state->count];
}

} // namespace S2Random
