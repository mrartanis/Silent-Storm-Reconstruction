#include "NativeSequenceData.h"

namespace NativeLifeStudio
{
namespace
{
std::uint32_t ReadU32(const unsigned char *p)
{
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
}

bool DecodeSequenceHeader(const void *bytes, std::size_t size, SequenceHeader *result)
{
  if (!bytes || !result || size < 32)
    return false;
  const unsigned char *p = static_cast<const unsigned char *>(bytes);
  if (ReadU32(p) != 0x46534D4Du || ReadU32(p + 4) != 1)
    return false;
  SequenceHeader parsed;
  parsed.payloadSize = ReadU32(p + 8);
  parsed.duration = ReadU32(p + 12);
  parsed.trackCount = ReadU32(p + 16);
  parsed.headerField20 = ReadU32(p + 20);
  if (parsed.payloadSize != size - 32 || !parsed.duration || !parsed.trackCount ||
      parsed.trackCount > parsed.payloadSize / 4)
    return false;
  *result = parsed;
  return true;
}
}
