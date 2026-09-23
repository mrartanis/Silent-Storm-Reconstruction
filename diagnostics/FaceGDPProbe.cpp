// Extract the original x86 FaceGen GDP transformer's generated animator and
// vertices. The outputs stay in the ignored lab directory for native parity.
#include <LifeStudioHeadAPIInit.h>
#include <LifeStudioHeadAPIGDP.h>
#include <LifeStudioHeadAPIMMTS.h>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

using namespace LifeStudioHeadAPI;

static bool WriteBytes(const std::string &path, const std::vector<char> &bytes)
{
  std::ofstream file(path, std::ios::binary);
  file.write(bytes.data(), bytes.size());
  return static_cast<bool>(file);
}

int main(int argc, char **argv)
{
  if (argc != 4 && argc != 6)
  {
    std::fprintf(stderr, "usage: FaceGDPProbe head.gdp head.mmt output-prefix [macro-name amplitude]\n");
    return 2;
  }
  char *end = nullptr;
  const float amplitude = argc == 6 ? std::strtof(argv[5], &end) : 0.0f;
  if (argc == 6 && (end == argv[5] || *end)) return 2;
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
  bool ok = defaultsOk && object->IsTransformable() && tree && transformer && animator &&
            tree->Load(argv[2]) && transformer->Load(object);
  if (ok)
  {
    transformer->OutputAnimator(animator);
    transformer->RegisterMacroMuscle(tree->RootMacroMuscle());
    transformer->ClearAllMacroMuscles();
    if (argc == 6)
    {
      IMacroMuscle *macro = tree->FindMacroMuscle(argv[4]);
      if (!macro)
      {
        std::fprintf(stderr, "unknown macro: %s\n", argv[4]);
        ok = false;
      }
      else
        transformer->AddMacroMuscle(macro, amplitude);
    }
    if (ok)
    {
      transformer->ComputePhysics();
      transformer->Generate();
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
