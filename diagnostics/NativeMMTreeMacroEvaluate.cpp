#include "NativeMMTreeData.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static void ListMacros(const NativeLifeStudio::MMTreeOperationRecord &record)
{
  if (record.headerWords[0] == 1 && !record.name.empty())
    std::printf("%s\n", record.name.c_str());
  for (const auto &child : record.children) ListMacros(child);
}

static bool InspectMacro(const NativeLifeStudio::MMTreeOperationRecord &record,
                         const std::string &name)
{
  if (record.name == name)
  {
    std::printf("macro=%s children=%zu\n", name.c_str(), record.children.size());
    for (const auto &child : record.children)
      std::printf("kind=%u name=%s target=%s offset=%zu reference=%zu\n",
                  child.headerWords[0], child.name.c_str(), child.resolvedName.c_str(),
                  child.offset, child.referenceOffset);
    return true;
  }
  for (const auto &child : record.children)
    if (InspectMacro(child, name)) return true;
  return false;
}

int main(int argc, char **argv)
{
  if (argc != 4 && !(argc == 3 && std::string(argv[2]) == "--list"))
  {
    std::fprintf(stderr, "usage: NativeMMTreeMacroEvaluate tree.mma macro-name expression | --list | --inspect macro-name\n");
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) return 2;
  const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  NativeLifeStudio::MMTreeRoot root;
  std::vector<NativeLifeStudio::MMTreeEffectSample> effects;
  if (!NativeLifeStudio::DecodeMMTreeRoot(bytes.data(), bytes.size(), &root)) return 3;
  if (argc == 3)
  {
    for (const auto &record : root.operations) ListMacros(record);
    return 0;
  }
  if (std::string(argv[2]) == "--inspect")
  {
    for (const auto &record : root.operations)
      if (InspectMacro(record, argv[3])) return 0;
    return 3;
  }
  errno = 0;
  char *end = nullptr;
  const float expression = std::strtof(argv[3], &end);
  if (errno || end == argv[3] || *end || !std::isfinite(expression)) return 2;
  if (
      !NativeLifeStudio::EvaluateMMTreeMacro(bytes.data(), bytes.size(), root,
                                              argv[2], expression, &effects))
    return 3;
  std::printf("kind,target,value,operation-offset,channel,input,runtime-type\n");
  for (const auto &effect : effects)
    std::printf("%u,%s,%.9g,%zu,%u,%.9g,%u\n", effect.kind, effect.targetName.c_str(),
                effect.expression, effect.operationOffset, effect.channel,
                effect.inputExpression, effect.runtimeType);
  return 0;
}
