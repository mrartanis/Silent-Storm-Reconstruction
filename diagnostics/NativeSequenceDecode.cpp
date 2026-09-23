#include "NativeSequenceData.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char **argv)
{
  const bool curveAudit = argc > 2 && std::strcmp(argv[1], "--curve-audit") == 0;
  const int firstFile = curveAudit ? 2 : 1;
  if (argc <= firstFile)
  {
    std::fprintf(stderr, "usage: NativeSequenceDecode [--curve-audit] sequence.bin [more-sequences.bin ...]\n");
    return 2;
  }
  std::size_t totalTracks = 0;
  std::size_t decodedEvents = 0;
  std::size_t opaqueTracks = 0;
  std::size_t nonIncreasingCurves = 0;
  for (int arg = firstFile; arg < argc; ++arg)
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
    if (argc == 2 && !curveAudit)
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
      {
        decodedEvents += track.macroEvents.size();
        if (curveAudit)
          for (const auto &event : track.macroEvents)
          {
            const auto &x = event.parameterA;
            for (std::size_t i = 1; i < x.size(); ++i)
              if (!std::isfinite(x[i]) || x[i] <= x[i - 1])
              {
                ++nonIncreasingCurves;
                std::printf("curve-anomaly,file=%s,track=%s,event=%s,start=%u,duration=%u,sequence=%u,index=%zu,prev=%.9g,current=%.9g\n",
                            argv[arg], track.name.c_str(), event.name.c_str(),
                            event.headerWords[3], event.headerWords[4],
                            header.duration, i, x[i - 1], x[i]);
                break;
              }
          }
      }
      else if (track.headerWords[2])
        ++opaqueTracks;
  }
  if (curveAudit)
    std::printf("curve-audit,%d sequences,%zu macro events,%zu nonincreasing curves\n",
                argc - firstFile, decodedEvents, nonIncreasingCurves);
  else if (argc > 2)
    std::printf("decoded %d sequences, %zu tracks, %zu macro events, %zu opaque event tracks\n",
                argc - 1, totalTracks, decodedEvents, opaqueTracks);
  return 0;
}
