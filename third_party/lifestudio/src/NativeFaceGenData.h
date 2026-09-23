#pragma once

#include "NativeHeadData.h"
#include <array>
#include <string>
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

struct FaceGenData
{
  std::vector<FaceGenRule> heads;
  std::vector<FaceGenRule> combinations;
  std::vector<FaceGenArchetype> archetypes;
  std::vector<char> links;
};

bool ParseFaceGenRules(const std::string &text, FaceGenData *result);
bool LoadFaceGenData(LifeStudioHeadAPI::ITransformerInput *input,
                     FaceGenData *result);
}
