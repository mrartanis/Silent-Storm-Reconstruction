// Extract the original x86 FaceGen GDP transformer's generated animator and
// vertices. The outputs stay in the ignored lab directory for native parity.
#include <LifeStudioHeadAPIInit.h>
#include <LifeStudioHeadAPIGDP.h>
#include <LifeStudioHeadAPIMMTS.h>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#if defined(_M_IX86)
#include <Windows.h>
#endif

using namespace LifeStudioHeadAPI;

static bool WriteBytes(const std::string &path, const std::vector<char> &bytes)
{
  std::ofstream file(path, std::ios::binary);
  file.write(bytes.data(), bytes.size());
  return static_cast<bool>(file);
}

int main(int argc, char **argv)
{
  if (argc < 4 || (argc - 4) % 2)
  {
    std::fprintf(stderr, "usage: FaceGDPProbe head.gdp head.mmt output-prefix [macro-name amplitude]...\n");
    return 2;
  }
  Init();
  IGDPFile *gdp = IGDPFile::Create(argv[1]);
  if (!gdp)
  {
    std::fprintf(stderr, "cannot open GDP: %s\n", argv[1]);
    return 3;
  }
  const int objectCount = gdp->ObjectsCount();
  std::fprintf(stderr, "GDP objects=%d first=%s\n", objectCount,
               objectCount > 0 ? gdp->ObjectName(0) : "(none)");
  IGDPObject *object = objectCount > 0 ? gdp->Object(0) : nullptr;
  if (!object)
  {
    gdp->Destroy();
    return 3;
  }
  const int defaultSize = object->DefaultAnimatorDataSize();
  std::fprintf(stderr, "GDP vertices=%d transformable=%d default-animator=%d\n",
               object->VerticesCount(), object->IsTransformable(), defaultSize);
  const int dataItems = object->DataListSize();
  std::fprintf(stderr, "GDP data-items=%d subobjects=%d\n", dataItems,
               object->SubObjectsCount());
  if (dataItems >= 0 && dataItems <= 10000)
    for (int i = 0; i < dataItems; ++i)
    {
      const char *name = object->DataListItem(i);
      if (name)
      {
        if (std::getenv("S2_FACE_GDP_TRACE_ITEMS"))
          std::fprintf(stderr, "GDP data[%d]=%s bytes=%d\n", i, name, object->Size(name));
        const char *dump = std::getenv("S2_FACE_GDP_DUMP_DIR");
        const int itemSize = object->Size(name);
        bool safeName = *name && std::string(name) != "." && std::string(name) != "..";
        for (const unsigned char *p = reinterpret_cast<const unsigned char *>(name); *p; ++p)
          if (!std::isalnum(*p) && *p != '_' && *p != '-' && *p != '.') safeName = false;
        if (dump && safeName && itemSize > 0 && itemSize < 100000000)
        {
          std::vector<char> data(itemSize);
          char numbered[32];
          std::snprintf(numbered, sizeof(numbered), "%02d-", i);
          if (!object->Get(name, data.data()) ||
              !WriteBytes(std::string(dump) + numbered + name, data))
          {
            std::fprintf(stderr, "cannot dump GDP data item %d\n", i);
            object->Destroy();
            gdp->Destroy();
            return 3;
          }
        }
      }
    }
  std::vector<char> defaults(defaultSize > 0 && defaultSize < 100000000 ? defaultSize : 0);
  const bool defaultsOk = !defaults.empty() && object->DefaultAnimatorData(defaults.data()) &&
                          WriteBytes(std::string(argv[3]) + "-default.bin", defaults);
  IMMTree *tree = IMMTree::Create();
  ITransformer *transformer = ITransformer::Create();
  IAnimator *animator = IAnimator::Create();
#if defined(_M_IX86)
  if (transformer && std::getenv("S2_FACE_TRACE_TRANSFORMER"))
  {
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("LifeStudioHeadAPI.dll"));
    void **vtable = *reinterpret_cast<void ***>(transformer);
    for (int i = 0; i < 43; ++i)
      std::fprintf(stderr, "transformer-vtable[%d] RVA=0x%zx\n", i,
                   reinterpret_cast<std::uintptr_t>(vtable[i]) - base);
    void **selectorVtable = *reinterpret_cast<void ***>(
        reinterpret_cast<unsigned char *>(transformer) + 0x0c);
    for (int i = 0; i < 5; ++i)
      std::fprintf(stderr, "transformer-selector-vtable[%d] RVA=0x%zx\n", i,
                   reinterpret_cast<std::uintptr_t>(selectorVtable[i]) - base);
  }
#endif
  bool ok = defaultsOk && object->IsTransformable() && tree && transformer && animator &&
            tree->Load(argv[2]) && transformer->Load(object);
#if defined(_M_IX86)
  if (ok && std::getenv("S2_FACE_TRACE_TRANSFORMER"))
  {
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("LifeStudioHeadAPI.dll"));
    const auto bytes = reinterpret_cast<const unsigned char *>(transformer);
    const auto selector = bytes + 0x0c;
    const auto ruleSpace = *reinterpret_cast<const void *const *>(selector + 0x14);
    if (ruleSpace)
    {
      MEMORY_BASIC_INFORMATION info{};
      if (VirtualQuery(*reinterpret_cast<void *const *>(ruleSpace), &info, sizeof(info)) == sizeof(info))
      {
        char modulePath[MAX_PATH]{};
        GetModuleFileNameA(static_cast<HMODULE>(info.AllocationBase), modulePath, MAX_PATH);
        std::fprintf(stderr, "transformer-rule-space-module=%s\n", modulePath);
        void **ruleVtable = *reinterpret_cast<void ***>(const_cast<void *>(ruleSpace));
        const auto moduleBase = reinterpret_cast<std::uintptr_t>(info.AllocationBase);
        for (int i = 0; i < 5; ++i)
          std::fprintf(stderr, "transformer-rule-space-vtable[%d] RVA=0x%zx\n", i,
                       reinterpret_cast<std::uintptr_t>(ruleVtable[i]) - moduleBase);
      }
    }
    for (int offset : {0x10, 0x14, 0x9c, 0xa0, 0xe0, 0xf4})
    {
      const auto value = *reinterpret_cast<const std::uintptr_t *>(bytes + offset);
      std::fprintf(stderr, "transformer[0x%x]=0x%zx\n", offset, value);
    }
    const auto worker = *reinterpret_cast<void *const *>(bytes + 0xa0);
    MEMORY_BASIC_INFORMATION region{};
    if (worker && VirtualQuery(worker, &region, sizeof(region)) == sizeof(region) &&
        region.State == MEM_COMMIT && !(region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
    {
      void **vtable = reinterpret_cast<void **>(worker);
      for (int i = 0; i < 8; ++i)
        std::fprintf(stderr, "transformer-worker-vtable[%d] RVA=0x%zx\n", i,
                     reinterpret_cast<std::uintptr_t>(vtable[i]) - base);
    }
  }
#endif
  if (ok)
  {
    transformer->OutputAnimator(animator);
    transformer->RegisterMacroMuscle(tree->RootMacroMuscle());
    transformer->ClearAllMacroMuscles();
    for (int argument = 4; ok && argument < argc; argument += 2)
    {
      char *end = nullptr;
      const float amplitude = std::strtof(argv[argument + 1], &end);
      if (end == argv[argument + 1] || *end)
      {
        std::fprintf(stderr, "invalid amplitude: %s\n", argv[argument + 1]);
        ok = false;
        break;
      }
      IMacroMuscle *macro = tree->FindMacroMuscle(argv[argument]);
      if (!macro)
      {
        std::fprintf(stderr, "unknown macro: %s\n", argv[argument]);
        ok = false;
      }
      else
        transformer->AddMacroMuscle(macro, amplitude);
    }
    if (ok)
    {
      transformer->ComputePhysics();
#if defined(_M_IX86)
      if (const char *path = std::getenv("S2_FACE_SELECTOR_PARAMS_PATH"))
      {
        const auto bytes = reinterpret_cast<const unsigned char *>(transformer);
        const int count = *reinterpret_cast<const int *>(bytes + 0xe0);
        const auto values = *reinterpret_cast<const float *const *>(bytes + 0x100);
        FILE *csv = count == 5 && values ? std::fopen(path, "wb") : nullptr;
        if (!csv) ok = false;
        else
        {
          std::fprintf(csv, "index,value\n");
          for (int index = 0; index < count; ++index)
            std::fprintf(csv, "%d,%.9g\n", index, values[index]);
          ok = std::fclose(csv) == 0;
        }
      }
      if (const char *path = std::getenv("S2_FACE_SELECTOR_WEIGHTS_PATH"))
      {
        const auto selector = reinterpret_cast<const unsigned char *>(transformer) + 0x0c;
        const auto begin = *reinterpret_cast<const float *const *>(selector + 0x70);
        const auto end = *reinterpret_cast<const float *const *>(selector + 0x74);
        if (!begin || !end || end < begin || end - begin > 10000)
          ok = false;
        else
        {
          FILE *csv = std::fopen(path, "wb");
          if (!csv) ok = false;
          else
          {
            std::fprintf(csv, "index,weight\n");
            for (const float *weight = begin; weight != end; ++weight)
              std::fprintf(csv, "%zu,%.9g\n",
                           static_cast<std::size_t>(weight - begin), *weight);
            ok = std::fclose(csv) == 0;
          }
        }
      }
      // The GDP selector blends its own prepared per-archetype coordinate
      // arrays, not necessarily the raw *_A.mld vertex positions. Expose
      // those x86-only arrays so the native blender can be checked at the
      // exact input boundary of the original blend loop (RVA 0x10830).
      if (const char *path = std::getenv("S2_FACE_SELECTOR_COORDS_PATH"))
      {
        const auto selector = reinterpret_cast<const unsigned char *>(transformer) + 0x0c;
        const int vertexCount = *reinterpret_cast<const int *>(selector + 0x10);
        const auto archetypeBegin = *reinterpret_cast<const void *const *const *>(selector + 0x30);
        const auto archetypeEnd = *reinterpret_cast<const void *const *const *>(selector + 0x34);
        const auto coordinates = *reinterpret_cast<const float *const *const *>(selector + 0x50);
        const auto weightBegin = *reinterpret_cast<const float *const *>(selector + 0x70);
        const auto weightEnd = *reinterpret_cast<const float *const *>(selector + 0x74);
        const auto archetypeCount = archetypeBegin && archetypeEnd && archetypeEnd >= archetypeBegin
            ? archetypeEnd - archetypeBegin : 0;
        if (vertexCount <= 0 || vertexCount > 1000000 || archetypeCount <= 0 ||
            archetypeCount > 10000 || !coordinates || !weightBegin || !weightEnd ||
            weightEnd - weightBegin != archetypeCount)
          ok = false;
        else
        {
          FILE *csv = std::fopen(path, "wb");
          if (!csv) ok = false;
          else
          {
            std::fprintf(csv, "archetype,vertex,x,y,z\n");
            for (int archetype = 0; archetype < archetypeCount; ++archetype)
            {
              const float *source = coordinates[archetype];
              if (!source) { ok = false; break; }
              for (int vertex = 0; vertex < vertexCount; ++vertex)
                std::fprintf(csv, "%d,%d,%.9g,%.9g,%.9g\n", archetype, vertex,
                             source[vertex * 3], source[vertex * 3 + 1],
                             source[vertex * 3 + 2]);
            }
            if (std::fclose(csv) != 0) ok = false;
          }
        }
      }
      // The original transformer owns a 36-muscle animation base at +0x10
      // and a 129-muscle morph head at +0x14. Export their canonical streams
      // and the morph head's live Process output before Generate transfers it
      // into the final animation head. This diagnostic is x86-DLL-specific.
      if (const char *prefix = std::getenv("S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX"))
      {
        const auto bytes = reinterpret_cast<const unsigned char *>(transformer);
        const auto dumpAnimator = [prefix, bytes](int offset, const char *suffix) {
          auto *value = *reinterpret_cast<IAnimator *const *>(bytes + offset);
          const int size = value ? value->SaveBufferSize() : 0;
          if (size <= 0 || size > 100000000) return false;
          std::vector<char> data(static_cast<std::size_t>(size));
          std::fprintf(stderr, "transformer-input[0x%x] bytes=%d vertices=%d muscles=%d bones=%d\n",
                       offset, size, value->VerticesCount(), value->MusclesCount(),
                       value->BonesCount());
          return value->Save(data.data()) && WriteBytes(std::string(prefix) + suffix, data);
        };
        if (!dumpAnimator(0x10, "-base-animation.bin") ||
            !dumpAnimator(0x14, "-morphed-head.bin"))
        {
          std::fprintf(stderr, "cannot dump transformer input animators\n");
          ok = false;
        }
        auto *morph = *reinterpret_cast<IAnimator *const *>(bytes + 0x14);
        const int count = morph ? morph->VerticesCount() : 0;
        if (ok && count > 0 && count < 1000000)
        {
          std::vector<float> vertices(static_cast<std::size_t>(count) * 3);
          if (morph->Process(vertices.data(), 3))
          {
            FILE *csv = std::fopen((std::string(prefix) + "-morph-processed.csv").c_str(), "wb");
            if (csv)
            {
              std::fprintf(csv, "vertex,x,y,z\n");
              for (int i = 0; i < count; ++i)
                std::fprintf(csv, "%d,%.9g,%.9g,%.9g\n", i,
                             vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
              ok = std::fclose(csv) == 0;
            }
            else ok = false;
          }
          else ok = false;
        }
      }
      if (std::getenv("S2_FACE_TRACE_TRANSFORMER"))
      {
        const auto bytes = reinterpret_cast<const unsigned char *>(transformer);
        for (int offset : {0x10, 0x14, 0x9c, 0xe0, 0xf4})
          std::fprintf(stderr, "transformer-after-physics[0x%x]=0x%zx\n", offset,
                       *reinterpret_cast<const std::uintptr_t *>(bytes + offset));
        const auto selector = bytes + 0x0c;
        for (int offset : {0x04, 0x10, 0x2c, 0x30, 0x34, 0x40, 0x44,
                           0x50, 0x6c, 0x70, 0x74, 0x8c})
          std::fprintf(stderr, "transformer-selector[0x%x]=0x%zx\n", offset,
                       *reinterpret_cast<const std::uintptr_t *>(selector + offset));
        const auto begin = *reinterpret_cast<const float *const *>(selector + 0x70);
        const auto end = *reinterpret_cast<const float *const *>(selector + 0x74);
        if (begin && end >= begin && end - begin <= 1000)
          for (const float *value = begin; value != end; ++value)
            std::fprintf(stderr, "transformer-selector-weight[%zu]=%.9g\n",
                         static_cast<std::size_t>(value - begin), *value);
      }
#endif
      transformer->Generate();
#if defined(_M_IX86)
      if (std::getenv("S2_FACE_TRACE_TRANSFORMER"))
      {
        const auto worker = reinterpret_cast<const unsigned char *>(transformer) + 0xa0;
        for (int offset : {0x04, 0x08, 0x10, 0x14, 0x18, 0x1c,
                           0x20, 0x24, 0x28, 0x2c})
          std::fprintf(stderr, "transformer-worker[0x%x]=0x%zx\n", offset,
                       *reinterpret_cast<const std::uintptr_t *>(worker + offset));
      }
      if (const char *prefix = std::getenv("S2_FACE_DUMP_WORKER_PREFIX"))
      {
        const auto worker = reinterpret_cast<const unsigned char *>(transformer) + 0xa0;
        const auto word = [worker](int offset) {
          return *reinterpret_cast<const std::uintptr_t *>(worker + offset);
        };
        const std::size_t muscles = word(0x04);
        const std::size_t bones = word(0x08);
        const std::size_t morphMuscles = word(0x10);
        const auto dump = [prefix, &word](int offset, std::size_t bytes, const char *suffix) {
          const auto *data = reinterpret_cast<const char *>(word(offset));
          return data && WriteBytes(std::string(prefix) + suffix,
              std::vector<char>(data, data + bytes));
        };
        if (muscles > 10000 || bones > 10000 || morphMuscles > 10000 ||
            !dump(0x20, muscles * sizeof(float), "-muscle-scalars.bin") ||
            !dump(0x24, (muscles * 2 + bones) * 40, "-worker-items.bin") ||
            !dump(0x28, morphMuscles * 3 * sizeof(float), "-morph-workspace.bin") ||
            !dump(0x2c, (muscles * 2 + bones) * 3 * sizeof(float), "-output-vectors.bin"))
        {
          std::fprintf(stderr, "cannot dump transformer worker\n");
          ok = false;
        }
      }
#endif
      const int size = animator->SaveBufferSize();
      const int count = animator->VerticesCount();
      std::fprintf(stderr, "generated animator bytes=%d vertices=%d muscles=%d bones=%d\n",
                   size, count, animator->MusclesCount(), animator->BonesCount());
      std::vector<char> generated(size > 0 && size < 100000000 ? size : 0);
      ok = !generated.empty() && animator->Save(generated.data()) &&
           WriteBytes(std::string(argv[3]) + "-generated.bin", generated) &&
           count > 0 && count < 1000000;
      if (ok)
      {
        std::vector<float> vertices(count * 3);
        animator->FillUnused(true);
        ok = animator->Process(vertices.data(), 3);
        FILE *csv = ok ? std::fopen((std::string(argv[3]) + "-vertices.csv").c_str(), "wb") : nullptr;
        ok = ok && csv;
        if (ok)
        {
          std::fprintf(csv, "vertex,x,y,z\n");
          for (int i = 0; i < count; ++i)
            std::fprintf(csv, "%d,%.9g,%.9g,%.9g\n", i,
                         vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
          ok = std::fclose(csv) == 0;
        }
      }
    }
  }
  if (animator) animator->Destroy();
  if (transformer) transformer->Destroy();
  if (tree) tree->Destroy();
  object->Destroy();
  gdp->Destroy();
  return ok ? 0 : 3;
}
