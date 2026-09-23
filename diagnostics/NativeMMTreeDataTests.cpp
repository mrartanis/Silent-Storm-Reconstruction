#include "NativeMMTreeData.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <vector>

static void U32(std::vector<unsigned char> *bytes, std::size_t offset, std::uint32_t value)
{
  for (int i = 0; i < 4; ++i)
    (*bytes)[offset + i] = static_cast<unsigned char>(value >> (i * 8));
}

int main()
{
  std::vector<unsigned char> tree(112, 0);
  U32(&tree, 0, 0x464C4D4Du); // MMLF
  U32(&tree, 4, 4);
  U32(&tree, 28, 80);
  U32(&tree, 32, 1);
  U32(&tree, 44, 8);
  U32(&tree, 48, 48);
  U32(&tree, 64, 4);
  for (int i = 0; i < 4; ++i) tree[68 + i] = "ROOT"[i];
  U32(&tree, 72, 1);
  U32(&tree, 72 + 12, 8);
  U32(&tree, 72 + 16, 8);
  U32(&tree, 72 + 20, 32);
  U32(&tree, 72 + 36, 5);
  NativeLifeStudio::MMTreeRoot decoded;
  assert(NativeLifeStudio::DecodeMMTreeRoot(tree.data(), tree.size(), &decoded));
  assert(decoded.name == "ROOT" && decoded.preludeSize == 8 &&
         decoded.operations.size() == 1 && decoded.operations[0].offset == 72 &&
         decoded.operations[0].payloadSize == 8 &&
         decoded.operations[0].serializedType == 5 &&
         decoded.operations[0].resolvedName == "ROOT");
  auto nested = tree;
  nested.resize(152, 0);
  U32(&nested, 28, 120);
  U32(&nested, 48, 88);
  U32(&nested, 72 + 16, 48);
  U32(&nested, 112, 1);
  U32(&nested, 112 + 8, 8);
  U32(&nested, 112 + 12, 8);
  U32(&nested, 112 + 16, 8);
  U32(&nested, 112 + 20, 32);
  U32(&nested, 112 + 36, 5);
  assert(NativeLifeStudio::DecodeMMTreeRoot(nested.data(), nested.size(), &decoded));
  assert(decoded.operations.size() == 1 && decoded.operations[0].children.size() == 1 &&
         decoded.operations[0].children[0].offset == 112 &&
         decoded.operations[0].children[0].resolvedName == "ROOT");
  auto invalidNested = nested;
  U32(&invalidNested, 112 + 16, 9);
  assert(!NativeLifeStudio::DecodeMMTreeRoot(invalidNested.data(), invalidNested.size(), &decoded));
  invalidNested = nested;
  U32(&invalidNested, 112 + 20, 114);
  assert(!NativeLifeStudio::DecodeMMTreeRoot(invalidNested.data(), invalidNested.size(), &decoded));
  invalidNested = nested;
  U32(&invalidNested, 112, 4);
  assert(!NativeLifeStudio::DecodeMMTreeRoot(invalidNested.data(), invalidNested.size(), &decoded));
  for (std::size_t n = 0; n < tree.size(); ++n)
    assert(!NativeLifeStudio::DecodeMMTreeRoot(tree.data(), n, &decoded));
  auto bad = tree;
  U32(&bad, 0, 0);
  assert(!NativeLifeStudio::DecodeMMTreeRoot(bad.data(), bad.size(), &decoded));
  bad = tree;
  U32(&bad, 28, 79);
  assert(!NativeLifeStudio::DecodeMMTreeRoot(bad.data(), bad.size(), &decoded));
  bad = tree;
  U32(&bad, 44, 9);
  assert(!NativeLifeStudio::DecodeMMTreeRoot(bad.data(), bad.size(), &decoded));
  bad = tree;
  U32(&bad, 72 + 16, 9);
  assert(!NativeLifeStudio::DecodeMMTreeRoot(bad.data(), bad.size(), &decoded));
  assert(!NativeLifeStudio::DecodeMMTreeRoot(nullptr, tree.size(), &decoded));
  assert(!NativeLifeStudio::DecodeMMTreeRoot(tree.data(), tree.size(), nullptr));
#if SIZE_MAX > UINT32_MAX
  assert(!NativeLifeStudio::DecodeMMTreeRoot(tree.data(),
         static_cast<std::size_t>(UINT32_MAX) + 1, &decoded));
#endif
  return 0;
}
