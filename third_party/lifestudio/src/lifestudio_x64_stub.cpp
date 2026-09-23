// Partial native x64 LifeStudio bridge. Original streams and macro-muscle
// effects run without the x86 DLL; bone effects and full vertex parity remain.
#include "LifeStudioHeadAPIGDP.h"
#include "LifeStudioHeadAPIMMTS.h"
#include "NativeHeadData.h"
#include "NativeMMTreeData.h"
#include "NativeMMTreeRuntime.h"
#include "NativeSequenceData.h"
#include <algorithm>
#include <array>
#include <cmath>
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
  std::vector<float> muscleAmplitudes;
  std::vector<std::array<float, 3>> evaluatedPointB;
  bool loaded = false;
  bool fillUnused = false;
public:
  bool Load(const char *bytes, int size)
  {
    NativeLifeStudio::HeadData parsed;
    if (size < 0 || !NativeLifeStudio::DecodeHeadVertices(bytes, static_cast<std::size_t>(size), &parsed))
    {
      head = {};
      muscleAmplitudes.clear();
      evaluatedPointB.clear();
      loaded = false;
      return false;
    }
    head = std::move(parsed);
    muscleAmplitudes.assign(head.muscles.size(), 0.0f);
    evaluatedPointB.resize(head.muscles.size());
    for (std::size_t i = 0; i < head.muscles.size(); ++i)
      for (int axis = 0; axis < 3; ++axis)
        evaluatedPointB[i][axis] = head.muscles[i].pointB[axis];
    loaded = true;
    return true;
  }
  int SaveBufferSize() { return 0; }
  bool Save(char *) { return false; }
  IMuscle *MuscleByName(const char *) { return 0; }
  IMuscle *Muscle(int) { return 0; }
  int MusclesCount() const { return 0; }
  IBone *BoneByName(const char *) { return 0; }
  IBone *Bone(int) { return 0; }
  IBone *BoneByType(unsigned long, IBone *) { return 0; }
  int BonesCount() const { return 0; }
  void FillUnused(bool fill) { fillUnused = fill; }
  bool FillUnused() const { return fillUnused; }
  bool Process(float *output, int step)
  {
    if (!loaded || !output || step < 3 || step > 1024)
      return false;
    for (const auto &vertex : head.vertices)
    {
      float changed[3] = {vertex.sourcePosition[0], vertex.sourcePosition[1],
                          vertex.sourcePosition[2]};
      for (const auto &influence : vertex.influences)
      {
        if (influence.muscleIndex >= head.muscles.size() ||
            influence.componentA <= 0.0f || influence.componentB <= 0.0f)
          continue;
        const std::size_t index = influence.muscleIndex;
        const float weight = influence.componentA * influence.componentB;
        for (int axis = 0; axis < 3; ++axis)
          changed[axis] += weight *
              (evaluatedPointB[index][axis] - head.muscles[index].pointB[axis]);
      }
      const float *position = changed;
      float transformed[3];
      float intermediate[3];
      // The original head stream maps muscles to bones. A vertex controlled
      // by one attached muscle receives the bone's stored B-then-A transform.
      // Multi-influence blending and animated muscle state are still pending.
      if (vertex.influences.size() == 1)
      {
        const std::uint32_t muscle = vertex.influences[0].muscleIndex;
        for (const auto &bone : head.bones)
        {
          bool attached = false;
          for (std::uint32_t index : bone.muscleIndices)
            if (index == muscle) { attached = true; break; }
          if (!attached)
            continue;
          TransformPoint(bone.matrixB, position, intermediate);
          TransformPoint(bone.matrixA, intermediate, transformed);
          position = transformed;
          break;
        }
      }
      for (int axis = 0; axis < 3; ++axis)
        output[std::size_t(vertex.index) * step + axis] = position[axis];
    }
    for (const auto &vertex : head.implicitVertices)
      for (int axis = 0; axis < 3; ++axis)
        output[std::size_t(vertex.index) * step + axis] = vertex.sourcePosition[axis];
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
    for (std::size_t i = 0; i < head.muscles.size(); ++i)
      for (int axis = 0; axis < 3; ++axis)
        evaluatedPointB[i][axis] = head.muscles[i].pointB[axis];
  }
  void AddMacroMuscle(IMacroMuscle *muscle, float expression)
  {
    if (!loaded || !muscle || !std::isfinite(expression)) return;
    const auto *macro = reinterpret_cast<NativeMacroMuscle *>(muscle);
    if (!macro->root || !macro->bytes || macro->bytes->empty()) return;
    std::vector<NativeLifeStudio::MMTreeEffectSample> effects;
    if (!NativeLifeStudio::EvaluateMMTreeMacro(macro->bytes->data(), macro->bytes->size(),
                                               *macro->root, macro->name,
                                               expression, &effects))
      return;
    for (const auto &effect : effects)
    {
      if (effect.kind != 3) continue;
      for (std::size_t i = 0; i < head.muscles.size(); ++i)
        if (head.muscles[i].name == effect.targetName)
        {
          muscleAmplitudes[i] += effect.expression;
          break;
        }
    }
  }
  void MultMacroMuscle(IMacroMuscle *, float) {}
  void ComputePhysics()
  {
    if (!loaded) return;
    for (std::size_t i = 0; i < head.muscles.size(); ++i)
    {
      float &amplitude = muscleAmplitudes[i];
      if (std::fabs(amplitude) < 0.0001f) amplitude = 0.0f;
      for (int axis = 0; axis < 3; ++axis)
        evaluatedPointB[i][axis] = head.muscles[i].pointA[axis] +
                                    (1.0f - amplitude) *
                                    (head.muscles[i].pointB[axis] - head.muscles[i].pointA[axis]);
    }
  }
  void RegisterMacroMuscle(IMacroMuscle *) {}
  void UnregisterMacroMuscle(IMacroMuscle *) {}
  void ClearAllRegistration() {}
  void CollectUserItems(bool) {}
  bool CollectUserItems() const { return false; }
  UserID UserItem(const char *) { return 0; }
  int UserValuesCount(UserID) { return 0; }
  float UserValue(UserID, int) { return 0.0f; }
  void ClearUserItems() {}
  void ComputeBonesHierarchy() {}
  bool HasNeck() const { return false; }
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
  void ClearAllMacroMuscles() {}
  void AddMacroMuscle(IMacroMuscle *, float) {}
  void MultMacroMuscle(IMacroMuscle *, float) {}
  void ComputePhysics() {}
  void RegisterMacroMuscle(IMacroMuscle *) {}
  void UnregisterMacroMuscle(IMacroMuscle *) {}
  void ClearAllRegistration() {}
  void CollectUserItems(bool) {}
  bool CollectUserItems() const { return false; }
  UserID UserItem(const char *) { return 0; }
  int UserValuesCount(UserID) { return 0; }
  float UserValue(UserID, int) { return 0.0f; }
  void ClearUserItems() {}
  void ComputeBonesHierarchy() {}
  bool HasNeck() const { return false; }
  void NeckProcessing2(bool) {}
  bool NeckProcessing2() const { return false; }
  IAnimator *Clone() { return new AnimatorStub; }
  void Destroy() { delete this; }
  bool Load(ITransformerInput *) { return false; }
  void OutputAnimator(IAnimator *) {}
  IAnimator *OutputAnimator() const { return 0; }
  void Generate() {}
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

class GDPFileStub : public IGDPFile
{
public:
  int ObjectsCount() const { return 0; }
  const char *ObjectName(int) const { return 0; }
  IGDPObject *Object(int) { return 0; }
  void Destroy() { delete this; }
};

IAnimator *__stdcall IAnimator::Create() { return new AnimatorStub; }
IMMTree *__stdcall IMMTree::Create() { return new MMTreeStub; }
ISequencer *__stdcall ISequencer::Create() { return new SequencerStub; }
ITransformer *__stdcall ITransformer::Create() { return new TransformerStub; }
IGDPFile *__stdcall IGDPFile::Create(const char *) { return new GDPFileStub; }
}
