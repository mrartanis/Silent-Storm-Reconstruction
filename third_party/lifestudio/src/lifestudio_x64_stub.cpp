// Partial native x64 LifeStudio bridge. Original head and sequence streams
// load without the x86 DLL; bone/muscle deformation is not implemented yet.
#include "LifeStudioHeadAPIGDP.h"
#include "LifeStudioHeadAPIMMTS.h"
#include "NativeHeadData.h"
#include "NativeSequenceData.h"
#include <fstream>
#include <iterator>
#include <limits>
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

std::uint32_t U32(const unsigned char *p)
{
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
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
class AnimatorStub : public IAnimator
{
  NativeLifeStudio::HeadData head;
  bool loaded = false;
  bool fillUnused = false;
public:
  bool Load(const char *bytes, int size)
  {
    NativeLifeStudio::HeadData parsed;
    if (size < 0 || !NativeLifeStudio::DecodeHeadVertices(bytes, static_cast<std::size_t>(size), &parsed))
    {
      head = {};
      loaded = false;
      return false;
    }
    head = std::move(parsed);
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
      const float *position = vertex.sourcePosition;
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
  IAnimator *Clone() { return new AnimatorStub(*this); }
  void Destroy() { delete this; }
};

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
  bool loaded = false;
public:
  bool Load(const char *path)
  {
    const auto bytes = ReadFile(path);
    return !bytes.empty() && Load(bytes.data(), static_cast<int>(bytes.size()));
  }
  bool Load(const char *bytes, int size)
  {
    loaded = size >= 32 && bytes && U32(reinterpret_cast<const unsigned char *>(bytes)) == 0x464C4D4Du &&
             U32(reinterpret_cast<const unsigned char *>(bytes) + 4) == 4 &&
             U32(reinterpret_cast<const unsigned char *>(bytes) + 28) == static_cast<std::uint32_t>(size - 32);
    return loaded;
  }
  IMacroMuscle *RootMacroMuscle() const { return 0; }
  IMacroMuscle *FindMacroMuscle(const char *) { return 0; }
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
  void RenderMacroMuscles(IAnimator *, int) {}
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
