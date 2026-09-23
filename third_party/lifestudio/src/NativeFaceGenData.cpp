#include "NativeFaceGenData.h"
#include "LifeStudioHeadAPITransform.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace NativeLifeStudio
{
namespace
{
#include "NativeFaceGenGameEthnicity.inc"

constexpr std::array<const char *, 5> kParameters =
    {"Age", "Gender", "African", "Asian", "Arab"};

bool Integer(const std::string &text, int *value)
{
  if (text.empty()) return false;
  char *end = nullptr;
  const long parsed = std::strtol(text.c_str(), &end, 10);
  if (*end || parsed < -100 || parsed > 100) return false;
  *value = static_cast<int>(parsed);
  return true;
}

bool Parameters(std::istringstream &fields, FaceGenRule *rule)
{
  std::array<bool, 5> seen{};
  for (int i = 0; i < 5; ++i)
  {
    std::string token;
    if (!(fields >> token)) return false;
    const auto equals = token.find('=');
    if (equals == std::string::npos) return false;
    bool matched = false;
    for (std::size_t index = 0; index < kParameters.size(); ++index)
      if (token.substr(0, equals) == kParameters[index] && !seen[index])
      {
        if (!Integer(token.substr(equals + 1), &rule->parameters[index]))
          return false;
        seen[index] = true;
        matched = true;
        break;
      }
    if (!matched) return false;
  }
  return true;
}

bool ReadItem(LifeStudioHeadAPI::ITransformerInput *input,
              const std::string &name, std::vector<char> *bytes)
{
  const int size = input->Size(name.c_str());
  if (size <= 0 || size > 100000000) return false;
  bytes->resize(static_cast<std::size_t>(size));
  return input->Get(name.c_str(), bytes->data());
}

bool ReadName(const char *data, std::size_t offset, std::string *name)
{
  std::size_t length = 0;
  while (length < 64 && data[offset + length])
  {
    const unsigned char ch = static_cast<unsigned char>(data[offset + length]);
    if (ch < 32 || ch > 126) return false;
    ++length;
  }
  if (length == 0 || length == 64) return false;
  name->assign(data + offset, length);
  return true;
}

std::uint32_t Word(const char *p)
{
  const auto *b = reinterpret_cast<const unsigned char *>(p);
  return std::uint32_t(b[0]) | (std::uint32_t(b[1]) << 8) |
         (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
}
}

bool DecodeFaceGenLinks(const void *bytes, std::size_t size, FaceGenLinks *result)
{
  if (!bytes || !result || size < 12) return false;
  const auto *data = static_cast<const char *>(bytes);
  const std::uint32_t morphCount = Word(data + 4);
  const std::uint32_t outputCount = Word(data + 8);
  if (Word(data) != 0x22D7118E || morphCount == 0 || outputCount == 0 ||
      morphCount > 10000 || outputCount > 10000) return false;
  const std::size_t namesCount = std::size_t(morphCount) + outputCount;
  const std::size_t expected = 12 + namesCount * 64 +
                               std::size_t(morphCount) * outputCount;
  if (size != expected) return false;
  FaceGenLinks parsed;
  for (std::size_t i = 0; i < namesCount; ++i)
  {
    std::string name;
    if (!ReadName(data, 12 + i * 64, &name)) return false;
    if (i < morphCount) parsed.morphNames.push_back(std::move(name));
    else parsed.outputNames.push_back(std::move(name));
  }
  const auto *matrix = reinterpret_cast<const std::uint8_t *>(data + 12 + namesCount * 64);
  parsed.matrix.assign(matrix, matrix + std::size_t(morphCount) * outputCount);
  *result = std::move(parsed);
  return true;
}

bool ParseFaceGenRules(const std::string &text, FaceGenData *result)
{
  if (!result) return false;
  FaceGenData parsed;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line))
  {
    std::istringstream fields(line);
    std::string tag;
    if (!(fields >> tag) || tag == "SLIDER") continue;
    if (tag != "HEAD" && tag != "COMB") return false;
    FaceGenRule rule;
    if (!Parameters(fields, &rule)) return false;
    std::string token;
    int explicitPercent = 0;
    int implicitIndex = -1;
    int implicitCount = 0;
    while (fields >> token)
    {
      const auto equals = token.find('=');
      FaceGenBlendPart part;
      part.stem = token.substr(0, equals);
      if (part.stem.empty() ||
          !std::all_of(part.stem.begin(), part.stem.end(), [](unsigned char ch) {
            return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                   (ch >= '0' && ch <= '9') || ch == '_';
          })) return false;
      if (equals == std::string::npos)
      {
        implicitIndex = static_cast<int>(rule.parts.size());
        ++implicitCount;
      }
      else if (!Integer(token.substr(equals + 1), &part.percent) ||
               part.percent < 0)
        return false;
      else
        explicitPercent += part.percent;
      rule.parts.push_back(std::move(part));
    }
    if (tag == "HEAD")
    {
      if (rule.parts.size() != 1 || implicitIndex != 0 || implicitCount != 1)
        return false;
      rule.parts[0].percent = 100;
      parsed.heads.push_back(std::move(rule));
    }
    else
    {
      if (rule.parts.size() < 2 || implicitCount != 1 ||
          implicitIndex != static_cast<int>(rule.parts.size()) - 1 ||
          explicitPercent >= 100) return false;
      rule.parts[implicitIndex].percent = 100 - explicitPercent;
      parsed.combinations.push_back(std::move(rule));
    }
  }
  if (parsed.heads.empty()) return false;
  std::unordered_set<std::string> stems;
  for (const auto &rule : parsed.heads)
    stems.insert(rule.parts[0].stem);
  for (const auto &rule : parsed.combinations)
    for (const auto &part : rule.parts)
      if (!stems.count(part.stem)) return false;
  *result = std::move(parsed);
  return true;
}

bool LoadFaceGenData(LifeStudioHeadAPI::ITransformerInput *input,
                     FaceGenData *result)
{
  if (!input || !result) return false;
  std::vector<char> bytes;
  if (!ReadItem(input, "Morph.txt", &bytes)) return false;
  FaceGenData parsed;
  if (!ParseFaceGenRules(std::string(bytes.begin(), bytes.end()), &parsed))
    return false;
  std::unordered_set<std::string> loaded;
  for (const auto &rule : parsed.heads)
  {
    const std::string &stem = rule.parts[0].stem;
    if (!loaded.insert(stem).second) continue;
    FaceGenArchetype archetype;
    archetype.stem = stem;
    if (!ReadItem(input, stem + "_A.mld", &bytes) ||
        !DecodeHeadVertices(bytes.data(), bytes.size(), &archetype.animation) ||
        !ReadItem(input, stem + "_M.mld", &bytes) ||
        !DecodeHeadVertices(bytes.data(), bytes.size(), &archetype.morph) ||
        archetype.animation.vertexCount != archetype.morph.vertexCount)
      return false;
    if (!parsed.archetypes.empty() &&
        (archetype.animation.vertexCount != parsed.archetypes.front().animation.vertexCount ||
         archetype.animation.muscleCount != parsed.archetypes.front().animation.muscleCount ||
         archetype.morph.muscleCount != parsed.archetypes.front().morph.muscleCount))
      return false;
    parsed.archetypes.push_back(std::move(archetype));
  }
  if (!ReadItem(input, "Links.dat", &bytes) ||
      !DecodeFaceGenLinks(bytes.data(), bytes.size(), &parsed.links) ||
      parsed.links.morphNames.size() != parsed.archetypes.front().morph.muscleCount ||
      parsed.links.outputNames.size() !=
          parsed.archetypes.front().animation.muscleCount +
          parsed.archetypes.front().animation.boneCount)
    return false;
  *result = std::move(parsed);
  return true;
}

namespace
{
bool BlendGeometry(const FaceGenData &data, const std::vector<float> &weights,
                   bool animation, FaceGenGeometry *result)
{
  if (!result || data.archetypes.empty() || weights.size() != data.archetypes.size())
    return false;
  double total = 0.0;
  for (float weight : weights)
  {
    if (!std::isfinite(weight) || weight < 0.0f) return false;
    total += weight;
  }
  if (!(total > 0.0) || !std::isfinite(total)) return false;
  const auto headOf = [animation](const FaceGenArchetype &archetype) -> const HeadData & {
    return animation ? archetype.animation : archetype.morph;
  };
  const auto &base = headOf(data.archetypes.front());
  if (base.vertices.size() + base.implicitVertices.size() != base.vertexCount ||
      base.muscles.size() != base.muscleCount) return false;
  for (const auto &archetype : data.archetypes)
  {
    const auto &head = headOf(archetype);
    if (head.vertexCount != base.vertexCount || head.muscleCount != base.muscleCount ||
        head.vertices.size() != base.vertices.size() ||
        head.implicitVertices.size() != base.implicitVertices.size() ||
        head.muscles.size() != base.muscles.size()) return false;
    for (std::size_t i = 0; i < base.vertices.size(); ++i)
      if (base.vertices[i].index >= base.vertexCount ||
          head.vertices[i].index != base.vertices[i].index) return false;
    for (std::size_t i = 0; i < base.muscles.size(); ++i)
      if (head.muscles[i].name != base.muscles[i].name) return false;
    for (std::size_t i = 0; i < base.implicitVertices.size(); ++i)
      if (base.implicitVertices[i].index >= base.vertexCount ||
          head.implicitVertices[i].index != base.implicitVertices[i].index) return false;
  }
  FaceGenGeometry blended;
  blended.vertices.resize(base.vertexCount);
  blended.musclePointA.resize(base.muscleCount);
  blended.musclePointB.resize(base.muscleCount);
  for (std::size_t i = 0; i < base.vertices.size(); ++i)
    for (int axis = 0; axis < 3; ++axis)
    {
      double sum = 0.0;
      for (std::size_t n = 0; n < weights.size(); ++n)
        sum += double(weights[n]) * headOf(data.archetypes[n]).vertices[i].sourcePosition[axis];
      blended.vertices[base.vertices[i].index][axis] = static_cast<float>(sum / total);
    }
  for (std::size_t i = 0; i < base.implicitVertices.size(); ++i)
    for (int axis = 0; axis < 3; ++axis)
    {
      double sum = 0.0;
      for (std::size_t n = 0; n < weights.size(); ++n)
        sum += double(weights[n]) *
            headOf(data.archetypes[n]).implicitVertices[i].sourcePosition[axis];
      blended.vertices[base.implicitVertices[i].index][axis] =
          static_cast<float>(sum / total);
    }
  for (std::size_t i = 0; i < base.muscles.size(); ++i)
    for (int axis = 0; axis < 3; ++axis)
    {
      double sumA = 0.0, sumB = 0.0;
      for (std::size_t n = 0; n < weights.size(); ++n)
      {
        sumA += double(weights[n]) * headOf(data.archetypes[n]).muscles[i].pointA[axis];
        sumB += double(weights[n]) * headOf(data.archetypes[n]).muscles[i].pointB[axis];
      }
      blended.musclePointA[i][axis] = static_cast<float>(sumA / total);
      blended.musclePointB[i][axis] = static_cast<float>(sumB / total);
    }
  *result = std::move(blended);
  return true;
}
}

bool BlendFaceGenGeometry(const FaceGenData &data,
                          const std::vector<float> &weights,
                          FaceGenGeometry *result)
{
  return BlendGeometry(data, weights, false, result);
}

bool BlendFaceGenAnimationGeometry(const FaceGenData &data,
                                   const std::vector<float> &weights,
                                   FaceGenGeometry *result)
{
  return BlendGeometry(data, weights, true, result);
}

bool EvaluateGameFaceGenParameters(
    const void *treeBytes, std::size_t treeSize, const MMTreeRoot &tree,
    const std::vector<std::pair<std::string, float>> &sliders,
    std::array<float, 5> *result)
{
  if (!treeBytes || !result) return false;
  std::array<double, 5> sums{};
  std::array<unsigned, 5> counts{};
  for (const auto &slider : sliders)
  {
    if (!std::isfinite(slider.second) || slider.second < -1.0f ||
        slider.second > 1.0f) return false;
    std::vector<MMTreeEffectSample> effects;
    if (!EvaluateMMTreeMacro(treeBytes, treeSize, tree,
                             slider.first, slider.second, &effects))
      return false;
    for (const auto &effect : effects)
      if (effect.kind == 3)
        for (std::size_t index = 0; index < kParameters.size(); ++index)
          if (effect.targetName == kParameters[index])
          {
            sums[index] += effect.expression;
            ++counts[index];
          }
  }
  std::array<float, 5> parameters{};
  for (std::size_t index = 0; index < parameters.size(); ++index)
    if (counts[index])
      parameters[index] = static_cast<float>(100.0 * sums[index] / counts[index]);
  *result = parameters;
  return true;
}

bool ComposeGameFaceGenWeights(const std::array<float, 5> &parameters,
                               const std::array<float, 4> &ethnicityTotals,
                               std::vector<float> *result)
{
  if (!result || !std::isfinite(parameters[0]) || !std::isfinite(parameters[1]) ||
      parameters[0] < -25.0f || parameters[0] > 100.0f ||
      parameters[1] < -100.0f || parameters[1] > 100.0f)
    return false;
  for (float weight : ethnicityTotals)
    if (!std::isfinite(weight) || weight < 0.0f) return false;
  const double adult = (100.0 - parameters[0]) / 125.0;
  const double male = (100.0 + parameters[1]) / 200.0;
  std::vector<float> weights(16, 0.0f);
  for (std::size_t ethnicity = 0; ethnicity < ethnicityTotals.size(); ++ethnicity)
  {
    const double group = ethnicityTotals[ethnicity];
    weights[ethnicity * 4] = static_cast<float>(group * adult * male);
    weights[ethnicity * 4 + 1] = static_cast<float>(group * adult * (1.0 - male));
    weights[ethnicity * 4 + 2] = static_cast<float>(group * (1.0 - adult));
  }
  *result = std::move(weights);
  return true;
}

bool SelectGameFaceGenWeights(
    const void *treeBytes, std::size_t treeSize, const MMTreeRoot &tree,
    const std::vector<std::pair<std::string, float>> &sliders,
    std::vector<float> *result)
{
  std::array<float, 5> parameters{};
  if (!EvaluateGameFaceGenParameters(treeBytes, treeSize, tree,
                                     sliders, &parameters)) return false;
  std::array<float, 4> ethnicities = {0.125f, 16.875f, 16.75f, 16.75f};
  for (const auto &slider : sliders)
    if (slider.first == "Nationality")
    {
      if (!std::isfinite(slider.second) || slider.second < -1.0f ||
          slider.second > 1.0f) return false;
      const double position = (double(slider.second) + 1.0) * 50.0;
      const auto nearest = static_cast<int>(std::round(position));
      const int first = std::max(0, std::min(100,
          std::fabs(position - nearest) < 0.0001 ? nearest :
          static_cast<int>(std::floor(position))));
      const int second = std::min(100, first + 1);
      const double fraction = std::fabs(position - nearest) < 0.0001 ? 0.0 : position - first;
      for (int ethnicity = 0; ethnicity < 4; ++ethnicity)
        ethnicities[ethnicity] = static_cast<float>(
            (1.0 - fraction) * kGameEthnicityTotals[first][ethnicity] +
            fraction * kGameEthnicityTotals[second][ethnicity]);
    }
  return ComposeGameFaceGenWeights(parameters, ethnicities, result);
}
}
