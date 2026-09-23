#pragma once

#include "NativeHeadData.h"
#include "NativeMMTreeData.h"
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace LifeStudioHeadAPI { struct ITransformerInput; }

namespace NativeLifeStudio
{
struct FaceGenBlendPart
{
  std::string stem;
  int percent = 0;
};

struct FaceGenRule
{
  std::array<int, 5> parameters{}; // Age, Gender, African, Asian, Arab
  std::vector<FaceGenBlendPart> parts;
};

struct FaceGenArchetype
{
  std::string stem;
  HeadData animation;
  HeadData morph;
};

struct FaceGenLinks
{
  std::vector<std::string> morphNames;
  std::vector<std::string> outputNames;
  // Raw row-major bytes; the semantics of nonzero entries are not known yet.
  std::vector<std::uint8_t> matrix;
};

struct FaceGenData
{
  std::vector<FaceGenRule> heads;
  std::vector<FaceGenRule> combinations;
  std::vector<FaceGenArchetype> archetypes;
  FaceGenLinks links;
};

struct FaceGenGeometry
{
  std::vector<std::array<float, 3>> vertices;
  std::vector<std::array<float, 3>> musclePointA;
  std::vector<std::array<float, 3>> musclePointB;
};

bool ParseFaceGenRules(const std::string &text, FaceGenData *result);
bool DecodeFaceGenLinks(const void *bytes, std::size_t size, FaceGenLinks *result);
bool LoadFaceGenData(LifeStudioHeadAPI::ITransformerInput *input,
                     FaceGenData *result);
// Blend only the confirmed geometric fields. This does not synthesize a full
// IAnimator stream or infer the transformer's archetype-selection weights.
bool BlendFaceGenGeometry(const FaceGenData &data,
                          const std::vector<float> &weights,
                          FaceGenGeometry *result);
// Evaluate the five GDP selector inputs for the named game sliders. This does
// not perform TriangLib's archetype interpolation or generate an animator.
bool EvaluateGameFaceGenParameters(
    const void *treeBytes, std::size_t treeSize, const MMTreeRoot &tree,
    const std::vector<std::pair<std::string, float>> &sliders,
    std::array<float, 5> *result);
}
