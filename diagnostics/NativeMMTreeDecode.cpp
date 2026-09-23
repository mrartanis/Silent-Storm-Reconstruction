#include "NativeMMTreeData.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

static void Count(const NativeLifeStudio::MMTreeOperationRecord &record,
                  std::size_t *records, std::size_t *named,
                  std::size_t *branches, std::size_t *maximumDepth,
                  std::size_t depth)
{
  ++*records;
  if (!record.name.empty()) ++*named;
  if (!record.children.empty()) ++*branches;
  if (depth > *maximumDepth) *maximumDepth = depth;
  for (const auto &child : record.children)
    Count(child, records, named, branches, maximumDepth, depth + 1);
}

static void PrintGraph(const NativeLifeStudio::MMTreeOperationRecord &record,
                       const std::string &parent, std::size_t depth)
{
  if (record.children.empty()) return;
  const std::string path = parent + "/" + record.name;
  std::printf("%zu,%zu,%s\n", depth, record.children.size(), path.c_str());
  for (const auto &child : record.children)
    PrintGraph(child, path, depth + 1);
}

static void PrintRecords(const NativeLifeStudio::MMTreeOperationRecord &record,
                         std::size_t depth, std::size_t *index)
{
  std::printf("%zu,%zu,%zu,%u,%u,%u,%u,%u,%zu,%u,%zu,%s,%s\n",
              (*index)++, depth, record.offset,
              record.headerWords[0], record.headerWords[1], record.headerWords[2],
              record.preludeSize, record.payloadSize, record.referenceOffset,
              record.serializedType,
              record.children.size(), record.name.c_str(), record.resolvedName.c_str());
  for (const auto &child : record.children)
    PrintRecords(child, depth + 1, index);
}

int main(int argc, char **argv)
{
  if (argc < 2 || argc > 3 ||
      (argc == 3 && std::strcmp(argv[2], "--graph") != 0 &&
       std::strcmp(argv[2], "--records") != 0))
  {
    std::fprintf(stderr, "usage: NativeMMTreeDecode tree.mma [--graph|--records]\n");
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  if (!file)
    return 2;
  const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  NativeLifeStudio::MMTreeRoot root;
  if (!NativeLifeStudio::DecodeMMTreeRoot(bytes.data(), bytes.size(), &root))
  {
    std::fprintf(stderr, "invalid MMLF v4 root operations: %s\n", argv[1]);
    return 3;
  }
  if (!NativeLifeStudio::ValidateMMTreeCurves(bytes.data(), bytes.size(), root))
  {
    std::fprintf(stderr, "invalid MMLF operation curves: %s\n", argv[1]);
    return 3;
  }
  if (argc == 3 && std::strcmp(argv[2], "--graph") == 0)
  {
    std::printf("depth,children,path\n0,%zu,%s\n",
                root.operations.size(), root.name.c_str());
    for (const auto &operation : root.operations)
      PrintGraph(operation, root.name, 1);
    return 0;
  }
  if (argc == 3)
  {
    std::printf("index,depth,offset,h0,h1,name-offset,prelude,payload,link,type,children,name,resolved-name\n");
    std::size_t index = 0;
    for (const auto &operation : root.operations)
      PrintRecords(operation, 1, &index);
    return 0;
  }
  std::printf("root=%s,prelude=%u,operations=%zu\n",
              root.name.c_str(), root.preludeSize, root.operations.size());
  std::size_t records = 0, named = 0, branches = 0, maximumDepth = 0;
  for (const auto &operation : root.operations)
    Count(operation, &records, &named, &branches, &maximumDepth, 1);
  std::printf("records=%zu,named=%zu,branches=%zu,max-depth=%zu\n",
              records, named, branches, maximumDepth);
  for (std::size_t i = 0; i < root.operations.size(); ++i)
    std::printf("operation=%zu,offset=%zu,payload=%u,type=%u\n",
                i, root.operations[i].offset, root.operations[i].payloadSize,
                root.operations[i].serializedType);
  return 0;
}
