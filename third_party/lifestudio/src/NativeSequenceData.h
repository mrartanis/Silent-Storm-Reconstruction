#pragma once

#include <cstddef>
#include <cstdint>

namespace NativeLifeStudio
{
struct SequenceHeader
{
  std::uint32_t payloadSize = 0;
  std::uint32_t duration = 0;
  std::uint32_t trackCount = 0;
  std::uint32_t headerField20 = 0; // Observed 76/84/100; semantics not yet established.
};

// Decode the verified 32-byte MMSF v1 envelope. This does not yet decode
// tracks or reproduce RenderMacroMuscles.
bool DecodeSequenceHeader(const void *bytes, std::size_t size, SequenceHeader *result);
}
