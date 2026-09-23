#include "LifeStudioHeadAPIGDP.h"
#include "NativeFaceGenData.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
  if (argc != 4 && argc != 5)
  {
    std::fprintf(stderr, "usage: NativeFaceGenBlendCheck face.gdp weights.csv x86-head.bin [--animation|--animation-fields]\n");
    return 2;
  }
  const bool fields = argc == 5 && std::strcmp(argv[4], "--animation-fields") == 0;
  const bool animation = fields || (argc == 5 && std::strcmp(argv[4], "--animation") == 0);
  if (argc == 5 && !animation) return 2;
  auto *gdp = LifeStudioHeadAPI::IGDPFile::Create(argv[1]);
  if (!gdp) return 3;
  if (gdp->ObjectsCount() < 1)
  {
    gdp->Destroy();
    return 3;
  }
  auto *object = gdp->Object(0);
  NativeLifeStudio::FaceGenData data;
  const bool loaded = object && NativeLifeStudio::LoadFaceGenData(object, &data);
  if (object) object->Destroy();
  gdp->Destroy();
  if (!loaded) return 3;
  std::ifstream weightsFile(argv[2]);
  std::string line;
  if (!std::getline(weightsFile, line) || line != "index,weight") return 3;
  std::vector<float> weights;
  while (std::getline(weightsFile, line))
  {
    std::size_t index = 0;
    float weight = 0.0f;
    char extra = 0;
    if (std::sscanf(line.c_str(), "%zu,%f%c", &index, &weight, &extra) != 2 ||
        index != weights.size()) return 3;
    weights.push_back(weight);
  }
  NativeLifeStudio::FaceGenGeometry geometry;
  if (!(animation ? NativeLifeStudio::BlendFaceGenAnimationGeometry(data, weights, &geometry)
                   : NativeLifeStudio::BlendFaceGenGeometry(data, weights, &geometry)))
    return 3;
  std::ifstream headFile(argv[3], std::ios::binary);
  const std::vector<char> bytes(std::istreambuf_iterator<char>{headFile}, {});
  NativeLifeStudio::HeadData reference;
  if (bytes.empty() || !NativeLifeStudio::DecodeHeadVertices(bytes.data(), bytes.size(), &reference) ||
      reference.vertices.size() + reference.implicitVertices.size() != geometry.vertices.size() ||
      reference.muscles.size() != geometry.musclePointA.size()) return 3;
  double maximumVertex = 0.0, maximumAnchor = 0.0;
  std::size_t badVertices = 0;
  for (const auto &vertex : reference.vertices)
  {
    double vertexDelta = 0.0;
    for (int axis = 0; axis < 3; ++axis)
      vertexDelta = std::max(vertexDelta,
          std::fabs(double(vertex.sourcePosition[axis]) - geometry.vertices[vertex.index][axis]));
    maximumVertex = std::max(maximumVertex, vertexDelta);
    if (vertexDelta > 0.0001)
    {
      ++badVertices;
      if (animation) std::fprintf(stderr, "animation vertex %u delta=%.9g\n", vertex.index, vertexDelta);
    }
  }
  for (const auto &vertex : reference.implicitVertices)
  {
    double vertexDelta = 0.0;
    for (int axis = 0; axis < 3; ++axis)
      vertexDelta = std::max(vertexDelta,
          std::fabs(double(vertex.sourcePosition[axis]) - geometry.vertices[vertex.index][axis]));
    maximumVertex = std::max(maximumVertex, vertexDelta);
    if (vertexDelta > 0.0001)
    {
      ++badVertices;
      if (animation) std::fprintf(stderr, "animation implicit vertex %u delta=%.9g\n", vertex.index, vertexDelta);
    }
  }
  for (std::size_t i = 0; i < reference.muscles.size(); ++i)
    for (int axis = 0; axis < 3; ++axis)
    {
      maximumAnchor = std::max(maximumAnchor,
          std::fabs(double(reference.muscles[i].pointA[axis]) - geometry.musclePointA[i][axis]));
      maximumAnchor = std::max(maximumAnchor,
          std::fabs(double(reference.muscles[i].pointB[axis]) - geometry.musclePointB[i][axis]));
    }
  std::printf("vertices=%zu muscles=%zu bad-vertices=%zu max-vertex=%.9g max-anchor=%.9g\n",
              geometry.vertices.size(), geometry.musclePointA.size(), badVertices,
              maximumVertex, maximumAnchor);
  if (fields)
  {
    double totalWeight = 0.0, maximumCurve = 0.0;
    double maximumInfluence = 0.0, maximumBoneMatrixA = 0.0;
    double maximumBoneMatrixB = 0.0, maximumBoneFixedOrientation = 0.0;
    double maximumBoneTranslationMean = 0.0, maximumBoneInverse = 0.0;
    std::size_t influenceTopologyMismatch = 0, boneTopologyMismatch = 0;
    std::size_t muscleTypeMismatch = 0;
    for (float weight : weights) totalWeight += weight;
    for (std::size_t muscle = 0; muscle < reference.muscles.size(); ++muscle)
    {
      if (reference.muscles[muscle].type != data.archetypes.front().animation.muscles[muscle].type ||
          reference.muscles[muscle].name != data.archetypes.front().animation.muscles[muscle].name)
        ++muscleTypeMismatch;
      for (int knot = 0; knot < 5; ++knot)
      {
        double x = 0.0, y = 0.0;
        for (std::size_t archetype = 0; archetype < weights.size(); ++archetype)
        {
          const auto &source = data.archetypes[archetype].animation.muscles[muscle];
          x += weights[archetype] * source.falloffX[knot];
          y += weights[archetype] * source.falloffY[knot];
        }
        maximumCurve = std::max(maximumCurve,
            std::fabs(reference.muscles[muscle].falloffX[knot] - x / totalWeight));
        maximumCurve = std::max(maximumCurve,
            std::fabs(reference.muscles[muscle].falloffY[knot] - y / totalWeight));
      }
    }
    const auto &first = data.archetypes.front().animation;
    if (reference.vertices.size() != first.vertices.size()) return 3;
    for (std::size_t slot = 0; slot < reference.vertices.size(); ++slot)
    {
      const auto &vertex = reference.vertices[slot];
      const auto &source = first.vertices[slot];
      if (vertex.index != source.index ||
          vertex.influences.size() != source.influences.size())
      {
        ++influenceTopologyMismatch;
        continue;
      }
      for (std::size_t n = 0; n < vertex.influences.size(); ++n)
      {
        const auto &influence = vertex.influences[n];
        if (influence.muscleIndex != source.influences[n].muscleIndex)
        {
          ++influenceTopologyMismatch;
          continue;
        }
        const auto &muscle = reference.muscles[influence.muscleIndex];
        double numerator = 0.0, denominator = 0.0;
        for (int axis = 0; axis < 3; ++axis)
        {
          const double vector = muscle.pointB[axis] - muscle.pointA[axis];
          numerator += (vertex.sourcePosition[axis] - muscle.pointA[axis]) * vector;
          denominator += vector * vector;
        }
        if (denominator > 0.0)
          maximumInfluence = std::max(maximumInfluence,
              std::fabs(influence.componentA - numerator / denominator));
      }
    }
    if (reference.bones.size() != first.bones.size()) return 3;
    for (std::size_t bone = 0; bone < reference.bones.size(); ++bone)
    {
      const auto &actual = reference.bones[bone];
      const auto &source = first.bones[bone];
      if (actual.name != source.name || actual.muscleIndices != source.muscleIndices)
        ++boneTopologyMismatch;
      for (int element = 0; element < 12; ++element)
      {
        double meanA = 0.0, meanB = 0.0;
        for (std::size_t archetype = 0; archetype < weights.size(); ++archetype)
        {
          const auto &candidate = data.archetypes[archetype].animation.bones[bone];
          meanA += weights[archetype] * candidate.matrixA[element];
          meanB += weights[archetype] * candidate.matrixB[element];
        }
        maximumBoneMatrixA = std::max(maximumBoneMatrixA,
            std::fabs(actual.matrixA[element] - meanA / totalWeight));
        maximumBoneMatrixB = std::max(maximumBoneMatrixB,
            std::fabs(actual.matrixB[element] - meanB / totalWeight));
        if (element < 9)
        {
          maximumBoneFixedOrientation = std::max(maximumBoneFixedOrientation,
              std::fabs(double(actual.matrixA[element]) - source.matrixA[element]));
          maximumBoneInverse = std::max(maximumBoneInverse,
              std::fabs(double(actual.matrixB[element]) -
                        actual.matrixA[(element % 3) * 3 + element / 3]));
        }
        else
          maximumBoneTranslationMean = std::max(maximumBoneTranslationMean,
              std::fabs(actual.matrixA[element] - meanA / totalWeight));
      }
      for (int axis = 0; axis < 3; ++axis)
      {
        double inverseTranslation = 0.0;
        for (int sourceAxis = 0; sourceAxis < 3; ++sourceAxis)
          inverseTranslation -= actual.matrixA[9 + sourceAxis] *
              actual.matrixB[sourceAxis * 3 + axis];
        maximumBoneInverse = std::max(maximumBoneInverse,
            std::fabs(actual.matrixB[9 + axis] - inverseTranslation));
      }
    }
    std::printf("animation-fields muscle-topology=%zu curve-max=%.9g influence-topology=%zu projection-max=%.9g bone-topology=%zu bone-A-mean-max=%.9g bone-B-mean-max=%.9g bone-fixed-orientation-max=%.9g bone-translation-mean-max=%.9g bone-inverse-max=%.9g\n",
        muscleTypeMismatch, maximumCurve, influenceTopologyMismatch,
        maximumInfluence, boneTopologyMismatch, maximumBoneMatrixA, maximumBoneMatrixB,
        maximumBoneFixedOrientation, maximumBoneTranslationMean, maximumBoneInverse);
    NativeLifeStudio::HeadData nativeHead;
    if (!NativeLifeStudio::BlendFaceGenAnimationHead(data, weights, &nativeHead) ||
        nativeHead.vertices.size() != reference.vertices.size() ||
        nativeHead.implicitVertices.size() != reference.implicitVertices.size() ||
        nativeHead.muscles.size() != reference.muscles.size() ||
        nativeHead.bones.size() != reference.bones.size()) return 3;
    double nativeCurve = 0.0, nativeProjection = 0.0;
    double nativeFalloff = 0.0, nativeBone = 0.0;
    std::size_t nativeTopology = 0;
    for (std::size_t muscle = 0; muscle < reference.muscles.size(); ++muscle)
    {
      if (nativeHead.muscles[muscle].type != reference.muscles[muscle].type ||
          nativeHead.muscles[muscle].name != reference.muscles[muscle].name)
        ++nativeTopology;
      for (int knot = 0; knot < 5; ++knot)
      {
        nativeCurve = std::max(nativeCurve, std::fabs(double(
            nativeHead.muscles[muscle].falloffX[knot]) -
            reference.muscles[muscle].falloffX[knot]));
        nativeCurve = std::max(nativeCurve, std::fabs(double(
            nativeHead.muscles[muscle].falloffY[knot]) -
            reference.muscles[muscle].falloffY[knot]));
      }
    }
    for (std::size_t slot = 0; slot < reference.vertices.size(); ++slot)
    {
      const auto &a = nativeHead.vertices[slot];
      const auto &b = reference.vertices[slot];
      if (a.index != b.index || a.influences.size() != b.influences.size())
      {
        ++nativeTopology;
        continue;
      }
      for (std::size_t n = 0; n < a.influences.size(); ++n)
      {
        if (a.influences[n].muscleIndex != b.influences[n].muscleIndex)
        {
          ++nativeTopology;
          continue;
        }
        nativeProjection = std::max(nativeProjection, std::fabs(double(
            a.influences[n].componentA) - b.influences[n].componentA));
        nativeFalloff = std::max(nativeFalloff, std::fabs(double(
            a.influences[n].componentB) - b.influences[n].componentB));
      }
    }
    for (std::size_t slot = 0; slot < reference.implicitVertices.size(); ++slot)
      if (nativeHead.implicitVertices[slot].index != reference.implicitVertices[slot].index)
        ++nativeTopology;
    for (std::size_t bone = 0; bone < reference.bones.size(); ++bone)
    {
      if (nativeHead.bones[bone].name != reference.bones[bone].name ||
          nativeHead.bones[bone].muscleIndices != reference.bones[bone].muscleIndices)
        ++nativeTopology;
      for (int element = 0; element < 12; ++element)
      {
        nativeBone = std::max(nativeBone, std::fabs(double(
            nativeHead.bones[bone].matrixA[element]) -
            reference.bones[bone].matrixA[element]));
        nativeBone = std::max(nativeBone, std::fabs(double(
            nativeHead.bones[bone].matrixB[element]) -
            reference.bones[bone].matrixB[element]));
      }
    }
    std::printf("native-animation-head topology=%zu curve-max=%.9g projection-max=%.9g falloff-max=%.9g bone-max=%.9g\n",
        nativeTopology, nativeCurve, nativeProjection, nativeFalloff, nativeBone);
    if (nativeTopology || nativeCurve > 0.0001 || nativeProjection > 0.0001 ||
        nativeFalloff > 0.0001 || nativeBone > 0.0001) return 1;
  }
  return maximumVertex <= 0.0001 && maximumAnchor <= 0.0001 ? 0 : 1;
}
