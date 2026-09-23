#include "NativeSequenceData.h"
#include <utility>

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
  parsed.preludeSize = ReadU32(p + 20);
  if (parsed.payloadSize != size - 32 || !parsed.duration || !parsed.trackCount ||
      parsed.trackCount > parsed.payloadSize / 4)
    return false;
  *result = parsed;
  return true;
}

bool DecodeSequenceTracks(const void *bytes, std::size_t size,
                          std::vector<SequenceTrack> *result)
{
  SequenceHeader header;
  if (!result || !DecodeSequenceHeader(bytes, size, &header) ||
      header.preludeSize > size - 32)
    return false;
  const auto *p = static_cast<const unsigned char *>(bytes);
  std::size_t offset = 32 + header.preludeSize;
  if (header.trackCount > (size - offset) / 36)
    return false;
  std::vector<SequenceTrack> parsed;
  parsed.reserve(header.trackCount);
  for (std::uint32_t i = 0; i < header.trackCount; ++i)
  {
    if (size - offset < 36)
      return false;
    SequenceTrack track;
    track.offset = offset;
    for (int word = 0; word < 8; ++word)
      track.headerWords[word] = ReadU32(p + offset + word * 4);
    track.payloadSize = track.headerWords[1];
    if (track.payloadSize > size - offset - 32 || track.payloadSize < 4)
      return false;
    const std::uint32_t nameLength = ReadU32(p + offset + 32);
    if (nameLength > track.payloadSize - 4)
      return false;
    const char *name = reinterpret_cast<const char *>(p + offset + 36);
    for (std::uint32_t j = 0; j < nameLength; ++j)
      if (static_cast<unsigned char>(name[j]) < 32)
        return false;
    track.name.assign(name, nameLength);
    parsed.push_back(std::move(track));
    offset += 32 + parsed.back().payloadSize;
  }
  if (offset != size)
    return false;
  *result = std::move(parsed);
  return true;
}
}
