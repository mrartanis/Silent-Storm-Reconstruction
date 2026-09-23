#include "LifeStudioHeadAPIGDP.h"
#include "NativeFaceGenData.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    std::fprintf(stderr, "usage: NativeFaceGenBlendCheck face.gdp weights.csv x86-morph-head.bin\n");
    return 2;
  }
  auto *gdp = LifeStudioHeadAPI::IGDPFile::Create(argv[1]);
  if (!gdp) return 3;
  if (gdp->ObjectsCount() < 1)
  {
    gdp->Destroy();
    return 3;
  }
  auto *object = gdp->Object(0);
  NativeLifeStudio::FaceGenData data;
  const bool loaded = object && NativeLifeStudio::LoadFaceGenData(object, &data);
  if (object) object->Destroy();
  gdp->Destroy();
  if (!loaded) return 3;
  std::ifstream weightsFile(argv[2]);
  std::string line;
  if (!std::getline(weightsFile, line) || line != "index,weight") return 3;
  std::vector<float> weights;
  while (std::getline(weightsFile, line))
  {
    std::size_t index = 0;
    float weight = 0.0f;
    char extra = 0;
    if (std::sscanf(line.c_str(), "%zu,%f%c", &index, &weight, &extra) != 2 ||
        index != weights.size()) return 3;
    weights.push_back(weight);
  }
  NativeLifeStudio::FaceGenGeometry geometry;
  if (!NativeLifeStudio::BlendFaceGenGeometry(data, weights, &geometry)) return 3;
  std::ifstream headFile(argv[3], std::ios::binary);
  const std::vector<char> bytes(std::istreambuf_iterator<char>{headFile}, {});
  NativeLifeStudio::HeadData reference;
  if (bytes.empty() || !NativeLifeStudio::DecodeHeadVertices(bytes.data(), bytes.size(), &reference) ||
      reference.vertices.size() != geometry.vertices.size() ||
      reference.muscles.size() != geometry.musclePointA.size()) return 3;
  double maximumVertex = 0.0, maximumAnchor = 0.0;
  for (const auto &vertex : reference.vertices)
    for (int axis = 0; axis < 3; ++axis)
      maximumVertex = std::max(maximumVertex,
          std::fabs(double(vertex.sourcePosition[axis]) - geometry.vertices[vertex.index][axis]));
  for (std::size_t i = 0; i < reference.muscles.size(); ++i)
    for (int axis = 0; axis < 3; ++axis)
    {
      maximumAnchor = std::max(maximumAnchor,
          std::fabs(double(reference.muscles[i].pointA[axis]) - geometry.musclePointA[i][axis]));
      maximumAnchor = std::max(maximumAnchor,
          std::fabs(double(reference.muscles[i].pointB[axis]) - geometry.musclePointB[i][axis]));
    }
  std::printf("vertices=%zu muscles=%zu max-vertex=%.9g max-anchor=%.9g\n",
              geometry.vertices.size(), geometry.musclePointA.size(),
              maximumVertex, maximumAnchor);
  return maximumVertex <= 0.0001 && maximumAnchor <= 0.0001 ? 0 : 1;
}
