#include "NativeHeadData.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::fprintf(stderr, "usage: NativeHeadDecode original-head.bin output.csv\n");
    return 2;
  }
  std::ifstream input(argv[1], std::ios::binary);
  if (!input)
    return 2;
  const std::vector<char> bytes(std::istreambuf_iterator<char>{input}, {});
  NativeLifeStudio::HeadData head;
  if (!NativeLifeStudio::DecodeHeadVertices(bytes.data(), bytes.size(), &head))
  {
    std::fprintf(stderr, "invalid or unsupported head stream\n");
    return 3;
  }
  FILE *out = std::fopen(argv[2], "wb");
  if (!out)
    return 2;
  std::fprintf(out, "vertex,kind,x,y,z,influences\n");
  for (const auto &vertex : head.vertices)
    std::fprintf(out, "%u,explicit,%.9g,%.9g,%.9g,%zu\n", vertex.index,
                 vertex.sourcePosition[0], vertex.sourcePosition[1],
                 vertex.sourcePosition[2], vertex.influences.size());
  for (const auto &vertex : head.implicitVertices)
    std::fprintf(out, "%u,implicit,%.9g,%.9g,%.9g,0\n", vertex.index,
                 vertex.sourcePosition[0], vertex.sourcePosition[1],
                 vertex.sourcePosition[2]);
  std::fclose(out);
  std::fprintf(stderr, "muscles=%u bones=%u vertices=%u explicit=%u implicit=%zu\n",
               head.muscleCount, head.boneCount, head.vertexCount, head.explicitVertexCount,
               head.implicitVertices.size());
  return 0;
}
