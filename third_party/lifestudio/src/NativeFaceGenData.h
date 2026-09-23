#pragma once

#include "NativeHeadData.h"
#include <array>
#include <cstdint>
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

bool ParseFaceGenRules(const std::string &text, FaceGenData *result);
bool DecodeFaceGenLinks(const void *bytes, std::size_t size, FaceGenLinks *result);
bool LoadFaceGenData(LifeStudioHeadAPI::ITransformerInput *input,
                     FaceGenData *result);
}
