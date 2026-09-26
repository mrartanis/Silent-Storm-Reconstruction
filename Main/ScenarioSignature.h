#pragma once

#include <cstddef>
#include <cstdint>

namespace NScenario {

// Scenario item IDs index 32-bit signature words. Keep bit 31 unsigned on
// every host; the original expression used a signed int literal.
inline std::size_t ScenarioSignatureWord(int id) {
  return static_cast<std::size_t>(id) / 32;
}

inline std::uint32_t ScenarioSignatureBit(int id) {
  return std::uint32_t{1} << (static_cast<unsigned int>(id) % 32);
}

} // namespace NScenario
