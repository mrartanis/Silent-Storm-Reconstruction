#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/ChapterInfo.h"
#include "../Main/GlobalInfo.h"
#include "../FileIO/PortablePackageIndex.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>

static std::uint64_t digest = 14695981039346656037ULL;
static void Add(std::uint32_t value)
{
  for (int i = 0; i < 4; ++i) {
    digest ^= static_cast<unsigned char>(value >> (i * 8));
    digest *= 1099511628211ULL;
  }
}
static void AddFloat(float value)
{
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  Add(bits);
}
static void AddPoint(const CVec2& point)
{
  AddFloat(point.x);
  AddFloat(point.y);
}
static void AddString(const std::string& value)
{
  Add(static_cast<std::uint32_t>(value.size()));
  for (unsigned char ch : value) {
    digest ^= ch;
    digest *= 1099511628211ULL;
  }
}

int main(int argc, char** argv)
{
  if (argc != 3) return 2;
  S2FileIO::PortablePackageIndex chapters, globals;
  if (!chapters.Open(argv[1]) || !globals.Open(argv[2])) return 3;
  if (chapters.Entries().empty() || globals.Entries().empty()) return 4;
  NGScene::AddResourceDir(std::filesystem::path(argv[1]).parent_path().string().c_str());
  unsigned int chapterSectors = 0, globalSectors = 0;
  unsigned int chapterData = 0, chapterImages = 0, globalPartial = 0;
  const char* kind = "Chapters";
  int resourceID = -1;
  const char* phase = "raw";
  std::uint32_t resourceLength = 0;
  try {
    for (const auto& entry : chapters.Entries()) {
      resourceID = entry.first;
      resourceLength = entry.second.length;
      // The seven large low-numbered entries are chapter-map image payloads,
      // not CChapterInfo records. Their typed parser fails at scalar tag 2/3.
      if (entry.second.length > 100000) {
        ++chapterImages;
        continue;
      }
      phase = "strict";
      {
        NGScene::CResourceOpener raw("Chapters", entry.first);
        CObj<CChapterInfo> strict = new CChapterInfo;
        strict->operator&(*raw.operator->());
      }
      ++chapterData;
      phase = "loader";
      CObj<CChapterInfoLoader> loader = new CChapterInfoLoader;
      loader->SetKey(entry.first);
      CChapterInfo* chapter = loader->GetValue();
      if (!chapter) return 5;
      Add(static_cast<std::uint32_t>(entry.first));
      Add(static_cast<std::uint32_t>(chapter->nMapID));
      AddPoint(chapter->vDeployPos);
      Add(static_cast<std::uint32_t>(chapter->sectorsSet.size()));
      chapterSectors += static_cast<unsigned int>(chapter->sectorsSet.size());
      for (const auto& sector : chapter->sectorsSet) {
        Add(static_cast<std::uint32_t>(sector.nTemplate));
        Add(static_cast<std::uint32_t>(sector.nProbability));
        Add(static_cast<std::uint32_t>(sector.nDescriptionID));
        Add(static_cast<std::uint32_t>(sector.eType));
        AddString(sector.szID);
        Add(static_cast<std::uint32_t>(sector.pointsSet.size()));
        for (const auto& point : sector.pointsSet) AddPoint(point);
      }
    }
    kind = "Globals";
    for (const auto& entry : globals.Entries()) {
      resourceID = entry.first;
      resourceLength = entry.second.length;
      phase = "strict";
      try {
        NGScene::CResourceOpener raw("Globals", entry.first);
        CObj<CGlobalInfo> strict = new CGlobalInfo;
        strict->operator&(*raw.operator->());
      } catch (const std::exception& error) {
        // The original Globals/2 record ends in a 6-byte point at tag 5.
        // The game loader catches this mismatch and returns its already-read
        // data. Pin that behavior explicitly so a different failure is not
        // silently accepted as a valid resource.
        if (entry.first != 2 || std::strcmp(error.what(),
            "structure field size mismatch: tag=5 wire=6 memory=8") != 0)
          throw;
        ++globalPartial;
      }
      phase = "loader";
      CObj<CGlobalInfoLoader> loader = new CGlobalInfoLoader;
      loader->SetKey(entry.first);
      CGlobalInfo* global = loader->GetValue();
      if (!global) return 6;
      if (entry.first == 2 && (global->nMapID != 523 ||
          global->sectorsSet.size() != 2)) return 8;
      Add(static_cast<std::uint32_t>(entry.first));
      Add(static_cast<std::uint32_t>(global->nMapID));
      Add(static_cast<std::uint32_t>(global->nScenarioID));
      Add(static_cast<std::uint32_t>(global->sectorsSet.size()));
      globalSectors += static_cast<unsigned int>(global->sectorsSet.size());
      for (const auto& sector : global->sectorsSet) {
        Add(static_cast<std::uint32_t>(sector.nImageID));
        Add(static_cast<std::uint32_t>(sector.nTemplate));
        Add(static_cast<std::uint32_t>(sector.nDescriptionID));
        AddPoint(sector.vImagePos);
        Add(static_cast<std::uint32_t>(sector.pointsSet.size()));
        for (const auto& point : sector.pointsSet) AddPoint(point);
      }
    }
  } catch (...) {
    std::fprintf(stderr, "campaign resource decode failed: %s id=%d bytes=%u phase=%s\n",
      kind, resourceID, resourceLength, phase);
    return 7;
  }
  NGScene::CloseAllResources();
  std::printf("chapters=%u chapter_images=%u chapter_sectors=%u globals=%zu partial_globals=%u global_sectors=%u digest=%016llX\n",
    chapterData, chapterImages, chapterSectors,
    globals.Entries().size(), globalPartial, globalSectors,
    static_cast<unsigned long long>(digest));
  if (chapterData != 20 || chapterImages != 7 || globalPartial != 1 ||
      chapterSectors != 176 || globalSectors != 19 ||
      digest != UINT64_C(0x4833295F6BB75174)) return 9;
  return 0;
}
