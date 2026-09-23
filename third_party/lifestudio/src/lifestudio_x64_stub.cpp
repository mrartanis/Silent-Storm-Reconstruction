// Partial native x64 LifeStudio bridge. Original streams and macro-muscle
// effects run without the x86 DLL; local-X/Y/Z bone channels are decoded,
// but full vertex parity remains incomplete.
#include "LifeStudioHeadAPIGDP.h"
#include "LifeStudioHeadAPIMMTS.h"
#include "NativeFaceGenData.h"
#include "NativeHeadData.h"
#include "NativeMMTreeData.h"
#include "NativeMMTreeRuntime.h"
#include "NativeSequenceData.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
std::vector<char> ReadFile(const char *path)
{
  if (!path) return {};
  std::ifstream file(path, std::ios::binary);
  if (!file) return {};
  return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void TransformPoint(const float *matrix, const float *source, float *result)
{
  for (int axis = 0; axis < 3; ++axis)
    result[axis] = source[0] * matrix[axis] + source[1] * matrix[3 + axis] +
                   source[2] * matrix[6 + axis] + matrix[9 + axis];
}

void TransformVector(const float *matrix, const float *source, float *result)
{
  for (int axis = 0; axis < 3; ++axis)
    result[axis] = source[0] * matrix[axis] + source[1] * matrix[3 + axis] +
                   source[2] * matrix[6 + axis];
}

void RotateLocal(float *vector, const std::array<float, 3> &amplitudes)
{
  const float angleX = -amplitudes[2];
  if (angleX != 0.0f)
  {
    const float cosine = std::cos(angleX);
    const float sine = std::sin(angleX);
    const float y = vector[1];
    const float z = vector[2];
    vector[1] = y * cosine - z * sine;
    vector[2] = y * sine + z * cosine;
  }
  const float angleY = -amplitudes[0];
  if (angleY != 0.0f)
  {
    const float cosine = std::cos(angleY);
    const float sine = std::sin(angleY);
    const float x = vector[0];
    const float z = vector[2];
    vector[0] = x * cosine - z * sine;
    vector[2] = x * sine + z * cosine;
  }
  const float angleZ = amplitudes[1];
  if (angleZ != 0.0f)
  {
    const float cosine = std::cos(angleZ);
    const float sine = std::sin(angleZ);
    const float x = vector[0];
    const float y = vector[1];
    vector[0] = x * cosine - y * sine;
    vector[1] = x * sine + y * cosine;
  }
}

void RotateNeckParent(float *vector, const std::array<float, 3> &amplitudes)
{
  // The saved neck attachment counter-rotates against the head's local-Y/Z
  // channels; its local-X channel follows the normal head rotation.
  RotateLocal(vector, {-amplitudes[0], -amplitudes[1], amplitudes[2]});
}
}

namespace LifeStudioHeadAPI
{
struct NativeMacroMuscle
{
  std::string name;
  const NativeLifeStudio::MMTreeOperationRecord *record = nullptr;
  const NativeLifeStudio::MMTreeRoot *root = nullptr;
  const std::vector<char> *bytes = nullptr;
};

const char *NativeMacroMuscleName(IMacroMuscle *muscle)
{
  return muscle ? reinterpret_cast<NativeMacroMuscle *>(muscle)->name.c_str() : nullptr;
}

class AnimatorStub : public IAnimator
{
  NativeLifeStudio::HeadData head;
  std::vector<char> rawBytes;
  std::vector<float> muscleAmplitudes;
  std::vector<float> muscleSignedSquares;
  std::vector<float> muscleAbsoluteSums;
  std::vector<std::array<float, 3>> boneAmplitudes;
  std::vector<std::array<float, 3>> boneSignedSquares;
  std::vector<std::array<float, 3>> boneAbsoluteSums;
  std::vector<std::array<float, 3>> evaluatedPointB;
  std::vector<float> neutralVertices;
  bool applyNeckZones = true;
  bool loaded = false;
  bool fillUnused = false;
public:
  bool AssignHead(const NativeLifeStudio::HeadData &value)
  {
    std::vector<char> saved;
    return NativeLifeStudio::EncodeSavedHead(value, &saved) &&
        saved.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()) &&
        Load(saved.data(), static_cast<int>(saved.size()));
  }
  bool Load(const char *bytes, int size)
  {
    NativeLifeStudio::HeadData parsed;
    if (size < 0 || !NativeLifeStudio::DecodeHeadVertices(bytes, static_cast<std::size_t>(size), &parsed))
    {
      head = {};
      rawBytes.clear();
      muscleAmplitudes.clear();
      muscleSignedSquares.clear();
      muscleAbsoluteSums.clear();
      boneAmplitudes.clear();
      boneSignedSquares.clear();
      boneAbsoluteSums.clear();
      evaluatedPointB.clear();
      neutralVertices.clear();
      loaded = false;
      return false;
    }
    head = std::move(parsed);
    rawBytes.assign(bytes, bytes + size);
    muscleAmplitudes.assign(head.muscles.size(), 0.0f);
    muscleSignedSquares.assign(head.muscles.size(), 0.0f);
    muscleAbsoluteSums.assign(head.muscles.size(), 0.0f);
    boneAmplitudes.assign(head.bones.size(), {0.0f, 0.0f, 0.0f});
    boneSignedSquares.assign(head.bones.size(), {0.0f, 0.0f, 0.0f});
    boneAbsoluteSums.assign(head.bones.size(), {0.0f, 0.0f, 0.0f});
    evaluatedPointB.resize(head.muscles.size());
    for (std::size_t i = 0; i < head.muscles.size(); ++i)
      for (int axis = 0; axis < 3; ++axis)
        evaluatedPointB[i][axis] = head.muscles[i].pointB[axis];
    loaded = true;
    neutralVertices.clear();
    if (head.hasNeckAppendix)
    {
      neutralVertices.resize(std::size_t(head.vertexCount) * 3);
      applyNeckZones = false;
      const bool neutralOk = Process(neutralVertices.data(), 3);
      applyNeckZones = true;
      if (!neutralOk)
      {
        loaded = false;
        neutralVertices.clear();
        return false;
      }
    }
    return true;
  }
  int SaveBufferSize() { return loaded ? static_cast<int>(rawBytes.size()) : 0; }
  bool Save(char *output)
  {
    if (!loaded || !output || rawBytes.empty()) return false;
    std::memcpy(output, rawBytes.data(), rawBytes.size());
    return true;
  }
  IMuscle *MuscleByName(const char *) { return 0; }
  IMuscle *Muscle(int) { return 0; }
  int MusclesCount() const { return loaded ? static_cast<int>(head.muscleCount) : 0; }
  IBone *BoneByName(const char *) { return 0; }
  IBone *Bone(int) { return 0; }
  IBone *BoneByType(unsigned long, IBone *) { return 0; }
  int BonesCount() const { return loaded ? static_cast<int>(head.boneCount) : 0; }
  void FillUnused(bool fill) { fillUnused = fill; }
  bool FillUnused() const { return fillUnused; }
  bool Process(float *output, int step)
  {
    if (!loaded || !output || step < 3 || step > 1024)
      return false;
    int neckParent = -1;
    if (head.hasNeckAppendix)
      for (std::size_t i = 0; i < head.bones.size(); ++i)
        if (head.bones[i].name == "a_Head_ROT")
        {
          neckParent = static_cast<int>(i);
          break;
        }
    for (const auto &vertex : head.vertices)
    {
      const std::size_t influenceCount = vertex.influences.size();
      std::vector<std::array<float, 3>> shifts(influenceCount, {0.0f, 0.0f, 0.0f});
      std::vector<float> lengths(influenceCount, 0.0f);
      for (std::size_t i = 0; i < influenceCount; ++i)
      {
        const auto &influence = vertex.influences[i];
        if (influence.muscleIndex >= head.muscles.size() ||
            influence.componentA <= 0.0f || influence.componentB <= 0.0f)
          continue;
        const std::size_t index = influence.muscleIndex;
        const float weight = influence.componentA * influence.componentB;
        for (int axis = 0; axis < 3; ++axis)
          shifts[i][axis] = weight *
              (evaluatedPointB[index][axis] - head.muscles[index].pointB[axis]);
        // A muscle attached to a bone emits a vector in that bone's rotated
        // frame. The original DLL transforms this vector separately from the
        // vertex position, without the bone's translation.
        for (std::size_t boneIndex = 0; boneIndex < head.bones.size(); ++boneIndex)
        {
          const auto &bone = head.bones[boneIndex];
          bool attached = false;
          for (std::uint32_t member : bone.muscleIndices)
            if (member == index) { attached = true; break; }
          if (!attached) continue;
          float intermediate[3];
          float transformed[3];
          TransformVector(bone.matrixB, shifts[i].data(), intermediate);
          if (bone.name == "a_Neck_ROT")
            RotateNeckParent(intermediate, boneAmplitudes[boneIndex]);
          else
            RotateLocal(intermediate, boneAmplitudes[boneIndex]);
          TransformVector(bone.matrixA, intermediate, transformed);
          for (int axis = 0; axis < 3; ++axis)
            shifts[i][axis] = transformed[axis];
          if (bone.name == "a_Neck_ROT" && neckParent >= 0)
          {
            const auto &parent = head.bones[neckParent];
            TransformVector(parent.matrixB, shifts[i].data(), intermediate);
            RotateNeckParent(intermediate, boneAmplitudes[neckParent]);
            TransformVector(parent.matrixA, intermediate, transformed);
            for (int axis = 0; axis < 3; ++axis)
              shifts[i][axis] = transformed[axis];
          }
          break;
        }
        lengths[i] = std::sqrt(shifts[i][0] * shifts[i][0] +
                               shifts[i][1] * shifts[i][1] +
                               shifts[i][2] * shifts[i][2]);
      }
      float position[3] = {};
      // The original x86 multi-influence path damps mutually aligned
      // displacements. Its per-vertex coefficient is -ln(n)/(2*(n-1)).
      const double interactionCoefficient = influenceCount > 1
          ? -std::log(static_cast<double>(influenceCount)) /
                (2.0 * static_cast<double>(influenceCount - 1))
          : 0.0;
      for (std::size_t i = 0; i < influenceCount; ++i)
      {
        const auto &influence = vertex.influences[i];
        const float length = lengths[i];
        if (length <= 0.0f) continue;
        double factor = 1.0;
        if (influenceCount > 1)
        {
          const float denominator = influence.componentB * length;
          if (denominator < 0.0001f) continue;
          double overlap = 0.0;
          if (length >= 0.0001f)
            for (std::size_t j = 0; j < influenceCount; ++j)
            {
              if (j == i) continue;
              const auto &other = shifts[j];
              const double dot = (static_cast<double>(shifts[i][0]) * other[0] +
                                  static_cast<double>(shifts[i][1]) * other[1] +
                                  static_cast<double>(shifts[i][2]) * other[2]) / length;
              overlap += std::fabs(dot * vertex.influences[j].componentB);
            }
          factor = std::exp(interactionCoefficient * overlap / denominator);
        }
        for (int axis = 0; axis < 3; ++axis)
          position[axis] += static_cast<float>(factor * shifts[i][axis]);
      }
      float weighted[3] = {};
      float totalWeight = 0.0f;
      float specifiedWeight = 0.0f;
      for (const auto &influence : vertex.influences)
        if (influence.componentB > 0.0f) specifiedWeight += influence.componentB;
      // Each influence's componentB is its bone-blend weight. A non-bone
      // influence contributes the untransformed position to the same blend.
      // When every serialized weight is zero, x86 uses an equal-weight blend.
      for (const auto &influence : vertex.influences)
      {
        const float weight = specifiedWeight > 0.0f ? influence.componentB : 1.0f;
        if (weight <= 0.0f) continue;
        float transformed[3];
        float intermediate[3];
        const float *candidate = vertex.sourcePosition;
        for (const auto &bone : head.bones)
        {
          bool attached = false;
          for (std::uint32_t index : bone.muscleIndices)
            if (index == influence.muscleIndex) { attached = true; break; }
          if (!attached)
            continue;
          TransformPoint(bone.matrixB, vertex.sourcePosition, intermediate);
          const std::size_t boneIndex = static_cast<std::size_t>(&bone - head.bones.data());
          if (bone.name == "a_Neck_ROT")
            RotateNeckParent(intermediate, boneAmplitudes[boneIndex]);
          else
            RotateLocal(intermediate, boneAmplitudes[boneIndex]);
          TransformPoint(bone.matrixA, intermediate, transformed);
          candidate = transformed;
          if (bone.name == "a_Neck_ROT" && neckParent >= 0)
          {
            const auto &parent = head.bones[neckParent];
            TransformPoint(parent.matrixB, candidate, intermediate);
            RotateNeckParent(intermediate, boneAmplitudes[neckParent]);
            TransformPoint(parent.matrixA, intermediate, transformed);
            candidate = transformed;
          }
          break;
        }
        for (int axis = 0; axis < 3; ++axis)
          weighted[axis] += weight * candidate[axis];
        totalWeight += weight;
      }
      if (totalWeight > 0.0f)
        for (int axis = 0; axis < 3; ++axis)
          position[axis] += weighted[axis] / totalWeight;
      else
        for (int axis = 0; axis < 3; ++axis)
          position[axis] += vertex.sourcePosition[axis];
      for (int axis = 0; axis < 3; ++axis)
        output[std::size_t(vertex.index) * step + axis] = position[axis];
    }
    for (const auto &vertex : head.implicitVertices)
      for (int axis = 0; axis < 3; ++axis)
        output[std::size_t(vertex.index) * step + axis] = vertex.sourcePosition[axis];
    if (applyNeckZones && head.hasNeckAppendix && neckParent >= 0 &&
        neutralVertices.size() == std::size_t(head.vertexCount) * 3)
    {
      struct ControlLine
      {
        std::array<float, 3> sourceLower{}, sourceUpper{};
        std::array<float, 3> movedLower{}, movedUpper{};
      };
      std::array<ControlLine, 9> lines;
      std::vector<float> base(std::size_t(head.vertexCount) * 3);
      for (std::size_t vertex = 0; vertex < head.vertexCount; ++vertex)
        for (int axis = 0; axis < 3; ++axis)
          base[vertex * 3 + axis] = output[vertex * step + axis];
      for (std::size_t zone = 0; zone < 8; ++zone)
      {
        const auto &record = head.neckZones[zone];
        for (int axis = 0; axis < 3; ++axis)
        {
          lines[zone].sourceLower[axis] = neutralVertices[std::size_t(record.lowerVertex) * 3 + axis];
          lines[zone].sourceUpper[axis] = neutralVertices[std::size_t(record.upperVertex) * 3 + axis];
          lines[zone].movedLower[axis] = base[std::size_t(record.lowerVertex) * 3 + axis];
          lines[zone].movedUpper[axis] = base[std::size_t(record.upperVertex) * 3 + axis];
        }
      }
      const auto &parent = head.bones[neckParent];
      const auto neckBone = std::find_if(head.bones.begin(), head.bones.end(),
          [](const NativeLifeStudio::BoneRecord &bone) { return bone.name == "a_Neck_ROT"; });
      if (neckBone == head.bones.end()) return false;
      const float *neckPivot = neckBone->matrixA + 9;
      float intermediate[3];
      TransformPoint(parent.matrixB, neckPivot, intermediate);
      float sourceCenter[3];
      TransformPoint(parent.matrixA, intermediate, sourceCenter);
      TransformPoint(parent.matrixB, neckPivot, intermediate);
      RotateNeckParent(intermediate, boneAmplitudes[neckParent]);
      float movedCenter[3];
      TransformPoint(parent.matrixA, intermediate, movedCenter);
      for (int axis = 0; axis < 3; ++axis)
      {
        lines[8].sourceLower[axis] = sourceCenter[axis];
        lines[8].sourceUpper[axis] = parent.matrixA[9 + axis];
        lines[8].movedLower[axis] = movedCenter[axis];
        lines[8].movedUpper[axis] = parent.matrixA[9 + axis];
      }
      std::vector<std::array<double, 3>> accumulated(head.vertexCount, {0.0, 0.0, 0.0});
      std::vector<int> counts(head.vertexCount, 0);
      for (std::size_t zone = 0; zone < 8; ++zone)
        for (std::size_t vertex = 0; vertex < head.vertexCount; ++vertex)
        {
          if (!(head.neckZones[zone].vertexMask[vertex / 8] & (1u << (vertex % 8)))) continue;
          const std::array<std::size_t, 3> nodes = {zone, (zone + 1) % 8, 8};
          // Retail caches these coordinates in the neutral pose unless
          // NeckProcessing2 is explicitly enabled. Re-evaluating them from
          // the morphed base vertex applies facial-muscle motion twice.
          const float *point = neutralVertices.data() + vertex * 3;
          std::array<double, 3> t{}, x{}, y{};
          for (int j = 0; j < 3; ++j)
          {
            const auto &line = lines[nodes[j]];
            const double dz = double(line.sourceUpper[2]) - line.sourceLower[2];
            if (std::fabs(dz) < 1e-6) return false;
            t[j] = (double(point[2]) - line.sourceLower[2]) / dz;
            x[j] = line.sourceLower[0] + t[j] *
                (double(line.sourceUpper[0]) - line.sourceLower[0]);
            y[j] = line.sourceLower[1] + t[j] *
                (double(line.sourceUpper[1]) - line.sourceLower[1]);
          }
          const double denominator = (x[1] - x[0]) * (y[2] - y[0]) -
                                     (x[2] - x[0]) * (y[1] - y[0]);
          if (std::fabs(denominator) < 1e-9) return false;
          const double w1 = ((double(point[0]) - x[0]) * (y[2] - y[0]) -
                             (x[2] - x[0]) * (double(point[1]) - y[0])) / denominator;
          const double w2 = ((x[1] - x[0]) * (double(point[1]) - y[0]) -
                             (double(point[0]) - x[0]) * (y[1] - y[0])) / denominator;
          const std::array<double, 3> weights = {1.0 - w1 - w2, w1, w2};
          for (int j = 0; j < 3; ++j)
            for (int axis = 0; axis < 3; ++axis)
            {
              const auto &line = lines[nodes[j]];
              accumulated[vertex][axis] += weights[j] *
                  (line.movedLower[axis] + t[j] *
                      (double(line.movedUpper[axis]) - line.movedLower[axis]));
            }
          ++counts[vertex];
        }
      for (std::size_t vertex = 0; vertex < head.vertexCount; ++vertex)
        if (counts[vertex] > 0)
          for (int axis = 0; axis < 3; ++axis)
            output[vertex * step + axis] =
                static_cast<float>(accumulated[vertex][axis] / counts[vertex]);
    }
    return true;
  }
  int VerticesCount() const { return loaded ? static_cast<int>(head.vertexCount) : 0; }
  float Amplitude(int index) const
  {
    return index >= 0 && static_cast<std::size_t>(index) < muscleAmplitudes.size()
             ? muscleAmplitudes[index] : 0.0f;
  }
  int NativeMuscleCount() const { return static_cast<int>(muscleAmplitudes.size()); }
  void ClearAllMacroMuscles()
  {
    std::fill(muscleAmplitudes.begin(), muscleAmplitudes.end(), 0.0f);
    std::fill(muscleSignedSquares.begin(), muscleSignedSquares.end(), 0.0f);
    std::fill(muscleAbsoluteSums.begin(), muscleAbsoluteSums.end(), 0.0f);
    std::fill(boneAmplitudes.begin(), boneAmplitudes.end(),
              std::array<float, 3>{0.0f, 0.0f, 0.0f});
    std::fill(boneSignedSquares.begin(), boneSignedSquares.end(),
              std::array<float, 3>{0.0f, 0.0f, 0.0f});
    std::fill(boneAbsoluteSums.begin(), boneAbsoluteSums.end(),
              std::array<float, 3>{0.0f, 0.0f, 0.0f});
    for (std::size_t i = 0; i < head.muscles.size(); ++i)
      for (int axis = 0; axis < 3; ++axis)
        evaluatedPointB[i][axis] = head.muscles[i].pointB[axis];
  }
  void AddMacroMuscle(IMacroMuscle *muscle, float expression)
  {
    if (!loaded || !muscle || !std::isfinite(expression)) return;
    expression = std::max(-1.0f, std::min(1.0f, expression));
    const auto *macro = reinterpret_cast<NativeMacroMuscle *>(muscle);
    if (!macro->root || !macro->bytes || macro->bytes->empty()) return;
    std::vector<NativeLifeStudio::MMTreeEffectSample> effects;
    if (!NativeLifeStudio::EvaluateMMTreeMacro(macro->bytes->data(), macro->bytes->size(),
                                               *macro->root, macro->name,
                                               expression, &effects))
      return;
    for (const auto &effect : effects)
    {
      if (effect.kind == 3)
        for (std::size_t i = 0; i < head.muscles.size(); ++i)
          if (head.muscles[i].name == effect.targetName)
          {
            // The x86 effect callback accumulates signed squares and absolute
            // values separately; ComputePhysics divides the former by the
            // latter. Summing leaf values breaks shared expression graphs.
            muscleSignedSquares[i] += std::copysign(
                effect.expression * effect.expression, effect.expression);
            muscleAbsoluteSums[i] += std::fabs(effect.expression);
            break;
          }
      // Bone axis comes from the referenced definition's runtime type.
      // Different definitions can share a name and serialized channel.
      if (effect.kind == 4 && effect.runtimeType >= 1 && effect.runtimeType <= 3)
        for (std::size_t i = 0; i < head.bones.size(); ++i)
          if (head.bones[i].name == effect.targetName)
          {
            const int axis = effect.runtimeType == 2 ? 0 :
                             effect.runtimeType == 1 ? 1 : 2;
            boneSignedSquares[i][axis] += std::copysign(
                effect.expression * effect.expression, effect.expression);
            boneAbsoluteSums[i][axis] += std::fabs(effect.expression);
            break;
          }
    }
  }
  void MultMacroMuscle(IMacroMuscle *, float) {}
  void ComputePhysics()
  {
    if (!loaded) return;
    for (std::size_t i = 0; i < head.bones.size(); ++i)
      for (int axis = 0; axis < 3; ++axis)
        boneAmplitudes[i][axis] = boneAbsoluteSums[i][axis] >= 0.0001f
            ? boneSignedSquares[i][axis] / boneAbsoluteSums[i][axis]
            : 0.0f;
    for (std::size_t i = 0; i < head.muscles.size(); ++i)
    {
      float &amplitude = muscleAmplitudes[i];
      amplitude = muscleAbsoluteSums[i] >= 0.0001f
                      ? muscleSignedSquares[i] / muscleAbsoluteSums[i]
                      : 0.0f;
      for (int axis = 0; axis < 3; ++axis)
        // Match the original x86 evaluation order. The algebraically equal
        // A + (1-a)*(B-A) can round one ULP higher; at a vertex influence's
        // 1e-4 activation boundary that changes whether the muscle moves.
        evaluatedPointB[i][axis] = head.muscles[i].pointB[axis] -
                                    amplitude *
                                    (head.muscles[i].pointB[axis] - head.muscles[i].pointA[axis]);
    }
  }
  void RegisterMacroMuscle(IMacroMuscle *) {}
  void UnregisterMacroMuscle(IMacroMuscle *) {}
  void ClearAllRegistration() {}
  void CollectUserItems(bool) {}
  bool CollectUserItems() const { return false; }
  UserID UserItem(const char *) { return -1; }
  int UserValuesCount(UserID) { return 0; }
  float UserValue(UserID, int) { return 0.0f; }
  void ClearUserItems() {}
  void ComputeBonesHierarchy() {}
  bool HasNeck() const { return loaded && head.hasNeckAppendix; }
  void NeckProcessing2(bool) {}
  bool NeckProcessing2() const { return false; }
  IAnimator *Clone() { return new AnimatorStub(*this); }
  void Destroy() { delete this; }
};

float NativeAnimatorMuscleAmplitude(IAnimator *animator, int index)
{
  return animator ? static_cast<AnimatorStub *>(animator)->Amplitude(index) : 0.0f;
}

int NativeAnimatorMuscleCount(IAnimator *animator)
{
  return animator ? static_cast<AnimatorStub *>(animator)->NativeMuscleCount() : 0;
}

class TransformerStub : public ITransformer
{
  NativeLifeStudio::FaceGenData faceGen;
  NativeMacroMuscle *rootMacro = nullptr;
  std::vector<std::pair<NativeMacroMuscle *, float>> sliders;
  AnimatorStub *output = nullptr;
  bool loaded = false;
  bool computed = false;
  bool collectItems = false;
  std::vector<std::string> userItemNames;
  std::vector<std::vector<float>> userItemValues;
  std::unordered_map<std::string, UserID> userItemIds;
public:
  bool Load(const char *, int) { return false; }
  int SaveBufferSize() { return 0; }
  bool Save(char *) { return false; }
  IMuscle *MuscleByName(const char *) { return 0; }
  IMuscle *Muscle(int) { return 0; }
  int MusclesCount() const { return 0; }
  IBone *BoneByName(const char *) { return 0; }
  IBone *Bone(int) { return 0; }
  IBone *BoneByType(unsigned long, IBone *) { return 0; }
  int BonesCount() const { return 0; }
  void FillUnused(bool) {}
  bool FillUnused() const { return false; }
  bool Process(float *, int) { return false; }
  int VerticesCount() const { return 0; }
  void ClearAllMacroMuscles()
  {
    sliders.clear();
    computed = false;
    ClearUserItems();
  }
  void AddMacroMuscle(IMacroMuscle *muscle, float expression)
  {
    if (muscle && std::isfinite(expression))
    {
      sliders.emplace_back(reinterpret_cast<NativeMacroMuscle *>(muscle), expression);
      computed = false;
    }
  }
  void MultMacroMuscle(IMacroMuscle *, float) {}
  void ComputePhysics() { computed = true; }
  void RegisterMacroMuscle(IMacroMuscle *muscle)
  {
    rootMacro = reinterpret_cast<NativeMacroMuscle *>(muscle);
  }
  void UnregisterMacroMuscle(IMacroMuscle *muscle)
  {
    if (reinterpret_cast<NativeMacroMuscle *>(muscle) == rootMacro) rootMacro = nullptr;
  }
  void ClearAllRegistration() { rootMacro = nullptr; }
  void CollectUserItems(bool value) { collectItems = value; if (!value) ClearUserItems(); }
  bool CollectUserItems() const { return collectItems; }
  UserID UserItem(const char *name)
  {
    if (!name || !collectItems) return -1;
    const auto found = userItemIds.find(name);
    return found == userItemIds.end() ? -1 : found->second;
  }
  int UserValuesCount(UserID id)
  {
    return id >= 0 && static_cast<std::size_t>(id) < userItemValues.size()
        ? static_cast<int>(userItemValues[id].size()) : 0;
  }
  float UserValue(UserID id, int number)
  {
    return id >= 0 && static_cast<std::size_t>(id) < userItemValues.size() &&
                   number >= 0 && static_cast<std::size_t>(number) < userItemValues[id].size()
        ? userItemValues[id][number] : 0.0f;
  }
  void ClearUserItems()
  {
    userItemNames.clear();
    userItemValues.clear();
    userItemIds.clear();
  }
  void ComputeBonesHierarchy() {}
  bool HasNeck() const { return false; }
  void NeckProcessing2(bool) {}
  bool NeckProcessing2() const { return false; }
  IAnimator *Clone() { return new AnimatorStub; }
  void Destroy() { delete this; }
  bool Load(ITransformerInput *input)
  {
    NativeLifeStudio::FaceGenData parsed;
    loaded = NativeLifeStudio::LoadFaceGenData(input, &parsed);
    faceGen = loaded ? std::move(parsed) : NativeLifeStudio::FaceGenData{};
    sliders.clear();
    computed = false;
    ClearUserItems();
    return loaded;
  }
  void OutputAnimator(IAnimator *animator)
  {
    output = dynamic_cast<AnimatorStub *>(animator);
  }
  IAnimator *OutputAnimator() const { return output; }
  void Generate()
  {
    if (!loaded || !computed || !output || !rootMacro ||
        !rootMacro->root || !rootMacro->bytes || rootMacro->bytes->empty()) return;
    ClearUserItems();
    if (collectItems)
      for (const auto &slider : sliders)
      {
        std::vector<NativeLifeStudio::MMTreeEffectSample> effects;
        if (!NativeLifeStudio::EvaluateMMTreeMacro(slider.first->bytes->data(),
                slider.first->bytes->size(), *slider.first->root, slider.first->name,
                std::max(-1.0f, std::min(1.0f, slider.second)), &effects)) return;
        for (const auto &effect : effects)
          if (effect.kind == 5)
          {
            auto found = userItemIds.find(effect.targetName);
            if (found == userItemIds.end())
            {
              const UserID id = static_cast<UserID>(userItemNames.size());
              userItemNames.push_back(effect.targetName);
              userItemValues.emplace_back();
              found = userItemIds.emplace(effect.targetName, id).first;
            }
            userItemValues[found->second].push_back(
                std::max(-1.0f, std::min(1.0f, effect.expression)));
          }
      }
    std::vector<std::pair<std::string, float>> controls;
    controls.reserve(sliders.size());
    for (const auto &slider : sliders)
      controls.emplace_back(slider.first->name, slider.second);
    std::vector<float> weights;
    if (!NativeLifeStudio::SelectGameFaceGenWeights(
            rootMacro->bytes->data(), rootMacro->bytes->size(),
            *rootMacro->root, controls, &weights)) return;
    NativeLifeStudio::HeadData animationBase, morphBase, generated;
    if (!NativeLifeStudio::BlendFaceGenAnimationHead(faceGen, weights, &animationBase) ||
        !NativeLifeStudio::BlendFaceGenMorphHead(faceGen, weights, &morphBase)) return;
    AnimatorStub morph;
    if (!morph.AssignHead(morphBase)) return;
    for (const auto &slider : sliders)
      morph.AddMacroMuscle(reinterpret_cast<IMacroMuscle *>(slider.first), slider.second);
    morph.ComputePhysics();
    std::vector<float> positions(std::size_t(morphBase.vertexCount) * 3);
    if (!morph.Process(positions.data(), 3)) return;
    std::vector<std::array<float, 3>> processed(morphBase.vertexCount);
    for (std::size_t vertex = 0; vertex < processed.size(); ++vertex)
      for (int axis = 0; axis < 3; ++axis)
        processed[vertex][axis] = positions[vertex * 3 + axis];
    if (!NativeLifeStudio::ComposeFaceGenOutputHead(
            animationBase, morphBase, processed, &generated)) return;
    output->AssignHead(generated);
  }
};

class MMTreeStub : public IMMTree
{
  NativeLifeStudio::MMTreeRoot root;
  std::vector<char> rawBytes;
  std::vector<std::unique_ptr<NativeMacroMuscle>> macroObjects;
  std::unordered_map<std::string, NativeMacroMuscle *> byName;
  NativeMacroMuscle *rootMacro = nullptr;
  bool loaded = false;
  bool AddMacro(const std::string &name, const NativeLifeStudio::MMTreeOperationRecord *record)
  {
    if (name.empty() || byName.find(name) != byName.end())
      return false;
    auto object = std::make_unique<NativeMacroMuscle>();
    object->name = name;
    object->record = record;
    object->root = &root;
    object->bytes = &rawBytes;
    NativeMacroMuscle *pointer = object.get();
    macroObjects.push_back(std::move(object));
    byName.emplace(name, pointer);
    if (!record) rootMacro = pointer;
    return true;
  }
  bool AddDescendants(const NativeLifeStudio::MMTreeOperationRecord &record)
  {
    if (record.headerWords[0] == 1 && !record.name.empty() &&
        !AddMacro(record.name, &record))
      return false;
    for (const auto &child : record.children)
      if (!AddDescendants(child))
        return false;
    return true;
  }
public:
  bool Load(const char *path)
  {
    const auto bytes = ReadFile(path);
    return !bytes.empty() && Load(bytes.data(), static_cast<int>(bytes.size()));
  }
  bool Load(const char *bytes, int size)
  {
    NativeLifeStudio::MMTreeRoot parsed;
    loaded = size >= 0 && NativeLifeStudio::DecodeMMTreeRoot(bytes, static_cast<std::size_t>(size), &parsed) &&
             NativeLifeStudio::ValidateMMTreeCurves(bytes, static_cast<std::size_t>(size), parsed);
    root = loaded ? std::move(parsed) : NativeLifeStudio::MMTreeRoot{};
    rawBytes = loaded ? std::vector<char>(bytes, bytes + size) : std::vector<char>{};
    macroObjects.clear();
    byName.clear();
    rootMacro = nullptr;
    if (loaded)
    {
      loaded = AddMacro(root.name, nullptr);
      for (const auto &operation : root.operations)
        if (loaded && !AddDescendants(operation)) loaded = false;
    }
    if (!loaded)
    {
      root = {};
      rawBytes.clear();
      macroObjects.clear();
      byName.clear();
      rootMacro = nullptr;
    }
    return loaded;
  }
  IMacroMuscle *RootMacroMuscle() const { return reinterpret_cast<IMacroMuscle *>(rootMacro); }
  IMacroMuscle *FindMacroMuscle(const char *name)
  {
    if (!name) return nullptr;
    const auto found = byName.find(name);
    return found == byName.end() ? nullptr : reinterpret_cast<IMacroMuscle *>(found->second);
  }
  void Destroy() { delete this; }
};

class SequencerStub : public ISequencer
{
  NativeLifeStudio::SequenceHeader header;
  std::vector<NativeLifeStudio::SequenceTrack> tracks;
  IMMTree *tree = nullptr;
  bool loaded = false;
public:
  bool Load(const char *path)
  {
    const auto bytes = ReadFile(path);
    return !bytes.empty() && Load(bytes.data(), static_cast<int>(bytes.size()));
  }
  bool Load(const char *bytes, int size)
  {
    NativeLifeStudio::SequenceHeader parsed;
    std::vector<NativeLifeStudio::SequenceTrack> parsedTracks;
    if (size < 0 || !NativeLifeStudio::DecodeSequenceHeader(bytes, static_cast<std::size_t>(size), &parsed) ||
        !NativeLifeStudio::DecodeSequenceTracks(bytes, static_cast<std::size_t>(size), &parsedTracks) ||
        parsed.duration > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        parsed.trackCount > static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
    {
      header = {};
      tracks.clear();
      loaded = false;
      return false;
    }
    header = parsed;
    tracks = std::move(parsedTracks);
    loaded = true;
    return true;
  }
  IMMTree *RegisterMMTree(IMMTree *value) { IMMTree *previous = tree; tree = value; return previous; }
  int SequenceTime() const { return loaded ? static_cast<int>(header.duration) : 0; }
  int TracksCount() const { return loaded ? static_cast<int>(tracks.size()) : 0; }
  int EnumerateMacroMuscles(MUSCLE_CB, void *) { return 0; }
  int EnumerateMacroMuscles(int, MUSCLE_EXPR_CB, void *) { return 0; }
  int EnumerateMacroMuscles(MUSCLE_NAME_CB, void *) { return 0; }
  int EnumerateMacroMuscles(int, MUSCLE_NAME_EXPR_CB, void *) { return 0; }
  int EnumerateSounds(SOUND_CB, void *) { return 0; }
  int EnumerateSounds(int, SOUND_TIME_CB, void *) { return 0; }
  void RenderMacroMuscles(IAnimator *animator, int time)
  {
    if (!loaded || !tree || !animator || time < 0 ||
        static_cast<std::uint32_t>(time) >= header.duration)
      return;
    for (const auto &track : tracks)
    {
      if (!track.macroEventsDecoded) continue;
      for (const auto &event : track.macroEvents)
      {
        float expression = 0.0f;
        if (!NativeLifeStudio::EvaluateMacroEvent(event,
              static_cast<std::uint32_t>(time), &expression))
          continue;
        IMacroMuscle *muscle = tree->FindMacroMuscle(event.name.c_str());
        if (muscle) animator->AddMacroMuscle(muscle, expression);
      }
    }
  }
  void Destroy() { delete this; }
};

IAnimator *__stdcall IAnimator::Create() { return new AnimatorStub; }
IMMTree *__stdcall IMMTree::Create() { return new MMTreeStub; }
ISequencer *__stdcall ISequencer::Create() { return new SequencerStub; }
ITransformer *__stdcall ITransformer::Create() { return new TransformerStub; }
}
