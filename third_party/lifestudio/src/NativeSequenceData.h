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
  struct MacroEvent
  {
    std::size_t offset = 0;
    std::uint32_t headerWords[8] = {};
    std::string name;
    std::vector<float> parameterA; // Nonuniform Hermite curve positions.
    std::vector<float> parameterB; // Corresponding expression values.
  };
  bool macroEventsDecoded = false; // False for other event formats, e.g. sound.
  std::vector<MacroEvent> macroEvents;
};

// Decode the verified 32-byte MMSF v1 envelope.
bool DecodeSequenceHeader(const void *bytes, std::size_t size, SequenceHeader *result);

// Walk the original length-delimited track records. Decode macro-muscle
// events where the complete record grammar matches; leave other event formats
// opaque. This does not evaluate curves or animate vertices yet.
bool DecodeSequenceTracks(const void *bytes, std::size_t size,
                          std::vector<SequenceTrack> *result);

// Evaluate one active macro-muscle event using the original sequence's
// nonuniform cubic-Hermite curve. Returns false outside its active interval
// or for invalid/nonfinite control points. The result is expression units.
bool EvaluateMacroEvent(const SequenceTrack::MacroEvent &event,
                        std::uint32_t time, float *expression);
}
