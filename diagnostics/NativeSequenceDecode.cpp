#include "NativeSequenceData.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::fprintf(stderr, "usage: NativeSequenceDecode sequence.bin\n");
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  if (!file)
    return 2;
  std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  NativeLifeStudio::SequenceHeader header;
  if (!NativeLifeStudio::DecodeSequenceHeader(bytes.data(), bytes.size(), &header))
  {
    std::fprintf(stderr, "invalid MMSF v1 sequence envelope: %s\n", argv[1]);
    return 3;
  }
  std::printf("payload=%u,duration=%u,tracks=%u,field20=%u\n",
              header.payloadSize, header.duration, header.trackCount, header.headerField20);
  return 0;
}
