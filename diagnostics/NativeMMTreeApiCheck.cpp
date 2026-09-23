#include "LifeStudioHeadAPIMMTS.h"
#include "NativeMMTreeData.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>
#include <vector>

using namespace LifeStudioHeadAPI;

static bool Check(const NativeLifeStudio::MMTreeOperationRecord &record,
                  IMMTree *tree, std::unordered_set<IMacroMuscle *> *unique,
                  std::size_t *targets)
{
  if (record.headerWords[0] == 1)
  {
    IMacroMuscle *macro = tree->FindMacroMuscle(record.resolvedName.c_str());
    if (!macro) return false;
    ++*targets;
    if (!record.name.empty() && !unique->insert(macro).second)
      return false;
  }
  for (const auto &child : record.children)
    if (!Check(child, tree, unique, targets)) return false;
  return true;
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::fprintf(stderr, "usage: NativeMMTreeApiCheck tree.mma\n");
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) return 2;
  const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  NativeLifeStudio::MMTreeRoot decoded;
  if (!NativeLifeStudio::DecodeMMTreeRoot(bytes.data(), bytes.size(), &decoded)) return 3;
  IMMTree *tree = IMMTree::Create();
  if (!tree) return 3;
  const bool loaded = tree->Load(bytes.data(), static_cast<int>(bytes.size()));
  std::unordered_set<IMacroMuscle *> unique;
  std::size_t targets = 0;
  bool okay = loaded && tree->RootMacroMuscle() &&
              tree->RootMacroMuscle() == tree->FindMacroMuscle(decoded.name.c_str());
  if (okay) unique.insert(tree->RootMacroMuscle());
  for (const auto &record : decoded.operations)
    if (okay && !Check(record, tree, &unique, &targets)) okay = false;
  tree->Destroy();
  if (!okay)
  {
    std::fprintf(stderr, "native MMLF API lookup failed\n");
    return 4;
  }
  std::printf("macro-nodes=%zu,macro-targets=%zu\n", unique.size(), targets);
  return 0;
}
