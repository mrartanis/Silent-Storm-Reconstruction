#include "NativeSequenceData.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::fprintf(stderr, "usage: NativeSequenceDecode sequence.bin [more-sequences.bin ...]\n");
    return 2;
  }
  std::size_t totalTracks = 0;
  std::size_t decodedEvents = 0;
  std::size_t opaqueTracks = 0;
  for (int arg = 1; arg < argc; ++arg)
  {
    std::ifstream file(argv[arg], std::ios::binary);
    if (!file)
      return 2;
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    NativeLifeStudio::SequenceHeader header;
    std::vector<NativeLifeStudio::SequenceTrack> tracks;
    if (!NativeLifeStudio::DecodeSequenceHeader(bytes.data(), bytes.size(), &header) ||
        !NativeLifeStudio::DecodeSequenceTracks(bytes.data(), bytes.size(), &tracks))
    {
      std::fprintf(stderr, "invalid MMSF v1 sequence or track records: %s\n", argv[arg]);
      return 3;
    }
    if (argc == 2)
    {
      std::printf("payload=%u,duration=%u,tracks=%u,prelude=%u\n",
                  header.payloadSize, header.duration, header.trackCount, header.preludeSize);
      for (std::size_t i = 0; i < tracks.size(); ++i)
      {
        const auto &track = tracks[i];
        std::printf("track=%zu,offset=%zu,payload=%u,events=%u,macro=%d,name=%s\n",
                    i, track.offset, track.payloadSize, track.headerWords[2],
                    track.macroEventsDecoded ? 1 : 0, track.name.c_str());
        for (const auto &event : track.macroEvents)
          std::printf("  event=%s,start=%u,duration=%u,samples=%zu\n",
                      event.name.c_str(), event.headerWords[3], event.headerWords[4],
                      event.parameterA.size());
      }
    }
    totalTracks += tracks.size();
    for (const auto &track : tracks)
      if (track.macroEventsDecoded)
        decodedEvents += track.macroEvents.size();
      else if (track.headerWords[2])
        ++opaqueTracks;
  }
  if (argc > 2)
    std::printf("decoded %d sequences, %zu tracks, %zu macro events, %zu opaque event tracks\n",
                argc - 1, totalTracks, decodedEvents, opaqueTracks);
  return 0;
}
