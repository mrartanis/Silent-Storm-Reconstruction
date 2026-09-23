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
        std::printf("track=%zu,offset=%zu,payload=%u,name=%s\n",
                    i, tracks[i].offset, tracks[i].payloadSize, tracks[i].name.c_str());
    }
  }
  if (argc > 2)
    std::printf("decoded %d sequences and all track envelopes\n", argc - 1);
  return 0;
}
