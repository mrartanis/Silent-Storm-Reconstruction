#pragma once

#include "PortableIsaac.h"

#include <chrono>
#include <cstdint>
#include <random>

namespace S2Random {

// ISAAC's state transition is deterministic; only its initial 256 words need
// host entropy. Keep this separate so the normal path needs no drive scan or
// platform API, while tests can supply a reproducible word source.
template <class WordSource>
inline void FillIsaacSeedWords(IsaacState* state, WordSource&& nextWord) {
  for (unsigned i = 0; i < kIsaacSize; ++i)
    state->results[i] = static_cast<std::uint32_t>(nextWord());
}

inline bool FillIsaacSeedFromSystem(IsaacState* state) {
  try {
    std::random_device entropy;
    FillIsaacSeedWords(state, [&entropy] { return entropy(); });
    return true;
  } catch (...) {
    // A missing OS entropy source must not hang game startup. This is only a
    // non-cryptographic fallback for a game RNG; deterministic harness seeds
    // bypass this function altogether.
    std::uint64_t seed = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    seed ^= static_cast<std::uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count());
    FillIsaacSeedWords(state, [&seed] {
      seed += UINT64_C(0x9e3779b97f4a7c15);
      std::uint64_t word = seed;
      word = (word ^ (word >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
      word = (word ^ (word >> 27)) * UINT64_C(0x94d049bb133111eb);
      return static_cast<std::uint32_t>(word ^ (word >> 31));
    });
    return false;
  }
}

} // namespace S2Random
