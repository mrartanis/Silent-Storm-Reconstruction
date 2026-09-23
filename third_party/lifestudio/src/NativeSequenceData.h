#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NativeLifeStudio
{
struct SequenceHeader
{
  std::uint32_t payloadSize = 0;
  std::uint32_t duration = 0;
  std::uint32_t trackCount = 0;
  std::uint32_t preludeSize = 0; // Bytes after the 32-byte envelope, before track records.
};

struct SequenceTrack
{
  std::size_t offset = 0; // Start of the 32-byte track header.
  std::uint32_t payloadSize = 0; // Length after the track header.
  std::uint32_t headerWords[8] = {};
  std::string name;
};

// Decode the verified 32-byte MMSF v1 envelope.
bool DecodeSequenceHeader(const void *bytes, std::size_t size, SequenceHeader *result);

// Walk the original length-delimited track records. Event/keyframe payloads
// are retained as offsets but are not evaluated by this decoder yet.
bool DecodeSequenceTracks(const void *bytes, std::size_t size,
                          std::vector<SequenceTrack> *result);
}
