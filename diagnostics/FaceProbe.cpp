// Deterministic LifeStudio vertex probe. Build for x86 (original DLL) and x64
// (native implementation), then compare the CSVs with Test-FaceParity.ps1.
#include <LifeStudioHeadAPIInit.h>
#include <LifeStudioHeadAPI.h>
#include <LifeStudioHeadAPIMMTS.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <Windows.h>
#include <cstdint>

using namespace LifeStudioHeadAPI;

#if defined(_M_IX86)
// Diagnostic snapshot of the original DLL's opaque bone objects. These bytes
// are local reverse-engineering evidence, never fixture data committed to Git.
static void SnapshotBones(IAnimator *anim, const char *stage)
{
  const char *prefix = std::getenv("S2_FACE_BONE_SNAPSHOT_PREFIX");
  if (!prefix || !stage)
    return;
  const int count = anim->BonesCount();
  if (count <= 0 || count > 1000)
    return;
  char path[1200];
  if (std::snprintf(path, sizeof(path), "%s-%s.bin", prefix, stage) >= static_cast<int>(sizeof(path)))
    return;
  FILE *file = std::fopen(path, "wb");
  if (!file)
    return;
  std::fwrite(&count, sizeof(count), 1, file);
  for (int i = 0; i < count; ++i)
  {
    IBone *bone = anim->Bone(i);
    if (!bone || std::fwrite(bone, 1, 556, file) != 556)
      break;
  }
  std::fclose(file);
}
#endif

// The x86 sequencer's RenderMacroMuscles calls IAnimator::Add/MultMacroMuscle.
// Interpose only on this diagnostic path to observe the sequence's real output
// without depending on the DLL's non-public enumerator callback ABI.
class TraceAnimator : public IAnimator
{
  IAnimator *real;
  int time;
  FILE *traceOut;
  static std::vector<IMacroMuscle *> known;
  static std::size_t Id(IMacroMuscle *muscle)
  {
    auto it = std::find(known.begin(), known.end(), muscle);
    if (it != known.end()) return std::size_t(it - known.begin());
    known.push_back(muscle);
    return known.size() - 1;
  }
public:
  TraceAnimator(IAnimator *animator, int frameTime, FILE *output): real(animator), time(frameTime), traceOut(output) {}
  bool Load(const char *data, int size) override { return real->Load(data, size); }
  int SaveBufferSize() override { return real->SaveBufferSize(); }
  bool Save(char *data) override { return real->Save(data); }
  IMuscle *MuscleByName(const char *name) override { return real->MuscleByName(name); }
  IMuscle *Muscle(int i) override { return real->Muscle(i); }
  int MusclesCount() const override { return real->MusclesCount(); }
  IBone *BoneByName(const char *name) override { return real->BoneByName(name); }
  IBone *Bone(int i) override { return real->Bone(i); }
  IBone *BoneByType(unsigned long type, IBone *prev) override { return real->BoneByType(type, prev); }
  int BonesCount() const override { return real->BonesCount(); }
  void FillUnused(bool fill) override { real->FillUnused(fill); }
  bool FillUnused() const override { return real->FillUnused(); }
  bool Process(float *vertices, int step) override { return real->Process(vertices, step); }
  int VerticesCount() const override { return real->VerticesCount(); }
  void ClearAllMacroMuscles() override { real->ClearAllMacroMuscles(); }
  void AddMacroMuscle(IMacroMuscle *muscle, float value) override
  {
    std::fprintf(traceOut, "muscle,%d,%zu,add,%.9g\n", time, Id(muscle), value);
    real->AddMacroMuscle(muscle, value);
  }
  void MultMacroMuscle(IMacroMuscle *muscle, float value) override
  {
    std::fprintf(traceOut, "muscle,%d,%zu,mult,%.9g\n", time, Id(muscle), value);
    real->MultMacroMuscle(muscle, value);
  }
  void ComputePhysics() override { real->ComputePhysics(); }
  void RegisterMacroMuscle(IMacroMuscle *muscle) override { real->RegisterMacroMuscle(muscle); }
  void UnregisterMacroMuscle(IMacroMuscle *muscle) override { real->UnregisterMacroMuscle(muscle); }
  void ClearAllRegistration() override { real->ClearAllRegistration(); }
  void CollectUserItems(bool use) override { real->CollectUserItems(use); }
  bool CollectUserItems() const override { return real->CollectUserItems(); }
  UserID UserItem(const char *name) override { return real->UserItem(name); }
  int UserValuesCount(UserID id) override { return real->UserValuesCount(id); }
  float UserValue(UserID id, int i) override { return real->UserValue(id, i); }
  void ClearUserItems() override { real->ClearUserItems(); }
  void ComputeBonesHierarchy() override { real->ComputeBonesHierarchy(); }
  bool HasNeck() const override { return real->HasNeck(); }
  void NeckProcessing2(bool use) override { real->NeckProcessing2(use); }
  bool NeckProcessing2() const override { return real->NeckProcessing2(); }
  IAnimator *Clone() override { return real->Clone(); }
  // This diagnostic adapter is stack-owned by WriteFrame.
  void Destroy() override {}
};
std::vector<IMacroMuscle *> TraceAnimator::known;

static std::vector<char> ReadAll(const char *path)
{
  std::ifstream file(path, std::ios::binary);
  if (!file)
    return {};
  return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

static bool WriteFrame(FILE *out, const std::vector<char> &animData,
                       IMMTree *tree, ISequencer *sequence, FILE *traceOut,
                       const char *label, int time)
{
  IAnimator *anim = IAnimator::Create();
  if (!anim)
    return false;
  bool ok = anim->Load(animData.data(), static_cast<int>(animData.size()));
  if (ok && !sequence && std::getenv("S2_FACE_TRACE_METADATA"))
    std::fprintf(stderr, "animator-metadata,muscles=%d,bones=%d,vertices=%d\n",
                 anim->MusclesCount(), anim->BonesCount(), anim->VerticesCount());
#if defined(_M_IX86)
  if (ok && !sequence)
    SnapshotBones(anim, "neutral-load");
#endif
  if (ok && tree)
    anim->RegisterMacroMuscle(tree->RootMacroMuscle());
  const int count = ok ? anim->VerticesCount() : 0;
  if (count <= 0 || count > 1000000)
    ok = false;
  std::vector<float> vertices(ok ? count * 3 : 0);
  if (ok)
  {
    anim->ClearAllMacroMuscles();
    if (sequence)
    {
      if (traceOut || std::getenv("S2_FACE_TRACE_ANIMATOR"))
      {
        TraceAnimator trace(anim, time, traceOut ? traceOut : stderr);
        sequence->RenderMacroMuscles(&trace, time);
      }
      else
        sequence->RenderMacroMuscles(anim, time);
    }
    if (!std::getenv("S2_FACE_SKIP_PHYSICS"))
      anim->ComputePhysics();
    if (!std::getenv("S2_FACE_SKIP_FILL_UNUSED"))
      anim->FillUnused(true);
    ok = anim->Process(vertices.data(), 3);
#if defined(_M_IX86)
    if (ok && !sequence)
      SnapshotBones(anim, "neutral-process");
#endif
  }
  if (ok)
    for (int i = 0; i < count; ++i)
      std::fprintf(out, "%s,%d,%d,%.9g,%.9g,%.9g\n", label, time, i,
                   vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
  anim->Destroy();
  return ok;
}

int main(int argc, char **argv)
{
  if (argc != 3 && argc != 4 && argc != 5 && argc != 6)
  {
    std::fprintf(stderr, "usage: FaceProbe animator.bin output.csv [sequence.bin tree.mma] [saved-animator.bin]\n");
    return 2;
  }
  const std::vector<char> animData = ReadAll(argv[1]);
  if (animData.empty())
  {
    std::fprintf(stderr, "cannot read animator: %s\n", argv[1]);
    return 2;
  }
  Init();
#if defined(_M_IX86)
  if (std::getenv("S2_FACE_TRACE_VTABLE"))
  {
    IAnimator *inspect = IAnimator::Create();
    if (!inspect)
      return 3;
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("LifeStudioHeadAPI.dll"));
    void **vtable = *reinterpret_cast<void ***>(inspect);
    for (int i = 0; i < 50; ++i)
      std::fprintf(stderr, "animator-vtable[%d] RVA=0x%zx\n", i,
                   reinterpret_cast<std::uintptr_t>(vtable[i]) - base);
    inspect->Destroy();
  }
#endif
  const char *savedPath = argc == 4 ? argv[3] : argc == 6 ? argv[5] : nullptr;
  if (savedPath)
  {
    IAnimator *copy = IAnimator::Create();
    if (!copy || !copy->Load(animData.data(), static_cast<int>(animData.size())))
      return 3;
    const int size = copy->SaveBufferSize();
    std::vector<char> saved(size > 0 && size < 100000000 ? size : 0);
    const bool savedOk = !saved.empty() && copy->Save(saved.data());
    copy->Destroy();
    if (!savedOk)
      return 3;
    std::ofstream file(savedPath, std::ios::binary);
    file.write(saved.data(), saved.size());
    if (!file)
      return 2;
  }
  IMMTree *tree = nullptr;
  ISequencer *sequence = nullptr;
  int duration = 0;
  if (argc >= 5)
  {
    const std::vector<char> seqData = ReadAll(argv[3]);
    if (seqData.empty())
      return 2;
    tree = IMMTree::Create();
    sequence = ISequencer::Create();
    if (!tree || !sequence || !tree->Load(argv[4]) ||
        !sequence->Load(seqData.data(), static_cast<int>(seqData.size())))
    {
      std::fprintf(stderr, "cannot load sequence or muscle tree\n");
      if (sequence) sequence->Destroy();
      if (tree) tree->Destroy();
      return 3;
    }
#if defined(_M_IX86)
    if (std::getenv("S2_FACE_TRACE_VTABLE"))
    {
      const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("LifeStudioHeadAPI.dll"));
      void **vtable = *reinterpret_cast<void ***>(sequence);
      for (int i = 0; i < 18; ++i)
        std::fprintf(stderr, "sequence-vtable[%d] RVA=0x%zx\n", i,
                     reinterpret_cast<std::uintptr_t>(vtable[i]) - base);
    }
#endif
    sequence->RegisterMMTree(tree);
    duration = sequence->SequenceTime();
    if (std::getenv("S2_FACE_TRACE_METADATA"))
      std::fprintf(stderr, "sequence-metadata,duration=%d,tracks=%d\n",
                   duration, sequence->TracksCount());
    if (duration <= 0)
    {
      std::fprintf(stderr, "invalid sequence duration\n");
      sequence->Destroy();
      tree->Destroy();
      return 3;
    }
  }
  FILE *out = std::fopen(argv[2], "wb");
  if (!out)
    return 2;
  FILE *traceOut = nullptr;
  if (const char *tracePath = std::getenv("S2_FACE_MUSCLE_TRACE_PATH"))
  {
    traceOut = std::fopen(tracePath, "wb");
    if (!traceOut)
    {
      std::fprintf(stderr, "cannot write muscle trace: %s\n", tracePath);
      std::fclose(out);
      return 2;
    }
    std::fprintf(traceOut, "kind,time,muscle,operation,value\n");
  }
  std::fprintf(out, "case,time,vertex,x,y,z\n");
  bool ok = WriteFrame(out, animData, tree, nullptr, traceOut, "neutral", 0);
  if (ok && sequence)
  {
    std::vector<int> times = {0, duration / 4, duration / 2, 3 * duration / 4, duration - 1};
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());
    for (int time : times)
      ok = WriteFrame(out, animData, tree, sequence, traceOut, "sequence", time) && ok;
  }
  std::fclose(out);
  if (traceOut) std::fclose(traceOut);
  if (sequence) sequence->Destroy();
  if (tree) tree->Destroy();
  if (!ok)
  {
    std::fprintf(stderr, "LifeStudio Load/Process failed\n");
    return 4;
  }
  return 0;
}
