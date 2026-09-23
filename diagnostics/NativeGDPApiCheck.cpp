#include <LifeStudioHeadAPIGDP.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static std::vector<char> Read(const char *path)
{
  std::ifstream file(path, std::ios::binary);
  if (!file) return {};
  return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

int main(int argc, char **argv)
{
  if (argc != 5 && argc != 6)
  {
    std::fprintf(stderr, "usage: NativeGDPApiCheck head.gdp x86-default.bin x86-Morph.txt x86-EuroM_A.mld [x86-dump-dir]\n");
    return 2;
  }
  LifeStudioHeadAPI::IGDPFile *file = LifeStudioHeadAPI::IGDPFile::Create(argv[1]);
  if (!file) return 3;
  const bool named = file->ObjectsCount() == 1 && file->ObjectName(0) &&
                     std::strcmp(file->ObjectName(0), "800") == 0;
  LifeStudioHeadAPI::IGDPObject *object = named ? file->Object(0) : nullptr;
  bool ok = object && object->IsTransformable() && object->VerticesCount() == 419 &&
            object->SubObjectsCount() == 7 && object->DataListSize() == 50 &&
            object->DataListItem(0) &&
            std::strcmp(object->DataListItem(0), "Morph.txt") == 0 &&
            object->DataListItem(49) &&
            std::strcmp(object->DataListItem(49), "Links.dat") == 0;
  if (ok)
  {
    const std::vector<char> expectedDefault = Read(argv[2]);
    const std::vector<char> expectedMorph = Read(argv[3]);
    const std::vector<char> expectedEuro = Read(argv[4]);
    std::vector<char> actualDefault(expectedDefault.size());
    std::vector<char> actualMorph(expectedMorph.size());
    std::vector<char> actualEuro(expectedEuro.size());
    ok = !expectedDefault.empty() && !expectedMorph.empty() && !expectedEuro.empty() &&
         object->DefaultAnimatorDataSize() == int(expectedDefault.size()) &&
         object->Size("Morph.txt") == int(expectedMorph.size()) &&
         object->Size("EuroM_A.mld") == int(expectedEuro.size()) &&
         object->DefaultAnimatorData(actualDefault.data()) &&
         object->Get("Morph.txt", actualMorph.data()) &&
         object->Get("EuroM_A.mld", actualEuro.data()) &&
         actualDefault == expectedDefault && actualMorph == expectedMorph &&
         actualEuro == expectedEuro;
    if (ok && argc == 6)
      for (int i = 0; i < object->DataListSize(); ++i)
      {
        const char *name = object->DataListItem(i);
        char numbered[32];
        std::snprintf(numbered, sizeof(numbered), "%02d-", i);
        const std::vector<char> expected = Read((std::string(argv[5]) + "\\" +
                                                 numbered + name).c_str());
        std::vector<char> actual(expected.size());
        if (expected.empty() || object->Size(name) != int(expected.size()) ||
            !object->Get(name, actual.data()) || actual != expected)
        {
          std::fprintf(stderr, "GDP data item mismatch: %d %s\n", i, name);
          ok = false;
          break;
        }
      }
    std::printf("objects=%d subobjects=%d data-items=%d default=%zu morph=%zu euro=%zu\n",
                file->ObjectsCount(), object->SubObjectsCount(), object->DataListSize(),
                actualDefault.size(), actualMorph.size(), actualEuro.size());
  }
  if (object) object->Destroy();
  file->Destroy();
  return ok ? 0 : 3;
}
