#include "NativeFaceGenData.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

static bool ReadWeights(const char *path, std::vector<float> *weights)
{
  std::ifstream file(path);
  std::string line;
  if (!std::getline(file, line) || line != "index,weight") return false;
  while (std::getline(file, line))
  {
    std::size_t index = 0;
    float value = 0.0f;
    char extra = 0;
    if (std::sscanf(line.c_str(), "%zu,%f%c", &index, &value, &extra) != 2 ||
        index != weights->size() || !std::isfinite(value)) return false;
    weights->push_back(value);
  }
  return weights->size() == 16;
}

int main(int argc, char **argv)
{
  if (argc < 4 || (argc - 4) % 2)
  {
    std::fprintf(stderr, "usage: NativeFaceGenGameWeightCheck head.mmt nationality-only-weights.csv|--native x86-case-weights.csv [macro amplitude]...\n");
    return 2;
  }
  std::ifstream treeFile(argv[1], std::ios::binary);
  const std::vector<char> bytes((std::istreambuf_iterator<char>(treeFile)), {});
  NativeLifeStudio::MMTreeRoot tree;
  if (bytes.empty() || !NativeLifeStudio::DecodeMMTreeRoot(bytes.data(), bytes.size(), &tree))
    return 3;
  std::vector<float> nationalityWeights, expected;
  const bool nativeEthnicity = std::string(argv[2]) == "--native";
  if ((!nativeEthnicity && !ReadWeights(argv[2], &nationalityWeights)) ||
      !ReadWeights(argv[3], &expected)) return 3;
  std::array<float, 4> groups{};
  if (!nativeEthnicity)
    for (std::size_t ethnicity = 0; ethnicity < groups.size(); ++ethnicity)
      for (int slot = 0; slot < 4; ++slot)
        groups[ethnicity] += nationalityWeights[ethnicity * 4 + slot];
  std::vector<std::pair<std::string, float>> sliders;
  for (int i = 4; i < argc; i += 2)
  {
    char *end = nullptr;
    const float value = std::strtof(argv[i + 1], &end);
    if (end == argv[i + 1] || *end || !std::isfinite(value)) return 2;
    sliders.emplace_back(argv[i], value);
  }
  std::vector<float> actual;
  if (nativeEthnicity)
  {
    if (!NativeLifeStudio::SelectGameFaceGenWeights(
            bytes.data(), bytes.size(), tree, sliders, &actual)) return 3;
  }
  else
  {
    std::array<float, 5> parameters{};
    if (!NativeLifeStudio::EvaluateGameFaceGenParameters(
            bytes.data(), bytes.size(), tree, sliders, &parameters) ||
        !NativeLifeStudio::ComposeGameFaceGenWeights(parameters, groups, &actual))
      return 3;
  }
  double maximum = 0.0;
  for (std::size_t index = 0; index < expected.size(); ++index)
    maximum = std::max(maximum, std::fabs(double(expected[index]) - actual[index]));
  std::printf("archetype-weights=16 max-delta=%.9g\n", maximum);
  return maximum <= 0.0001 ? 0 : 1;
}
