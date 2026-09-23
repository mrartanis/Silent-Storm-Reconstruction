#include "NativeFaceGenData.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int main()
{
  const std::string valid =
      "SLIDER Age -100 100\n"
      "HEAD Age=-25 Gender=100 African=100 Asian=-100 Arab=-100 One\n"
      "HEAD Age=-25 Gender=100 African=-100 Asian=100 Arab=-100 Two\n"
      "HEAD Age=-25 Gender=100 African=-100 Asian=-100 Arab=100 Three\n"
      "COMB Age=-25 Gender=100 African=100 Asian=100 Arab=-100 One=50 Two\n"
      "COMB Age=-25 Gender=100 African=100 Asian=100 Arab=100 One=33 Two=33 Three\n";
  NativeLifeStudio::FaceGenData data;
  std::vector<char> links(12 + 3 * 64 + 2, 0);
  const unsigned char header[] = {0x8e, 0x11, 0xd7, 0x22, 2, 0, 0, 0, 1, 0, 0, 0};
  std::memcpy(links.data(), header, sizeof(header));
  std::memcpy(links.data() + 12, "MorphA", 6);
  std::memcpy(links.data() + 76, "MorphB", 6);
  std::memcpy(links.data() + 140, "Output", 6);
  links[links.size() - 2] = 7;
  NativeLifeStudio::FaceGenLinks decoded;
  NativeLifeStudio::FaceGenData blendData;
  blendData.archetypes.resize(2);
  for (auto &archetype : blendData.archetypes)
  {
    archetype.morph.vertexCount = 1;
    archetype.morph.muscleCount = 1;
    archetype.morph.vertices.resize(1);
    archetype.morph.muscles.resize(1);
    archetype.morph.muscles[0].name = "Test";
    archetype.animation.vertexCount = 1;
    archetype.animation.muscleCount = 1;
    archetype.animation.vertices.resize(1);
    archetype.animation.muscles.resize(1);
    archetype.animation.muscles[0].name = "Animation";
  }
  blendData.archetypes[0].morph.vertices[0].sourcePosition[0] = 2.0f;
  blendData.archetypes[1].morph.vertices[0].sourcePosition[0] = 6.0f;
  blendData.archetypes[0].morph.muscles[0].pointB[0] = 4.0f;
  blendData.archetypes[1].morph.muscles[0].pointB[0] = 12.0f;
  blendData.archetypes[0].animation.vertices[0].sourcePosition[0] = 100.0f;
  blendData.archetypes[1].animation.vertices[0].sourcePosition[0] = 200.0f;
  blendData.archetypes[0].animation.muscles[0].pointB[0] = 20.0f;
  blendData.archetypes[1].animation.muscles[0].pointB[0] = 40.0f;
  for (auto &archetype : blendData.archetypes)
    for (int knot = 0; knot < 5; ++knot)
      archetype.animation.muscles[0].falloffX[knot] = static_cast<float>(knot);
  blendData.archetypes[0].animation.muscles[0].falloffY[0] = 1.0f;
  blendData.archetypes[1].animation.muscles[0].falloffY[0] = 3.0f;
  auto invalidBlend = blendData;
  invalidBlend.archetypes[0].morph.vertices[0].index = 1;
  NativeLifeStudio::FaceGenGeometry geometry;
  NativeLifeStudio::HeadData animationHead;
  std::vector<float> gameWeights;
  const bool ok = NativeLifeStudio::ParseFaceGenRules(valid, &data) &&
      data.heads.size() == 3 && data.combinations.size() == 2 &&
      data.combinations[0].parts.size() == 2 &&
      data.combinations[0].parts[0].percent == 50 &&
      data.combinations[0].parts[1].percent == 50 &&
      data.combinations[1].parts[2].percent == 34 &&
      data.heads[0].parameters[0] == -25 &&
      data.heads[0].parameters[2] == 100 &&
      !NativeLifeStudio::ParseFaceGenRules(
          "HEAD Age=-25 Gender=100 African=100 Asian=-100 Arab=-100 Bad/Name\n", &data) &&
      !NativeLifeStudio::ParseFaceGenRules(valid +
          "COMB Age=0 Gender=0 African=0 Asian=0 Arab=0 One=99 Unknown\n", &data) &&
      !NativeLifeStudio::ParseFaceGenRules(valid +
          "COMB Age=0 Gender=0 African=0 Asian=0 Arab=0 One=100 Two\n", &data) &&
      !NativeLifeStudio::ParseFaceGenRules(valid +
          "COMB Age=0 Gender=0 African=0 Asian=0 Arab=0 One Two\n", &data) &&
      NativeLifeStudio::DecodeFaceGenLinks(links.data(), links.size(), &decoded) &&
      decoded.morphNames.size() == 2 && decoded.outputNames.size() == 1 &&
      decoded.morphNames[0] == "MorphA" && decoded.outputNames[0] == "Output" &&
      decoded.matrix.size() == 2 && decoded.matrix[0] == 7 &&
      !NativeLifeStudio::DecodeFaceGenLinks(links.data(), links.size() - 1, &decoded) &&
      NativeLifeStudio::BlendFaceGenGeometry(blendData, {1.0f, 3.0f}, &geometry) &&
      geometry.vertices.size() == 1 && geometry.vertices[0][0] == 5.0f &&
      geometry.musclePointB[0][0] == 10.0f &&
      NativeLifeStudio::BlendFaceGenAnimationGeometry(blendData, {1.0f, 3.0f}, &geometry) &&
      geometry.vertices.size() == 1 && geometry.vertices[0][0] == 5.0f &&
      geometry.musclePointB[0][0] == 35.0f &&
      NativeLifeStudio::BlendFaceGenAnimationHead(blendData, {1.0f, 3.0f}, &animationHead) &&
      animationHead.vertices[0].sourcePosition[0] == 5.0f &&
      animationHead.muscles[0].pointB[0] == 35.0f &&
      animationHead.muscles[0].falloffY[0] == 2.5f &&
      !NativeLifeStudio::BlendFaceGenGeometry(invalidBlend, {1.0f, 3.0f}, &geometry) &&
      !NativeLifeStudio::BlendFaceGenGeometry(blendData, {0.0f, 0.0f}, &geometry) &&
      NativeLifeStudio::ComposeGameFaceGenWeights({0, 0, 0, 0, 0},
                                                   {0, 50, 0, 0}, &gameWeights) &&
      gameWeights.size() == 16 && gameWeights[4] == 20.0f &&
      gameWeights[5] == 20.0f && gameWeights[6] == 10.0f &&
      !NativeLifeStudio::ComposeGameFaceGenWeights({-100, 0, 0, 0, 0},
                                                    {0, 50, 0, 0}, &gameWeights);
  if (!ok) std::fprintf(stderr, "FaceGen rule parser regression\n");
  return ok ? 0 : 1;
}
