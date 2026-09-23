#include "NativeHeadData.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char **argv)
{
  const bool encode = argc == 4 && std::strcmp(argv[2], "--encode") == 0;
  if (argc != 3 && !encode)
  {
    std::fprintf(stderr, "usage: NativeHeadDecode head.bin output.csv|--muscles|--influences|--bones|--neck|--encode output.bin\n");
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
  if (encode)
  {
    std::vector<char> saved;
    if (!NativeLifeStudio::EncodeSavedHead(head, &saved)) return 3;
    std::ofstream output(argv[3], std::ios::binary);
    output.write(saved.data(), static_cast<std::streamsize>(saved.size()));
    return output ? 0 : 3;
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
  if (std::strcmp(argv[2], "--bones") == 0)
  {
    std::printf("index,name,matrix,element,value\n");
    for (std::size_t i = 0; i < head.bones.size(); ++i)
      for (int n = 0; n < 12; ++n)
      {
        std::printf("%zu,%s,A,%d,%.9g\n", i, head.bones[i].name.c_str(), n,
                    head.bones[i].matrixA[n]);
        std::printf("%zu,%s,B,%d,%.9g\n", i, head.bones[i].name.c_str(), n,
                    head.bones[i].matrixB[n]);
      }
    return 0;
  }
  if (std::strcmp(argv[2], "--neck") == 0)
  {
    std::printf("zone,upper,lower,vertex\n");
    if (!head.hasNeckAppendix) return 0;
    for (std::size_t zone = 0; zone < head.neckZones.size(); ++zone)
    {
      const auto &record = head.neckZones[zone];
      for (std::uint32_t vertex = 0; vertex < head.vertexCount; ++vertex)
        if (record.vertexMask[vertex / 8] & (1u << (vertex % 8)))
          std::printf("%zu,%u,%u,%u\n", zone, record.upperVertex,
                      record.lowerVertex, vertex);
    }
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
