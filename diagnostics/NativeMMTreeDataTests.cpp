#include "NativeCurve.h"
#include "NativeMMTreeData.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
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
  auto morphEnvelope = tree;
  U32(&morphEnvelope, 32, 2); // FaceGen MMT uses root class 2.
  U32(&morphEnvelope, 72, 2); // Referenced child must match the root class.
  assert(NativeLifeStudio::DecodeMMTreeRoot(morphEnvelope.data(), morphEnvelope.size(), &decoded));
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
  std::vector<unsigned char> curveBytes(88, 0);
  U32(&curveBytes, 32 + 4, 5);
  for (int i = 0; i < 5; ++i)
  {
    static const std::uint32_t values[] =
      {0xBF800000u, 0xBF000000u, 0u, 0x3F000000u, 0x3F800000u};
    U32(&curveBytes, 32 + 8 + i * 4, values[i]);
    U32(&curveBytes, 32 + 28 + i * 4, values[i]);
  }
  U32(&curveBytes, 32 + 48, 0x3F800000u);
  U32(&curveBytes, 32 + 52, 0xBF800000u);
  NativeLifeStudio::MMTreeOperationRecord curveRecord;
  curveRecord.payloadSize = 56;
  curveRecord.serializedType = 5;
  curveRecord.headerWords[2] = 56;
  NativeLifeStudio::MMTreeCurve decodedCurve;
  assert(NativeLifeStudio::DecodeMMTreeCurve(curveBytes.data(), curveBytes.size(),
                                            curveRecord, &decodedCurve));
  float value = 0.0f;
  assert(NativeLifeStudio::EvaluateHermiteCurve(decodedCurve.x, decodedCurve.y,
                                                0.5f, &value) && value == 0.5f);
  curveRecord.headerWords[2] = 52;
  assert(!NativeLifeStudio::DecodeMMTreeCurve(curveBytes.data(), curveBytes.size(),
                                             curveRecord, &decodedCurve));
  curveRecord.headerWords[2] = 56;
  U32(&curveBytes, 32 + 8 + 4 * 3, 0);
  assert(!NativeLifeStudio::DecodeMMTreeCurve(curveBytes.data(), curveBytes.size(),
                                             curveRecord, &decodedCurve));
  U32(&curveBytes, 32 + 8 + 4 * 3, 0x3F000000u);
  std::vector<unsigned char> macroBytes(176, 0);
  std::copy(curveBytes.begin() + 32, curveBytes.end(), macroBytes.begin() + 120);
  NativeLifeStudio::MMTreeOperationRecord effect;
  effect.offset = 88;
  effect.headerWords[0] = 3;
  effect.headerWords[1] = 20;
  effect.headerWords[2] = 56;
  effect.payloadSize = 56;
  effect.serializedType = 5;
  effect.name = "M";
  effect.resolvedName = "M";
  NativeLifeStudio::MMTreeOperationRecord macro;
  macro.headerWords[0] = 1;
  macro.name = "TEST";
  macro.resolvedName = "TEST";
  macro.children.push_back(effect);
  NativeLifeStudio::MMTreeRoot root;
  root.operations.push_back(macro);
  std::vector<NativeLifeStudio::MMTreeEffectSample> samples;
  assert(NativeLifeStudio::EvaluateMMTreeMacro(macroBytes.data(), macroBytes.size(),
                                               root, "TEST", 0.5f, &samples));
  assert(samples.size() == 1 && samples[0].kind == 3 && samples[0].channel == 20 &&
         samples[0].targetName == "M" && samples[0].expression == 0.5f);
  macro.children[0].headerWords[0] = 5;
  root.operations[0] = macro;
  assert(NativeLifeStudio::EvaluateMMTreeMacro(macroBytes.data(), macroBytes.size(),
                                               root, "TEST", 0.5f, &samples));
  assert(samples.size() == 1 && samples[0].kind == 5 &&
         samples[0].targetName == "M" && samples[0].expression == 0.5f);
  NativeLifeStudio::MMTreeOperationRecord boneDefinition;
  boneDefinition.offset = 88;
  boneDefinition.headerWords[0] = 4;
  boneDefinition.runtimeType = 2;
  boneDefinition.name = "Bone";
  boneDefinition.resolvedName = "Bone";
  NativeLifeStudio::MMTreeOperationRecord boneReference = effect;
  boneReference.headerWords[0] = 4;
  boneReference.headerWords[1] = 17; // Header channel is not the axis.
  boneReference.referenceOffset = boneDefinition.offset;
  boneReference.name.clear();
  boneReference.resolvedName = "Bone";
  macro.children.clear();
  macro.children.push_back(boneReference);
  root.operations.clear();
  root.operations.push_back(boneDefinition);
  root.operations.push_back(macro);
  assert(NativeLifeStudio::EvaluateMMTreeMacro(macroBytes.data(), macroBytes.size(),
                                               root, "TEST", 0.5f, &samples));
  assert(samples.size() == 1 && samples[0].kind == 4 &&
         samples[0].channel == 17 && samples[0].runtimeType == 2);
  assert(!NativeLifeStudio::EvaluateMMTreeMacro(macroBytes.data(), macroBytes.size(),
                                                root, "MISSING", 0.5f, &samples));
  return 0;
}
