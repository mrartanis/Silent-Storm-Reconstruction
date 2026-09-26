#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#include "../Main/iSaveManager.h"
#include "../FileIO/Streams.h"
#include "../FileIO/LinuxUserData.h"
#include "../MiscDll/Commands.h"

#include <algorithm>
#include <codecvt>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
  namespace fs = std::filesystem;
  const char* overrideRoot = std::getenv("S2_USER_DATA_DIR");
  if (!overrideRoot || S2FileIO::LinuxUserDataRoot() != overrideRoot) return 1;
  const fs::path cwd = fs::current_path();

  NGlobal::RegisterVar("game_profile", nullptr, nullptr,
    NGlobal::CValue(std::wstring(L"Default")), true);
  const std::wstring profile = L"Тестовый профиль";
  const std::wstring slot = L"Полевое сохранение";
  const std::string encodedProfile = S2FileIO::EncodeLinuxProfileConfig(profile);
  const std::string encodedSlot = S2FileIO::EncodeLinuxProfileConfig(slot);
  NMainLoop::CreateProfile(encodedProfile);
  NMainLoop::SetActiveProfile(encodedProfile);
  if (NMainLoop::GetActiveProfile() != encodedProfile) return 2;
  NMainLoop::CSaveManager* manager = NMainLoop::GetSaveManager();
  manager->PrepareSlot(NMainLoop::S_SLOT_ACTIVE);
  const std::wstring active = manager->GetSlotFilePathW(
    NMainLoop::S_SLOT_ACTIVE, NMainLoop::S_SAVE_FILENAME);
  if (active.empty()) return 3;

  NMainLoop::SSaveFileHeader written{};
  written.nMagic = NMainLoop::N_SAVE_MAGIC_NUMBER;
  written.nMods = 2;
  written.sScreenShot[0][0] = NGfx::SPixel8888(11, 22, 33, 44);
  written.sScreenShot[199][319] = NGfx::SPixel8888(55, 66, 77, 88);
  {
    CFileStream file;
    file.OpenWrite(active.c_str());
    NMainLoop::WriteSaveFileHeader(file, written);
    const unsigned char payload[] = {1, 2, 3, 4};
    file.Write(payload, sizeof(payload));
  }
  manager->SaveSlot(encodedSlot);
  const std::wstring saved = manager->GetSlotFilePathW(encodedSlot, NMainLoop::S_SAVE_FILENAME);
  const fs::path savedPath = fs::u8path(
    std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(saved));
  if (saved.empty() || !fs::is_regular_file(savedPath)) return 4;
  {
    CFileStream file;
    file.OpenWrite(active.c_str());
    const unsigned char changed = 99;
    file.Write(&changed, 1);
  }
  manager->LoadSlot(encodedSlot);
  {
    CFileStream file;
    file.OpenRead(active.c_str());
    NMainLoop::SSaveFileHeader read{};
    NMainLoop::ReadSaveFileHeader(file, &read);
    unsigned char payload[4]{};
    file.Read(payload, sizeof(payload));
    if (read.nMagic != written.nMagic || read.nMods != 2 ||
        read.sScreenShot[0][0].r != 11 ||
        read.sScreenShot[199][319].b != 77 ||
        payload[0] != 1 || payload[3] != 4) return 5;
  }
  CArray2D<NGfx::SPixel8888> screenshot;
  manager->GetSlotScreenShot(encodedSlot, &screenshot);
  if (screenshot[0][0].g != 22 || screenshot[199][319].a != 88) return 6;

  std::list<std::string> profiles, slots;
  manager->GetProfilesList(&profiles);
  manager->GetSlotsList(&slots);
  if (std::find(profiles.begin(), profiles.end(), encodedProfile) == profiles.end() ||
      std::find(slots.begin(), slots.end(), encodedSlot) == slots.end() ||
      std::find(slots.begin(), slots.end(), "temp") != slots.end()) return 7;
  if (!manager->GetSlotFilePathW("../escape", "game.sav").empty() ||
      !manager->GetSlotFilePathW(encodedSlot, "../escape").empty()) return 8;

  const fs::path saveRoot = fs::path(overrideRoot) / "save";
  const fs::path outside = fs::path(overrideRoot) / "outside";
  fs::create_directories(outside);
  const fs::path linked = saveRoot / "Linked";
  std::error_code error;
  fs::remove(linked, error);
  error.clear();
  fs::create_directory_symlink(outside, linked, error);
  if (error) return 9;
  manager->CreateProfile("Linked");
  if (fs::exists(outside / "temp") ||
      !manager->GetSlotFilePathW(encodedSlot, "bad/name").empty()) return 10;
  const fs::path linkedFile = savedPath.parent_path() / "linked.sav";
  fs::create_symlink(outside / "victim", linkedFile, error);
  if (error || !manager->GetSlotFilePathW(encodedSlot, "linked.sav").empty()) return 13;
  manager->SaveSlot(encodedSlot);
  if (!fs::is_symlink(linkedFile) || fs::exists(outside / "victim")) return 14;
  fs::remove(linkedFile, error);
  if (error) return 15;
  manager->DeleteSlot(encodedSlot);
  if (fs::exists(savedPath)) return 11;
  if (fs::current_path() != cwd || fs::exists(cwd / "save")) return 12;
  std::cout << "native Linux save manager: passed\n";
  return 0;
}
