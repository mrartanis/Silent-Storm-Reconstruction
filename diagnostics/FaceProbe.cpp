// Deterministic LifeStudio vertex probe. Build for x86 (original DLL) and x64
// (native implementation), then compare the CSVs with Test-FaceParity.ps1.
#include <LifeStudioHeadAPIInit.h>
#include <LifeStudioHeadAPI.h>
#include <LifeStudioHeadAPIMMTS.h>
#if defined(_M_X64)
#include <NativeMMTreeRuntime.h>
#endif

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <string>
#include <vector>
#include <Windows.h>
#include <cstdint>

using namespace LifeStudioHeadAPI;

#if defined(_M_X64)
static void SnapshotNativeMuscles(IAnimator *anim, const char *label, int time)
{
  const char *prefix = std::getenv("S2_FACE_NATIVE_MUSCLE_SNAPSHOT_PREFIX");
  const char *selected = std::getenv("S2_FACE_STATE_SNAPSHOT_TIME");
  if (!prefix || !selected || !label || std::strcmp(label, "sequence") != 0 ||
      std::atoi(selected) != time)
    return;
  char path[1200];
  if (std::snprintf(path, sizeof(path), "%s-sequence-%d-post-physics.csv", prefix, time) >=
      static_cast<int>(sizeof(path)))
    return;
  FILE *file = std::fopen(path, "wb");
  if (!file) return;
  std::fprintf(file, "muscle,amplitude\n");
  const int count = NativeAnimatorMuscleCount(anim);
  for (int i = 0; i < count; ++i)
    std::fprintf(file, "%d,%.9g\n", i, NativeAnimatorMuscleAmplitude(anim, i));
  std::fclose(file);
}
#endif

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

static void SnapshotMuscles(IAnimator *anim, const char *stage)
{
  const char *prefix = std::getenv("S2_FACE_MUSCLE_SNAPSHOT_PREFIX");
  if (!prefix || !stage) return;
  const int count = anim->MusclesCount();
  if (count <= 0 || count > 1000) return;
  constexpr std::size_t bytesPerMuscle = 256;
  std::vector<IMuscle *> muscles;
  muscles.reserve(count);
  for (int i = 0; i < count; ++i)
  {
    IMuscle *muscle = anim->Muscle(i);
    MEMORY_BASIC_INFORMATION region{};
    if (!muscle || !VirtualQuery(muscle, &region, sizeof(region)) ||
        region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
      return;
    const auto address = reinterpret_cast<std::uintptr_t>(muscle);
    const auto end = reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize;
    if (address > end || end - address < bytesPerMuscle) return;
    muscles.push_back(muscle);
  }
  char path[1200];
  if (std::snprintf(path, sizeof(path), "%s-%s.bin", prefix, stage) >= static_cast<int>(sizeof(path)))
    return;
  FILE *file = std::fopen(path, "wb");
  if (!file) return;
  std::fwrite(&count, sizeof(count), 1, file);
  for (IMuscle *muscle : muscles)
    std::fwrite(muscle, 1, bytesPerMuscle, file);
  std::fclose(file);
}

static bool ReadableMemory(const void *pointer, std::size_t bytes)
{
  MEMORY_BASIC_INFORMATION region{};
  if (!pointer || !VirtualQuery(pointer, &region, sizeof(region)) ||
      region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
    return false;
  const auto address = reinterpret_cast<std::uintptr_t>(pointer);
  const auto end = reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize;
  return address <= end && bytes <= end - address;
}

// x86-only diagnostic of the original animator's vertex objects. Process()
// iterates its 8-byte vertex entries at animator+0x20 and calls each object.
static void SnapshotVertexState(IAnimator *anim)
{
  const char *path = std::getenv("S2_FACE_VERTEX_STATE_PATH");
  if (!path || !ReadableMemory(anim, 0x28)) return;
  const auto *header = reinterpret_cast<const unsigned char *>(anim);
  const auto begin = *reinterpret_cast<const std::uintptr_t *>(header + 0x20);
  const auto end = *reinterpret_cast<const std::uintptr_t *>(header + 0x24);
  if (end < begin || (end - begin) % 8 || (end - begin) / 8 > 1000000)
    return;
  FILE *file = std::fopen(path, "wb");
  if (!file) return;
  std::fprintf(file, "vertex,influences,interaction-coefficient\n");
  for (std::uintptr_t entry = begin; entry < end; entry += 8)
  {
    if (!ReadableMemory(reinterpret_cast<const void *>(entry), 8)) break;
    const auto *pair = reinterpret_cast<const std::uintptr_t *>(entry);
    const auto *vertex = reinterpret_cast<const unsigned char *>(pair[1]);
    if (!ReadableMemory(vertex, 0x28)) break;
    const auto index = *reinterpret_cast<const std::uint32_t *>(vertex + 8);
    const auto count = *reinterpret_cast<const std::int32_t *>(vertex + 0x1C);
    const double coefficient = *reinterpret_cast<const double *>(vertex + 0x20);
    if (index >= 1000000 || count < 0 || count > 1000)
    {
      std::fprintf(stderr, "vertex-state-invalid index=%u count=%d entry=%zu\n",
                   index, count, static_cast<std::size_t>((entry - begin) / 8));
      break;
    }
    std::fprintf(file, "%u,%d,%.17g\n", index, count, coefficient);
  }
  std::fclose(file);
}

static void SnapshotVertexInfluences(IAnimator *anim)
{
  const char *path = std::getenv("S2_FACE_VERTEX_INFLUENCES_PATH");
  if (!path || !ReadableMemory(anim, 0x28)) return;
  const auto *header = reinterpret_cast<const unsigned char *>(anim);
  const auto begin = *reinterpret_cast<const std::uintptr_t *>(header + 0x20);
  const auto end = *reinterpret_cast<const std::uintptr_t *>(header + 0x24);
  if (end < begin || (end - begin) % 8 || (end - begin) / 8 > 1000000) return;
  FILE *file = std::fopen(path, "wb");
  if (!file) return;
  std::fprintf(file, "vertex,muscle,componentA,componentB\n");
  for (std::uintptr_t address = begin; address < end; address += 8)
  {
    if (!ReadableMemory(reinterpret_cast<const void *>(address), 8)) break;
    const auto *entry = reinterpret_cast<const std::uintptr_t *>(address);
    const auto *vertex = reinterpret_cast<const unsigned char *>(entry[1]);
    if (!ReadableMemory(vertex, 0x20)) break;
    const std::uint32_t index = *reinterpret_cast<const std::uint32_t *>(vertex + 8);
    const std::int32_t count = *reinterpret_cast<const std::int32_t *>(vertex + 0x1C);
    const auto influences = *reinterpret_cast<const std::uintptr_t *>(vertex + 0x0C);
    if (index >= 1000000 || count < 0 || count > 1000 ||
        !ReadableMemory(reinterpret_cast<const void *>(influences), std::size_t(count) * 12))
      break;
    const auto *data = reinterpret_cast<const unsigned char *>(influences);
    for (int i = 0; i < count; ++i)
    {
      const auto muscle = *reinterpret_cast<const std::uint32_t *>(data + i * 12);
      const auto componentA = *reinterpret_cast<const float *>(data + i * 12 + 4);
      const auto componentB = *reinterpret_cast<const float *>(data + i * 12 + 8);
      std::fprintf(file, "%u,%u,%.9g,%.9g\n", index, muscle, componentA, componentB);
    }
  }
  std::fclose(file);
}

// x86-only view of the private 52-byte neck-zone work records. The original
// Process() fills their barycentric weights and updates each 32-byte control
// node; this snapshot is diagnostic evidence, not a serialized fixture.
static void SnapshotNeckState(IAnimator *anim, const char *stage)
{
  const char *prefix = std::getenv("S2_FACE_NECK_SNAPSHOT_PREFIX");
  if (!prefix || !stage || !ReadableMemory(anim, 0x11F)) return;
  const auto *header = reinterpret_cast<const unsigned char *>(anim);
  std::uintptr_t begin = 0, end = 0;
  std::memcpy(&begin, header + 0x117, sizeof(begin));
  std::memcpy(&end, header + 0x11B, sizeof(end));
  if (!begin || end < begin || (end - begin) % 52 || (end - begin) / 52 > 1000 ||
      !ReadableMemory(reinterpret_cast<const void *>(begin), end - begin)) return;
  char path[1200];
  if (std::snprintf(path, sizeof(path), "%s-%s.csv", prefix, stage) >= static_cast<int>(sizeof(path))) return;
  FILE *file = std::fopen(path, "wb");
  if (!file) return;
  std::fprintf(file, "entry,vertex,variant,node,node-pointer,node-index,node-x,node-y,node-z,node-dx,node-dy,node-dz,entry-t,entry-weight\n");
  for (std::uintptr_t entry = begin; entry < end; entry += 52)
  {
    const auto *record = reinterpret_cast<const unsigned char *>(entry);
    std::uint32_t vertex = 0;
    std::memcpy(&vertex, record, sizeof(vertex));
    for (int variant = 0; variant < 2; ++variant)
    for (int nodeIndex = 0; nodeIndex < 3; ++nodeIndex)
    {
      std::uintptr_t pointer = 0;
      std::memcpy(&pointer, record + 0x04 + variant * 12 + nodeIndex * 4, sizeof(pointer));
      if (!ReadableMemory(reinterpret_cast<const void *>(pointer), 32)) continue;
      const auto *node = reinterpret_cast<const unsigned char *>(pointer);
      std::uint32_t sourceIndex = 0;
      std::memcpy(&sourceIndex, node, sizeof(sourceIndex));
      auto f32 = [](const unsigned char *p) { float value; std::memcpy(&value, p, sizeof(value)); return value; };
      std::fprintf(file, "%zu,%u,%d,%d,%p,%u,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
                   static_cast<std::size_t>((entry - begin) / 52), vertex, variant, nodeIndex,
                   reinterpret_cast<const void *>(pointer), sourceIndex,
                   f32(node + 8), f32(node + 12), f32(node + 16),
                   f32(node + 20), f32(node + 24), f32(node + 28),
                   f32(record + 0x1C + nodeIndex * 4),
                   f32(record + 0x28 + nodeIndex * 4));
    }
  }
  std::fclose(file);
}

static void SnapshotNeckCenters(IAnimator *anim, const char *stage)
{
  const char *prefix = std::getenv("S2_FACE_NECK_SNAPSHOT_PREFIX");
  if (!prefix || !stage) return;
  char path[1200];
  if (std::snprintf(path, sizeof(path), "%s-%s-centers.csv", prefix, stage) >= static_cast<int>(sizeof(path))) return;
  FILE *file = std::fopen(path, "wb");
  if (!file) return;
  std::fprintf(file, "bone-type,bone-index,x,y,z\n");
  for (unsigned long type = 3; type <= 4; ++type)
  {
    IBone *bone = anim->BoneByType(type);
    if (!ReadableMemory(bone, sizeof(void *))) continue;
    void **vtable = *reinterpret_cast<void ***>(bone);
    if (!ReadableMemory(vtable, 6 * sizeof(void *))) continue;
    using StateFn = const unsigned char *(__thiscall *)(IBone *);
    const auto *state = reinterpret_cast<StateFn>(vtable[5])(bone);
    if (!ReadableMemory(state, 0x3C)) continue;
    float xyz[3];
    std::memcpy(xyz, state + 0x30, sizeof(xyz));
    int index = -1;
    for (int i = 0; i < anim->BonesCount(); ++i)
      if (anim->Bone(i) == bone) { index = i; break; }
    std::fprintf(file, "%lu,%d,%.9g,%.9g,%.9g\n", type, index, xyz[0], xyz[1], xyz[2]);
  }
  std::fclose(file);
}

static void SnapshotAnimatedState(IAnimator *anim, const char *label, int time, const char *stage)
{
  const char *selected = std::getenv("S2_FACE_STATE_SNAPSHOT_TIME");
  if (!selected || !label || std::strcmp(label, "sequence") != 0 ||
      std::atoi(selected) != time)
    return;
  char suffix[128];
  if (std::snprintf(suffix, sizeof(suffix), "sequence-%d-%s", time, stage) >= static_cast<int>(sizeof(suffix)))
    return;
  SnapshotBones(anim, suffix);
  SnapshotMuscles(anim, suffix);
  SnapshotNeckState(anim, suffix);
  SnapshotNeckCenters(anim, suffix);
}

static void DumpMacroTreeNode(FILE *out, IMMTree *tree, IMacroMuscle *node,
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
  IMacroMuscle *found = tree->FindMacroMuscle(name);
  std::fprintf(out, "lookup,%d,%s,%d\n", depth, name, found == node ? 1 : found ? 2 : 0);
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
        {
          const char *targetName = reinterpret_cast<NameFn>(candidateVtable[0])(
              reinterpret_cast<IMacroMuscle *>(candidate));
          std::fprintf(out, "operation-target,%d,%d,%s\n", depth, i,
                       targetName ? targetName : "");
          DumpMacroTreeNode(out, tree, reinterpret_cast<IMacroMuscle *>(candidate),
                            current, depth + 1, visited);
        }
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
  DumpMacroTreeNode(out, tree, tree->RootMacroMuscle(), "", 0, &visited);
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
#elif defined(_M_X64)
    for (std::size_t id = 0; id < known.size(); ++id)
      if (const char *name = NativeMacroMuscleName(known[id]))
        std::fprintf(out, "%zu,%s\n", id, name);
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

static bool LoadOverlay(const char *pathVariable, const char *shiftVariable,
                        IMMTree *tree, ISequencer **out, int *duration, int *shift)
{
  *out = nullptr;
  *duration = 0;
  *shift = 0;
  const char *path = std::getenv(pathVariable);
  if (!path || !*path) return true;
  const std::vector<char> bytes = ReadAll(path);
  ISequencer *loaded = ISequencer::Create();
  if (bytes.empty() || !loaded || !loaded->Load(bytes.data(), static_cast<int>(bytes.size())))
  {
    std::fprintf(stderr, "cannot load overlay sequence: %s\n", path);
    if (loaded) loaded->Destroy();
    return false;
  }
  loaded->RegisterMMTree(tree);
  const int length = loaded->SequenceTime();
  if (length < 2)
  {
    loaded->Destroy();
    return false;
  }
  int parsedShift = 0;
  if (const char *value = std::getenv(shiftVariable))
  {
    errno = 0;
    char *end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (errno || end == value || *end ||
        parsed < (std::numeric_limits<int>::min)() ||
        parsed > (std::numeric_limits<int>::max)())
    {
      std::fprintf(stderr, "invalid overlay time shift: %s\n", value);
      loaded->Destroy();
      return false;
    }
    parsedShift = static_cast<int>(parsed);
  }
  *out = loaded;
  *duration = length;
  *shift = parsedShift;
  return true;
}

static bool WriteFrame(FILE *out, const std::vector<char> &animData,
                       IMMTree *tree, ISequencer *sequence,
                       ISequencer *overlay, int overlayShift, int overlayDuration,
                       ISequencer *overlay2, int overlay2Shift, int overlay2Duration,
                       bool overlay2HoldLast,
                       FILE *traceOut,
                       const char *label, int time)
{
  IAnimator *anim = IAnimator::Create();
  if (!anim)
    return false;
  bool ok = anim->Load(animData.data(), static_cast<int>(animData.size()));
  if (ok && std::getenv("S2_FACE_DISABLE_NECK_PROCESSING"))
    anim->NeckProcessing2(false);
  if (ok && std::getenv("S2_FACE_ENABLE_NECK_PROCESSING"))
    anim->NeckProcessing2(true);
  if (ok && !sequence && std::getenv("S2_FACE_TRACE_METADATA"))
    std::fprintf(stderr, "animator-metadata,muscles=%d,bones=%d,vertices=%d,has-neck=%d,neck-processing2=%d\n",
                 anim->MusclesCount(), anim->BonesCount(), anim->VerticesCount(),
                 anim->HasNeck() ? 1 : 0, anim->NeckProcessing2() ? 1 : 0);
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
#if defined(_M_IX86)
    SnapshotAnimatedState(anim, label, time, "pre");
#endif
    const char *forcedMacro = std::getenv("S2_FACE_FORCE_MACRO_NAME");
    if (sequence && forcedMacro && *forcedMacro)
    {
      IMacroMuscle *macro = tree ? tree->FindMacroMuscle(forcedMacro) : nullptr;
      const char *valueText = std::getenv("S2_FACE_FORCE_MACRO_VALUE");
      char *end = nullptr;
      const float value = valueText ? std::strtof(valueText, &end) : 1.0f;
      if (!macro || (valueText && (!end || end == valueText || *end)))
        ok = false;
      else
        anim->AddMacroMuscle(macro, value);
    }
    else if (sequence)
    {
      const std::int64_t shifted = std::int64_t(time) + overlayShift;
      const bool overlayActive = overlay && shifted >= 0 && shifted < overlayDuration;
      const std::int64_t shifted2 = std::int64_t(time) + overlay2Shift;
      const bool overlay2Active = overlay2 && shifted2 >= 0 &&
          (shifted2 < overlay2Duration || overlay2HoldLast);
      const int overlay2Time = shifted2 >= overlay2Duration
          ? overlay2Duration - 2 : static_cast<int>(shifted2);
      if (traceOut || std::getenv("S2_FACE_TRACE_ANIMATOR") ||
          std::getenv("S2_FACE_MUSCLE_MAP_PATH"))
      {
        TraceAnimator trace(anim, time, traceOut ? traceOut :
                            (std::getenv("S2_FACE_TRACE_ANIMATOR") ? stderr : nullptr));
        sequence->RenderMacroMuscles(&trace, time);
        if (overlayActive) overlay->RenderMacroMuscles(&trace, static_cast<int>(shifted));
        if (overlay2Active) overlay2->RenderMacroMuscles(&trace, overlay2Time);
      }
      else
      {
        sequence->RenderMacroMuscles(anim, time);
        if (overlayActive) overlay->RenderMacroMuscles(anim, static_cast<int>(shifted));
        if (overlay2Active) overlay2->RenderMacroMuscles(anim, overlay2Time);
      }
    }
#if defined(_M_IX86)
    SnapshotAnimatedState(anim, label, time, "post-render");
#endif
    if (!std::getenv("S2_FACE_SKIP_PHYSICS"))
      anim->ComputePhysics();
#if defined(_M_IX86)
    SnapshotAnimatedState(anim, label, time, "post-physics");
#elif defined(_M_X64)
    SnapshotNativeMuscles(anim, label, time);
#endif
    if (!std::getenv("S2_FACE_SKIP_FILL_UNUSED"))
      anim->FillUnused(true);
#if defined(_M_IX86)
    if (sequence && std::getenv("S2_FACE_VERTEX_STATE_PATH") &&
        std::getenv("S2_FACE_STATE_SNAPSHOT_TIME") &&
        std::atoi(std::getenv("S2_FACE_STATE_SNAPSHOT_TIME")) == time)
      SnapshotVertexState(anim);
    if (sequence && std::getenv("S2_FACE_VERTEX_INFLUENCES_PATH") &&
        std::getenv("S2_FACE_STATE_SNAPSHOT_TIME") &&
        std::atoi(std::getenv("S2_FACE_STATE_SNAPSHOT_TIME")) == time)
      SnapshotVertexInfluences(anim);
#endif
    ok = anim->Process(vertices.data(), 3);
#if defined(_M_IX86)
    SnapshotAnimatedState(anim, label, time, "post-process");
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
    void **effectVtable = *reinterpret_cast<void ***>(
        reinterpret_cast<unsigned char *>(inspect) + 8);
    for (int i = 0; i < 4; ++i)
      std::fprintf(stderr, "effect-vtable[%d] RVA=0x%zx\n", i,
                   reinterpret_cast<std::uintptr_t>(effectVtable[i]) - base);
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
  ISequencer *overlay = nullptr;
  ISequencer *overlay2 = nullptr;
  int overlayShift = 0;
  int overlayDuration = 0;
  int overlay2Shift = 0;
  int overlay2Duration = 0;
  const bool overlay2HoldLast = std::getenv("S2_FACE_OVERLAY2_HOLD_LAST") != nullptr;
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
    if (!LoadOverlay("S2_FACE_OVERLAY_SEQUENCE_FILE", "S2_FACE_OVERLAY_TIME_SHIFT",
                     tree, &overlay, &overlayDuration, &overlayShift) ||
        !LoadOverlay("S2_FACE_OVERLAY2_SEQUENCE_FILE", "S2_FACE_OVERLAY2_TIME_SHIFT",
                     tree, &overlay2, &overlay2Duration, &overlay2Shift))
    {
      if (overlay2) overlay2->Destroy();
      if (overlay) overlay->Destroy();
      sequence->Destroy();
      tree->Destroy();
      return 3;
    }
  }
  std::vector<int> times;
  if (sequence && !SampleTimes(duration, &times))
  {
    std::fprintf(stderr, "invalid S2_FACE_SAMPLE_TIMES (expected comma-separated times in [0,%d))\n", duration);
    if (overlay) overlay->Destroy();
    if (overlay2) overlay2->Destroy();
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
  bool ok = WriteFrame(out, animData, tree, nullptr, nullptr, 0, 0,
                       nullptr, 0, 0, false,
                       traceOut, "neutral", 0);
  if (ok && sequence)
  {
    for (int time : times)
      ok = WriteFrame(out, animData, tree, sequence, overlay, overlayShift,
                      overlayDuration, overlay2, overlay2Shift,
                      overlay2Duration, overlay2HoldLast,
                      traceOut, "sequence", time) && ok;
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
  if (overlay) overlay->Destroy();
  if (overlay2) overlay2->Destroy();
  if (sequence) sequence->Destroy();
  if (tree) tree->Destroy();
  if (!ok)
  {
    std::fprintf(stderr, "LifeStudio Load/Process failed\n");
    return 4;
  }
  return 0;
}
