#include "NativeFaceGenData.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char **argv)
{
  if (argc < 3 || (argc - 3) % 2)
  {
    std::fprintf(stderr, "usage: NativeFaceGenSelectorParamCheck head.mmt x86-params.csv [macro amplitude]...\n");
    return 2;
  }
  std::ifstream treeFile(argv[1], std::ios::binary);
  const std::vector<char> bytes((std::istreambuf_iterator<char>(treeFile)), {});
  NativeLifeStudio::MMTreeRoot tree;
  if (bytes.empty() || !NativeLifeStudio::DecodeMMTreeRoot(bytes.data(), bytes.size(), &tree))
    return 3;
  std::vector<std::pair<std::string, float>> sliders;
  for (int i = 3; i < argc; i += 2)
  {
    char *end = nullptr;
    const float value = std::strtof(argv[i + 1], &end);
    if (end == argv[i + 1] || *end || !std::isfinite(value)) return 2;
    sliders.emplace_back(argv[i], value);
  }
  std::array<float, 5> actual{};
  if (!NativeLifeStudio::EvaluateGameFaceGenParameters(
          bytes.data(), bytes.size(), tree, sliders, &actual)) return 3;
  std::ifstream referenceFile(argv[2]);
  std::string line;
  if (!std::getline(referenceFile, line) || line != "index,value") return 3;
  double maximum = 0.0;
  for (std::size_t index = 0; index < actual.size(); ++index)
  {
    if (!std::getline(referenceFile, line)) return 3;
    std::size_t row = 0;
    float expected = 0.0f;
    char extra = 0;
    if (std::sscanf(line.c_str(), "%zu,%f%c", &row, &expected, &extra) != 2 ||
        row != index || !std::isfinite(expected)) return 3;
    maximum = std::max(maximum, std::fabs(double(expected) - actual[index]));
  }
  if (std::getline(referenceFile, line)) return 3;
  std::printf("selector-parameters=5 max-delta=%.9g\n", maximum);
  return maximum <= 0.0001 ? 0 : 1;
}
