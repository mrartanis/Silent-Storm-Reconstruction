#pragma once

#include <cstdint>
#include <cstring>

namespace S2Random {

// Retail SRand uses a 32-bit MSVC LCG and truncates both multiplications to
// 32 bits. Express those bit operations without signed-overflow UB so the
// sequence survives a different compiler or host architecture.
inline std::int32_t Next(std::int32_t* seed, std::int32_t maximum) {
  const std::uint32_t oldBits = static_cast<std::uint32_t>(*seed);
  const std::uint32_t nextBits = oldBits * UINT32_C(214013) + UINT32_C(2531011);
  std::memcpy(seed, &nextBits, sizeof(nextBits));
  const std::uint32_t value = (nextBits >> 16) & UINT32_C(0x7fff);
  const std::uint32_t productBits = value * static_cast<std::uint32_t>(maximum);
  std::int32_t product;
  std::memcpy(&product, &productBits, sizeof(product));
  return product / 0x8000;
}

} // namespace S2Random
