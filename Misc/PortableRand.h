#pragma once

#include <cstdint>
#include <cstring>

namespace S2Random {

// SRandomSeed's save field is one little-endian signed 32-bit word. Preserve
// its bits explicitly instead of serializing the C++ wrapper's object layout.
inline bool DecodeSeed(const std::uint8_t* source, std::size_t length,
                       std::int32_t* seed) {
  if (!source || !seed || length != 4) return false;
  const std::uint32_t bits = std::uint32_t(source[0]) |
      (std::uint32_t(source[1]) << 8) | (std::uint32_t(source[2]) << 16) |
      (std::uint32_t(source[3]) << 24);
  std::memcpy(seed, &bits, 4);
  return true;
}

inline bool EncodeSeed(std::int32_t seed, std::uint8_t* destination,
                       std::size_t length) {
  if (!destination || length != 4) return false;
  std::uint32_t bits = 0;
  std::memcpy(&bits, &seed, 4);
  for (unsigned i = 0; i < 4; ++i)
    destination[i] = static_cast<std::uint8_t>(bits >> (8 * i));
  return true;
}

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
