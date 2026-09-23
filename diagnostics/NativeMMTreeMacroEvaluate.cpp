#include "NativeMMTreeData.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    std::fprintf(stderr, "usage: NativeMMTreeMacroEvaluate tree.mma macro-name expression\n");
    return 2;
  }
  errno = 0;
  char *end = nullptr;
  const float expression = std::strtof(argv[3], &end);
  if (errno || end == argv[3] || *end || !std::isfinite(expression)) return 2;
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) return 2;
  const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  NativeLifeStudio::MMTreeRoot root;
  std::vector<NativeLifeStudio::MMTreeEffectSample> effects;
  if (!NativeLifeStudio::DecodeMMTreeRoot(bytes.data(), bytes.size(), &root) ||
      !NativeLifeStudio::EvaluateMMTreeMacro(bytes.data(), bytes.size(), root,
                                              argv[2], expression, &effects))
    return 3;
  std::printf("kind,target,value,operation-offset\n");
  for (const auto &effect : effects)
    std::printf("%u,%s,%.9g,%zu\n", effect.kind, effect.targetName.c_str(),
                effect.expression, effect.operationOffset);
  return 0;
}
