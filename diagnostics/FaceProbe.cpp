// Deterministic LifeStudio vertex probe. Build for x86 (original DLL) and x64
// (native implementation), then compare the CSVs with Test-FaceParity.ps1.
#include <LifeStudioHeadAPIInit.h>
#include <LifeStudioHeadAPI.h>
#include <LifeStudioHeadAPIMMTS.h>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
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

static void DumpMacroTreeNode(FILE *out, IMacroMuscle *node,
                              const std::string &path, int depth,
                              std::set<IMacroMuscle *> *visited)
{
  if (!node || depth > 64 || visited->size() > 10000 || !visited->insert(node).second)
    return;
  void **vtable = *reinterpret_cast<void ***>(node);
  using NameFn = const char *(__thiscall *)(IMacroMuscle *);
  using CountFn = int (__thiscall *)(IMacroMuscle *);
  using ChildFn = IMacroMuscle *(__thiscall *)(IMacroMuscle *, int);
  const char *name = reinterpret_cast<NameFn>(vtable[0])(node);
  const int count = reinterpret_cast<CountFn>(vtable[10])(node);
  if (!name || count < 0 || count > 10000)
    return;
  const std::string current = path.empty() ? name : path + "/" + name;
  std::fprintf(out, "%d,%d,%s\n", depth, count, current.c_str());
  for (int i = 0; i < count; ++i)
  {
    IMacroMuscle *child = reinterpret_cast<ChildFn>(vtable[9])(node, i);
    if (child)
    {
      const auto *words = reinterpret_cast<const std::uint32_t *>(child);
      std::fprintf(out, "operation,%d,%d,type=%u,%s\n", depth, i, words[5], current.c_str());
      const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("LifeStudioHeadAPI.dll"));
      const int field = words[5] == 0 ? 1 : words[5] < 4 ? 2 : words[5] == 4 ? 4 : 0;
      if (field)
      {
        void *candidate = reinterpret_cast<void *>(words[field]);
        MEMORY_BASIC_INFORMATION region{};
        if (!candidate || !VirtualQuery(candidate, &region, sizeof(region)) ||
            region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
          continue;
        void **candidateVtable = *reinterpret_cast<void ***>(candidate);
        const std::uintptr_t vtableRva = reinterpret_cast<std::uintptr_t>(candidateVtable) - base;
        std::fprintf(out, "operation-pointer,%d,%d,%d,vtable-rva=%zx,%s\n", depth, i, field,
                     vtableRva, current.c_str());
        if (vtableRva == 0x37208)
          DumpMacroTreeNode(out, reinterpret_cast<IMacroMuscle *>(candidate),
                            current, depth + 1, visited);
      }
    }
  }
}

static void DumpMacroTree(IMMTree *tree)
{
  const char *path = std::getenv("S2_FACE_TREE_DUMP_PATH");
  if (!path) return;
  FILE *out = std::fopen(path, "wb");
  if (!out) return;
  std::fprintf(out, "depth,children,path\n");
  std::set<IMacroMuscle *> visited;
  DumpMacroTreeNode(out, tree->RootMacroMuscle(), "", 0, &visited);
  std::fclose(out);
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
  static void WriteNameMap(FILE *out)
  {
    std::fprintf(out, "id,name\n");
#if defined(_M_IX86)
    // Original x86 DLL object layout (observed for the macro-muscle objects
    // passed to AddMacroMuscle): two vtable pointers, then an inline name.
    // This is diagnostic-only and must not become part of the native x64 ABI.
    for (std::size_t id = 0; id < known.size(); ++id)
    {
      const auto address = reinterpret_cast<std::uintptr_t>(known[id]);
      MEMORY_BASIC_INFORMATION region{};
      if (!address || !VirtualQuery(known[id], &region, sizeof(region)) ||
          region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
        continue;
      const auto end = reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize;
      if (address + 8 >= end)
        continue;
      const char *name = reinterpret_cast<const char *>(known[id]) + 8;
      std::size_t length = 0;
      while (length < 64 && address + 8 + length < end && name[length] &&
             static_cast<unsigned char>(name[length]) >= 32 &&
             static_cast<unsigned char>(name[length]) <= 126 && name[length] != ',')
        ++length;
      if (length && length < 64 && address + 8 + length < end && name[length] == 0)
        std::fprintf(out, "%zu,%.*s\n", id, static_cast<int>(length), name);
    }
#endif
  }
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
    const std::size_t id = Id(muscle);
    if (traceOut) std::fprintf(traceOut, "muscle,%d,%zu,add,%.9g\n", time, id, value);
    real->AddMacroMuscle(muscle, value);
  }
  void MultMacroMuscle(IMacroMuscle *muscle, float value) override
  {
    const std::size_t id = Id(muscle);
    if (traceOut) std::fprintf(traceOut, "muscle,%d,%zu,mult,%.9g\n", time, id, value);
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

static bool SampleTimes(int duration, std::vector<int> *result)
{
  if (!result || duration <= 0) return false;
  *result = {0, duration / 4, duration / 2, 3 * duration / 4, duration - 1};
  if (const char *spec = std::getenv("S2_FACE_SAMPLE_TIMES"))
  {
    result->clear();
    const char *cursor = spec;
    while (*cursor)
    {
      errno = 0;
      char *end = nullptr;
      const long value = std::strtol(cursor, &end, 10);
      if (errno || end == cursor || value < 0 || value >= duration ||
          (*end && *end != ','))
        return false;
      result->push_back(static_cast<int>(value));
      cursor = *end ? end + 1 : end;
      if (!*cursor && end[0] == ',') return false;
    }
    if (result->empty()) return false;
  }
  std::sort(result->begin(), result->end());
  result->erase(std::unique(result->begin(), result->end()), result->end());
  return true;
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
      if (traceOut || std::getenv("S2_FACE_TRACE_ANIMATOR") ||
          std::getenv("S2_FACE_MUSCLE_MAP_PATH"))
      {
        TraceAnimator trace(anim, time, traceOut ? traceOut :
                            (std::getenv("S2_FACE_TRACE_ANIMATOR") ? stderr : nullptr));
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
    if (std::getenv("S2_FACE_TRACE_TREE"))
    {
      const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("LifeStudioHeadAPI.dll"));
      void **treeVtable = *reinterpret_cast<void ***>(tree);
      for (int i = 0; i < 8; ++i)
        std::fprintf(stderr, "tree-vtable[%d] RVA=0x%zx\n", i,
                     reinterpret_cast<std::uintptr_t>(treeVtable[i]) - base);
      IMacroMuscle *root = tree->RootMacroMuscle();
      std::fprintf(stderr, "tree-root=%p\n", root);
      if (root)
      {
        void **macroVtable = *reinterpret_cast<void ***>(root);
        for (int i = 0; i < 32; ++i)
          std::fprintf(stderr, "macro-vtable[%d] RVA=0x%zx\n", i,
                       reinterpret_cast<std::uintptr_t>(macroVtable[i]) - base);
        const auto *words = reinterpret_cast<const std::uint32_t *>(root);
        for (int i = 0; i < 20; ++i)
          std::fprintf(stderr, "macro-root-word[%d]=%08x\n", i, words[i]);
      }
    }
    DumpMacroTree(tree);
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
  std::vector<int> times;
  if (sequence && !SampleTimes(duration, &times))
  {
    std::fprintf(stderr, "invalid S2_FACE_SAMPLE_TIMES (expected comma-separated times in [0,%d))\n", duration);
    sequence->Destroy();
    tree->Destroy();
    return 2;
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
    for (int time : times)
      ok = WriteFrame(out, animData, tree, sequence, traceOut, "sequence", time) && ok;
  }
  std::fclose(out);
  if (traceOut) std::fclose(traceOut);
  if (sequence)
    if (const char *mapPath = std::getenv("S2_FACE_MUSCLE_MAP_PATH"))
    {
      FILE *map = std::fopen(mapPath, "wb");
      if (map)
      {
        TraceAnimator::WriteNameMap(map);
        std::fclose(map);
      }
    }
  if (sequence) sequence->Destroy();
  if (tree) tree->Destroy();
  if (!ok)
  {
    std::fprintf(stderr, "LifeStudio Load/Process failed\n");
    return 4;
  }
  return 0;
}
