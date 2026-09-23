#include "NativeSequenceData.h"
#include "NativeCurve.h"
#include <cmath>
#include <cstring>
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

float ReadFloat(const unsigned char *p)
{
  const std::uint32_t bits = ReadU32(p);
  float value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

bool DecodeMacroEvents(const unsigned char *bytes, std::size_t begin,
                       std::size_t end, std::uint32_t count,
                       std::vector<SequenceTrack::MacroEvent> *result)
{
  if (count > (end - begin) / 40)
    return false;
  std::vector<SequenceTrack::MacroEvent> parsed;
  parsed.reserve(count);
  std::size_t cursor = begin;
  for (std::uint32_t i = 0; i < count; ++i)
  {
    if (end - cursor < 40)
      return false;
    SequenceTrack::MacroEvent event;
    event.offset = cursor;
    for (int word = 0; word < 8; ++word)
      event.headerWords[word] = ReadU32(bytes + cursor + word * 4);
    const std::uint32_t nameLength = ReadU32(bytes + cursor + 32);
    if (nameLength > end - cursor - 40)
      return false;
    const char *name = reinterpret_cast<const char *>(bytes + cursor + 36);
    for (std::uint32_t j = 0; j < nameLength; ++j)
      if (static_cast<unsigned char>(name[j]) < 32)
        return false;
    event.name.assign(name, nameLength);
    const std::uint32_t samples = ReadU32(bytes + cursor + 36 + nameLength);
    if (samples > (end - cursor - 40 - nameLength) / 8)
      return false;
    const std::size_t parameters = cursor + 40 + nameLength;
    event.parameterA.reserve(samples);
    event.parameterB.reserve(samples);
    for (std::uint32_t j = 0; j < samples; ++j)
      event.parameterA.push_back(ReadFloat(bytes + parameters + j * 4));
    for (std::uint32_t j = 0; j < samples; ++j)
      event.parameterB.push_back(ReadFloat(bytes + parameters + (samples + j) * 4));
    parsed.push_back(std::move(event));
    cursor = parameters + std::size_t(samples) * 8;
  }
  if (cursor != end)
    return false;
  *result = std::move(parsed);
  return true;
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
    if (nameLength > track.payloadSize - 4 || track.headerWords[3] != nameLength + 4)
      return false;
    const char *name = reinterpret_cast<const char *>(p + offset + 36);
    for (std::uint32_t j = 0; j < nameLength; ++j)
      if (static_cast<unsigned char>(name[j]) < 32)
        return false;
    track.name.assign(name, nameLength);
    const std::size_t trackEnd = offset + 32 + track.payloadSize;
    track.macroEventsDecoded = DecodeMacroEvents(p, offset + 36 + nameLength,
                                                  trackEnd, track.headerWords[2],
                                                  &track.macroEvents);
    parsed.push_back(std::move(track));
    offset += 32 + parsed.back().payloadSize;
  }
  if (offset != size)
    return false;
  *result = std::move(parsed);
  return true;
}

bool EvaluateMacroEvent(const SequenceTrack::MacroEvent &event,
                        std::uint32_t time, float *expression)
{
  const std::uint32_t start = event.headerWords[3];
  const std::uint32_t duration = event.headerWords[4];
  const auto &x = event.parameterA;
  const auto &y = event.parameterB;
  if (!expression || !duration || time < start ||
      std::uint64_t(time) >= std::uint64_t(start) + duration)
    return false;
  const float position = float(time - start) / float(duration);
  float value = 0.0f;
  if (!EvaluateHermiteCurve(x, y, position, &value))
    return false;
  *expression = value / 100.0f;
  return true;
}
}
