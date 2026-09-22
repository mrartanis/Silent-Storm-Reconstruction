// x64 bridge for the proprietary 32-bit LifeStudio runtime.  These no-op
// objects keep optional face-animation data from blocking the game core while
// preserving the API's ownership and failure contracts.
#include "LifeStudioHeadAPIGDP.h"
#include "LifeStudioHeadAPIMMTS.h"

namespace LifeStudioHeadAPI
{
class AnimatorStub : public IAnimator
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
public:
  bool Load(const char *) { return false; }
  bool Load(const char *, int) { return false; }
  IMacroMuscle *RootMacroMuscle() const { return 0; }
  IMacroMuscle *FindMacroMuscle(const char *) { return 0; }
  void Destroy() { delete this; }
};

class SequencerStub : public ISequencer
{
public:
  bool Load(const char *) { return false; }
  bool Load(const char *, int) { return false; }
  IMMTree *RegisterMMTree(IMMTree *) { return 0; }
  int SequenceTime() const { return 0; }
  int TracksCount() const { return 0; }
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
