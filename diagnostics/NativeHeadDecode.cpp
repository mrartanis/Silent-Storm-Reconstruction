#include "NativeHeadData.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::fprintf(stderr, "usage: NativeHeadDecode original-head.bin output.csv|--muscles|--influences\n");
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
  if (std::strcmp(argv[2], "--muscles") == 0)
  {
    std::printf("index,name\n");
    for (std::size_t i = 0; i < head.muscles.size(); ++i)
      std::printf("%zu,%s\n", i, head.muscles[i].name.c_str());
    return 0;
  }
  if (std::strcmp(argv[2], "--influences") == 0)
  {
    std::printf("vertex,muscle,component-a,component-b,x,y,z\n");
    for (const auto &vertex : head.vertices)
      for (const auto &influence : vertex.influences)
        std::printf("%u,%u,%.9g,%.9g,%.9g,%.9g,%.9g\n",
                    vertex.index, influence.muscleIndex,
                    influence.componentA, influence.componentB,
                    vertex.sourcePosition[0], vertex.sourcePosition[1],
                    vertex.sourcePosition[2]);
    return 0;
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
  if (std::getenv("S2_FACE_PRINT_MUSCLES"))
    for (std::size_t i = 0; i < head.muscles.size(); ++i)
      std::fprintf(stderr, "muscle[%zu]=%s type=%u A=(%.9g,%.9g,%.9g) B=(%.9g,%.9g,%.9g)\n",
                   i, head.muscles[i].name.c_str(), head.muscles[i].type,
                   head.muscles[i].pointA[0], head.muscles[i].pointA[1], head.muscles[i].pointA[2],
                   head.muscles[i].pointB[0], head.muscles[i].pointB[1], head.muscles[i].pointB[2]);
  if (std::getenv("S2_FACE_PRINT_BONES"))
    for (std::size_t i = 0; i < head.bones.size(); ++i)
    {
      std::fprintf(stderr, "bone[%zu]=%s muscles=", i, head.bones[i].name.c_str());
      for (std::size_t n = 0; n < head.bones[i].muscleIndices.size(); ++n)
        std::fprintf(stderr, "%s%u", n ? ";" : "", head.bones[i].muscleIndices[n]);
      std::fprintf(stderr, "\n");
    }
  return 0;
}
