#include "NativeSequenceData.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::fprintf(stderr, "usage: NativeSequenceEvaluate sequence.bin time [time ...]\n");
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  if (!file)
    return 2;
  std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  NativeLifeStudio::SequenceHeader header;
  std::vector<NativeLifeStudio::SequenceTrack> tracks;
  if (!NativeLifeStudio::DecodeSequenceHeader(bytes.data(), bytes.size(), &header) ||
      !NativeLifeStudio::DecodeSequenceTracks(bytes.data(), bytes.size(), &tracks))
    return 3;
  std::printf("time,name,value,track,event\n");
  for (int arg = 2; arg < argc; ++arg)
  {
    errno = 0;
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(argv[arg], &end, 10);
    if (errno || !end || *end || end == argv[arg] ||
        parsed >= header.duration || parsed > std::numeric_limits<std::uint32_t>::max())
      return 2;
    const auto time = static_cast<std::uint32_t>(parsed);
    for (std::size_t trackIndex = 0; trackIndex < tracks.size(); ++trackIndex)
    {
      const auto &track = tracks[trackIndex];
      if (!track.macroEventsDecoded)
        continue;
      for (std::size_t eventIndex = 0; eventIndex < track.macroEvents.size(); ++eventIndex)
      {
        const auto &event = track.macroEvents[eventIndex];
        float expression;
        if (NativeLifeStudio::EvaluateMacroEvent(event, time, &expression))
          std::printf("%u,%s,%.9g,%zu,%zu\n", time, event.name.c_str(),
                      expression, trackIndex, eventIndex);
      }
    }
  }
  return 0;
}
