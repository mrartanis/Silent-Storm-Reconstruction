#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/ModManager.h"
#include "../Main/GResource.h"
#include "../DBFormat/DataMap.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>

static std::string ReadGlobalMarker()
{
  NGScene::CResourceFileOpener resource("Globals", 3);
  CDataStream* stream = resource.GetStream();
  std::string bytes(stream->GetSize(), '\0');
  if (!bytes.empty()) stream->Read(&bytes[0], static_cast<unsigned int>(bytes.size()));
  return bytes;
}

int main(int argc, char** argv)
{
  if (argc != 3) return 2;
  const std::filesystem::path source = std::filesystem::absolute(argv[1]);
  const std::filesystem::path parent = std::filesystem::absolute(argv[2]);
  std::error_code error;
  std::filesystem::create_directories(parent, error);
  if (error) return 3;
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path root = parent / ("mod-manager-" + std::to_string(stamp));
  if (!std::filesystem::create_directory(root, error) || error) return 4;
  const std::filesystem::path oldWorkingDirectory = std::filesystem::current_path();
  int result = 0;
  try {
    std::filesystem::copy_file(source, root / "game.db");
    std::filesystem::create_directory(root / "MyMod");
    std::filesystem::create_directories(root / "res" / "Globals");
    std::filesystem::create_directories(root / "MyMod" / "Globals");
    {
      std::ofstream description(root / "MyMod" / "description.txt", std::ios::binary);
      description << "Portable test mod";
      std::ofstream base(root / "res" / "Globals" / "3", std::ios::binary);
      base << "BASE";
      std::ofstream override(root / "MyMod" / "Globals" / "3", std::ios::binary);
      override << "MOD-OVERRIDE";
    }
    std::filesystem::current_path(root);
    std::vector<SModInfo> available;
    CModManager::GetAvailableMods(&available);
    if (available.size() != 1 || available[0].szDirectory != "MyMod" ||
        available[0].szName != "Portable test mod") result = 5;
    if (!result && !CModManager::Activate(std::vector<std::string>{"mYmOd"})) result = 6;
    if (!result && (CModManager::GetBaseVersion() != 1 ||
        CModManager::GetActiveMods()->size() != 1 ||
        CModManager::GetActiveMods()->front().szDirectory != "mYmOd" ||
        !NDb::GetGlobalMap(3))) result = 7;
    if (!result && ReadGlobalMarker() != "MOD-OVERRIDE") result = 13;
    NDb::CGlobalMap* originalMap = result ? nullptr : NDb::GetGlobalMap(3);
    if (!result) {
      const SModInfo missing = {"missing", "NoSuchMod"};
      if (CModManager::Activate(std::vector<SModInfo>{missing}) ||
          CModManager::GetBaseVersion() != 1 ||
          CModManager::GetActiveMods()->size() != 1 ||
          NDb::GetGlobalMap(3) != originalMap ||
          ReadGlobalMarker() != "MOD-OVERRIDE") result = 8;
    }
    if (!result && !CModManager::Activate(std::vector<SModInfo>{})) result = 9;
    if (!result && (CModManager::GetBaseVersion() != 2 ||
        !CModManager::GetActiveMods()->empty() || !NDb::GetGlobalMap(3))) result = 10;
    if (!result && ReadGlobalMarker() != "BASE") result = 14;
    std::filesystem::current_path(oldWorkingDirectory);
  } catch (...) {
    result = 11;
    std::filesystem::current_path(oldWorkingDirectory, error);
  }
  if (!result) std::filesystem::remove_all(root, error);
  if (error && !result) result = 12;
  std::printf("mod_manager_result=%d base_version=%d active_mods=%zu\n",
      result, CModManager::GetBaseVersion(), CModManager::GetActiveMods()->size());
  return result;
}
