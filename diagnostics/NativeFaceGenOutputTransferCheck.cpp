#include "NativeFaceGenData.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
bool ReadHead(const char *path, NativeLifeStudio::HeadData *head)
{
  std::ifstream file(path, std::ios::binary);
  const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), {});
  return !bytes.empty() &&
      NativeLifeStudio::DecodeHeadVertices(bytes.data(), bytes.size(), head);
}
}

int main(int argc, char **argv)
{
  if (argc != 5)
  {
    std::fprintf(stderr, "usage: NativeFaceGenOutputTransferCheck animation-base.bin morph-base.bin morph-processed.csv x86-generated.bin\n");
    return 2;
  }
  NativeLifeStudio::HeadData animationBase, morphBase, reference;
  if (!ReadHead(argv[1], &animationBase) || !ReadHead(argv[2], &morphBase) ||
      !ReadHead(argv[4], &reference)) return 3;
  std::ifstream csv(argv[3]);
  std::string line;
  if (!std::getline(csv, line) || line != "vertex,x,y,z") return 3;
  std::vector<std::array<float, 3>> processed;
  while (std::getline(csv, line))
  {
    unsigned index = 0;
    std::array<float, 3> vertex{};
    char extra = 0;
    if (std::sscanf(line.c_str(), "%u,%f,%f,%f%c", &index, &vertex[0],
                    &vertex[1], &vertex[2], &extra) != 4 ||
        index != processed.size()) return 3;
    processed.push_back(vertex);
  }
  NativeLifeStudio::HeadData native;
  if (!NativeLifeStudio::ComposeFaceGenOutputHead(
          animationBase, morphBase, processed, &native) ||
      native.vertices.size() != reference.vertices.size() ||
      native.implicitVertices.size() != reference.implicitVertices.size() ||
      native.muscles.size() != reference.muscles.size() ||
      native.bones.size() != reference.bones.size()) return 3;
  double maximumVertex = 0.0, maximumProjection = 0.0;
  double maximumFalloff = 0.0, maximumMuscle = 0.0, maximumBone = 0.0;
  std::size_t topology = 0;
  for (std::size_t slot = 0; slot < native.vertices.size(); ++slot)
  {
    const auto &a = native.vertices[slot];
    const auto &b = reference.vertices[slot];
    if (a.index != b.index || a.influences.size() != b.influences.size())
    {
      ++topology;
      continue;
    }
    for (int axis = 0; axis < 3; ++axis)
      maximumVertex = std::max(maximumVertex,
          std::fabs(double(a.sourcePosition[axis]) - b.sourcePosition[axis]));
    for (std::size_t influence = 0; influence < a.influences.size(); ++influence)
    {
      if (a.influences[influence].muscleIndex !=
          b.influences[influence].muscleIndex) ++topology;
      maximumProjection = std::max(maximumProjection,
          std::fabs(double(a.influences[influence].componentA) -
                    b.influences[influence].componentA));
      maximumFalloff = std::max(maximumFalloff,
          std::fabs(double(a.influences[influence].componentB) -
                    b.influences[influence].componentB));
    }
  }
  for (std::size_t slot = 0; slot < native.implicitVertices.size(); ++slot)
  {
    const auto &a = native.implicitVertices[slot];
    const auto &b = reference.implicitVertices[slot];
    if (a.index != b.index) ++topology;
    for (int axis = 0; axis < 3; ++axis)
      maximumVertex = std::max(maximumVertex,
          std::fabs(double(a.sourcePosition[axis]) - b.sourcePosition[axis]));
  }
  for (std::size_t slot = 0; slot < native.muscles.size(); ++slot)
  {
    const auto &a = native.muscles[slot];
    const auto &b = reference.muscles[slot];
    if (a.type != b.type || a.name != b.name) ++topology;
    for (int axis = 0; axis < 3; ++axis)
    {
      maximumMuscle = std::max(maximumMuscle,
          std::fabs(double(a.pointA[axis]) - b.pointA[axis]));
      maximumMuscle = std::max(maximumMuscle,
          std::fabs(double(a.pointB[axis]) - b.pointB[axis]));
    }
    for (int knot = 0; knot < 5; ++knot)
    {
      maximumMuscle = std::max(maximumMuscle,
          std::fabs(double(a.falloffX[knot]) - b.falloffX[knot]));
      maximumMuscle = std::max(maximumMuscle,
          std::fabs(double(a.falloffY[knot]) - b.falloffY[knot]));
    }
  }
  for (std::size_t slot = 0; slot < native.bones.size(); ++slot)
  {
    const auto &a = native.bones[slot];
    const auto &b = reference.bones[slot];
    if (a.name != b.name || a.muscleIndices != b.muscleIndices) ++topology;
    for (int element = 0; element < 12; ++element)
    {
      maximumBone = std::max(maximumBone,
          std::fabs(double(a.matrixA[element]) - b.matrixA[element]));
      maximumBone = std::max(maximumBone,
          std::fabs(double(a.matrixB[element]) - b.matrixB[element]));
    }
  }
  std::printf("output vertices=%zu topology=%zu max-vertex=%.9g max-projection=%.9g max-falloff=%.9g max-muscle=%.9g max-bone=%.9g\n",
      static_cast<std::size_t>(native.vertexCount), topology, maximumVertex, maximumProjection,
      maximumFalloff, maximumMuscle, maximumBone);
  return !topology && maximumVertex <= 0.0001 && maximumProjection <= 0.0001 &&
      maximumFalloff <= 0.0001 && maximumMuscle <= 0.0001 &&
      maximumBone <= 0.0001 ? 0 : 1;
}
