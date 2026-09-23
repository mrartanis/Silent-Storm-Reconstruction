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
// IAnimator stream or select the transformer's archetype weights.
bool BlendFaceGenGeometry(const FaceGenData &data,
                          const std::vector<float> &weights,
                          FaceGenGeometry *result);
// For the 36-muscle animation base, x86 blends *_M.mld vertex coordinates
// with *_A.mld muscle anchors. This does not build its complete animator.
bool BlendFaceGenAnimationGeometry(const FaceGenData &data,
                                   const std::vector<float> &weights,
                                   FaceGenGeometry *result);
// Evaluate the five GDP selector inputs for the named game sliders. This does
// not perform TriangLib's archetype interpolation or generate an animator.
bool EvaluateGameFaceGenParameters(
    const void *treeBytes, std::size_t treeSize, const MMTreeRoot &tree,
    const std::vector<std::pair<std::string, float>> &sliders,
    std::array<float, 5> *result);
// The shipping game's Age macro stays in the adult/older range. Its 16
// archetypes are ordered as four ethnicities times M/W/O/C; children get zero
// weight on that game-used range. ethnicityTotals are the four nationality
// group weights, either passed by a diagnostic or selected below.
bool ComposeGameFaceGenWeights(const std::array<float, 5> &parameters,
                               const std::array<float, 4> &ethnicityTotals,
                               std::vector<float> *result);
// The game's nationality slider has 101 discrete UI positions. At those
// positions this uses x86-oracle-derived group coefficients, while Age and
// Gender are evaluated natively from the MMT and composed analytically.
bool SelectGameFaceGenWeights(
    const void *treeBytes, std::size_t treeSize, const MMTreeRoot &tree,
    const std::vector<std::pair<std::string, float>> &sliders,
    std::vector<float> *result);
}
