#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NativeLifeStudio
{
struct MMTreeOperationRecord
{
  std::size_t offset = 0;
  std::uint32_t headerWords[8] = {};
  std::uint32_t payloadSize = 0;
  std::uint32_t preludeSize = 0;
  std::uint32_t serializedType = 0;
  std::size_t referenceOffset = 0;
  std::string name;
  std::string resolvedName;
  std::vector<MMTreeOperationRecord> children;
};

struct MMTreeRoot
{
  std::string name;
  std::uint32_t preludeSize = 0;
  std::vector<MMTreeOperationRecord> operations;
};

struct MMTreeCurve
{
  std::vector<float> x;
  std::vector<float> y;
  float maximum = 0.0f;
  float minimum = 0.0f;
};

struct MMTreeEffectSample
{
  std::size_t operationOffset = 0;
  std::uint32_t kind = 0; // Serialized effect class 3 (muscle) or 4 (bone).
  std::uint32_t channel = 0; // Leaf header word 1; distinguishes bone axes.
  std::string targetName;
  float expression = 0.0f;
};

// Decode the MMLF v4 envelope and nested length-delimited operation graph.
// This does not interpret effect payloads or apply any facial deformation.
bool DecodeMMTreeRoot(const void *bytes, std::size_t size, MMTreeRoot *result);

// Read the operation's sampled expression curve. The two trailing floats are
// the serialized range; their role in bone effects remains under study.
bool DecodeMMTreeCurve(const void *bytes, std::size_t size,
                       const MMTreeOperationRecord &record, MMTreeCurve *result);
bool ValidateMMTreeCurves(const void *bytes, std::size_t size,
                          const MMTreeRoot &root);

// Evaluate operation curves beneath one directly called macro. The macro
// definition's own curve is not applied at the call boundary. Returns leaf
// effects in traversal order; it does not combine them or deform geometry.
bool EvaluateMMTreeMacro(const void *bytes, std::size_t size,
                         const MMTreeRoot &root, const std::string &macroName,
                         float expression, std::vector<MMTreeEffectSample> *result);
}
