#if !defined(_WIN32)
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#include "iSaveManager.h"
#if defined(S2_FULL_GAME)
#include "Interface.h"
#endif
#include "../FileIO/Streams.h"
#include "../FileIO/LinuxUserData.h"
#include "../FileIO/PortableSaveHeader.h"
#include "../FileIO/PortableUserPaths.h"
#include "../MiscDll/Commands.h"

#include <codecvt>
#include <cstdint>
#include <ctime>
#include <cwchar>
#include <filesystem>
#include <locale>
#include <stdexcept>
#include <sys/stat.h>
#include <system_error>
#include <vector>

namespace NMainLoop {
namespace {
namespace fs = std::filesystem;

bool IsDirectory(const fs::path& path) {
  std::error_code error;
  const fs::file_status status = fs::symlink_status(path, error);
  return !error && fs::is_directory(status) && !fs::is_symlink(status);
}

bool EnsureDirectory(const fs::path& path) {
  if (!path.is_absolute()) return false;
  fs::path current = path.root_path();
  for (const auto& component : path.relative_path()) {
    current /= component;
    std::error_code error;
    fs::file_status status = fs::symlink_status(current, error);
    if (error == std::errc::no_such_file_or_directory) error.clear();
    if (error) return false;
    if (status.type() == fs::file_type::not_found) {
      if (!fs::create_directory(current, error) || error) return false;
      status = fs::symlink_status(current, error);
      if (error) return false;
    }
    if (!fs::is_directory(status) || fs::is_symlink(status)) return false;
  }
  return true;
}

fs::path SaveRoot() {
  const std::string& root = S2FileIO::LinuxUserDataRoot();
  if (root.empty()) return {};
  const fs::path path = fs::path(root) / "save";
  return EnsureDirectory(path) ? path : fs::path();
}

bool DecodeName(const std::string& encoded, fs::path* result) {
  std::wstring name;
  if (!S2FileIO::DecodeLinuxProfileConfig(encoded, &name) ||
      !S2FileIO::IsSafeSaveComponent(name)) return false;
  try {
    *result = fs::u8path(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(name));
    return true;
  } catch (const std::range_error&) {
    return false;
  }
}

std::string EncodeName(const fs::path& path) {
  try {
    const std::wstring name =
      std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(path.u8string());
    return S2FileIO::IsSafeSaveComponent(name) ?
      S2FileIO::EncodeLinuxProfileConfig(name) : std::string();
  } catch (const std::range_error&) {
    return {};
  }
}

fs::path ProfileDir(const std::string& profile) {
  fs::path name;
  const fs::path root = SaveRoot();
  if (root.empty() || !DecodeName(profile, &name)) return {};
  const fs::path path = root / name;
  std::error_code error;
  const fs::file_status status = fs::symlink_status(path, error);
  if (error == std::errc::no_such_file_or_directory) error.clear();
  return !error && !fs::is_symlink(status) ? path : fs::path();
}

fs::path SlotDir(const std::string& profile, const std::string& slot) {
  fs::path name;
  const fs::path parent = ProfileDir(profile);
  if (parent.empty() || !DecodeName(slot, &name)) return {};
  const fs::path path = parent / name;
  std::error_code error;
  const fs::file_status status = fs::symlink_status(path, error);
  if (error == std::errc::no_such_file_or_directory) error.clear();
  return !error && !fs::is_symlink(status) ? path : fs::path();
}

bool RemoveTree(const fs::path& path) {
  std::error_code error;
  const fs::file_status root = fs::symlink_status(path, error);
  if (error == std::errc::no_such_file_or_directory) return true;
  if (error || root.type() == fs::file_type::not_found) return !error;
  if (!fs::is_directory(root) || fs::is_symlink(root)) return false;
  for (const auto& entry : fs::directory_iterator(path, error)) {
    if (error) return false;
    const fs::file_status status = entry.symlink_status(error);
    if (error || fs::is_symlink(status) || EncodeName(entry.path().filename()).empty())
      return false;
    if (fs::is_directory(status)) {
      if (!RemoveTree(entry.path())) return false;
    } else if (fs::is_regular_file(status)) {
      if (!fs::remove(entry.path(), error) || error) return false;
    } else return false;
  }
  return !error && fs::remove(path, error) && !error;
}

void CopyFiles(const fs::path& source, const fs::path& target) {
  if (!IsDirectory(source) || !IsDirectory(target)) return;
  std::error_code error;
  for (const auto& entry : fs::directory_iterator(source, error)) {
    if (error) return;
    const fs::file_status status = entry.symlink_status(error);
    if (error || !fs::is_regular_file(status) ||
        EncodeName(entry.path().filename()).empty()) continue;
    const fs::path destination = target / entry.path().filename();
    const fs::file_status existing = fs::symlink_status(destination, error);
    if (error == std::errc::no_such_file_or_directory) error.clear();
    if (error || fs::is_symlink(existing)) continue;
    fs::copy_file(entry.path(), destination, fs::copy_options::overwrite_existing, error);
    if (error) return;
  }
}
} // namespace

void ReadSaveFileHeader(CFileStream& stream, SSaveFileHeader* header) {
  if (!header) throw std::runtime_error("null save header");
  std::vector<std::uint8_t> wire(S2FileIO::kSaveHeaderWireSize);
  stream.Read(wire.data(), static_cast<unsigned int>(wire.size()));
  S2FileIO::SaveHeaderData decoded;
  if (!S2FileIO::DecodeSaveHeader(wire.data(), wire.size(), &decoded))
    throw std::runtime_error("invalid save header");
  header->nMagic = decoded.magic;
  header->nMods = decoded.activeMods;
  for (int y = 0; y < N_SAVE_SCREENSHOT_Y; ++y)
    for (int x = 0; x < N_SAVE_SCREENSHOT_X; ++x) {
      const auto& pixel = decoded.screenshot[y * N_SAVE_SCREENSHOT_X + x];
      header->sScreenShot[y][x] = NGfx::SPixel8888(
        pixel.red, pixel.green, pixel.blue, pixel.alpha);
    }
}

void WriteSaveFileHeader(CFileStream& stream, const SSaveFileHeader& header) {
  S2FileIO::SaveHeaderData value;
  value.magic = header.nMagic;
  value.activeMods = header.nMods;
  value.screenshot.resize(S2FileIO::kSaveScreenshotPixels);
  for (int y = 0; y < N_SAVE_SCREENSHOT_Y; ++y)
    for (int x = 0; x < N_SAVE_SCREENSHOT_X; ++x) {
      const auto& pixel = header.sScreenShot[y][x];
      value.screenshot[y * N_SAVE_SCREENSHOT_X + x] = {
        static_cast<std::uint8_t>(pixel.r), static_cast<std::uint8_t>(pixel.g),
        static_cast<std::uint8_t>(pixel.b), static_cast<std::uint8_t>(pixel.a)};
    }
  std::vector<std::uint8_t> wire(S2FileIO::kSaveHeaderWireSize);
  if (!S2FileIO::EncodeSaveHeader(value, wire.data(), wire.size()))
    throw std::runtime_error("invalid save header");
  stream.Write(wire.data(), static_cast<unsigned int>(wire.size()));
}

CSaveManager* GetSaveManager() {
  static CSaveManager manager;
  return &manager;
}
CSaveManager::CSaveManager() : nActiveSlotID(0) {}
void CSaveManager::CreateProfile(const string& profile) const {
  const fs::path path = ProfileDir(profile);
  if (!path.empty()) EnsureDirectory(path);
}
void CSaveManager::DeleteProfile(const string& profile) const {
  const fs::path path = ProfileDir(profile);
  if (!path.empty()) (void)RemoveTree(path);
}
void CSaveManager::GetProfilesList(list<string>* result) const {
  const fs::path root = SaveRoot();
  if (!result || root.empty()) return;
  std::error_code error;
  for (const auto& entry : fs::directory_iterator(root, error))
    if (!error && IsDirectory(entry.path())) {
      const std::string name = EncodeName(entry.path().filename());
      if (!name.empty()) result->push_back(name);
    }
}
string CSaveManager::GetActiveProfile() const { return NMainLoop::GetActiveProfile(); }
void CSaveManager::SetActiveProfile(const string& profile) { NMainLoop::SetActiveProfile(profile); }
void CSaveManager::SaveSlot(const string& name) {
  const string profile = GetActiveProfile();
  const fs::path source = SlotDir(profile, S_SLOT_ACTIVE), target = SlotDir(profile, name);
  if (source.empty() || target.empty()) return;
  if (source == target) { EnsureDirectory(target); return; }
  if (!RemoveTree(target)) return;
  if (EnsureDirectory(source) && EnsureDirectory(target)) CopyFiles(source, target);
}
void CSaveManager::LoadSlot(const string& name) {
  const string profile = GetActiveProfile();
  const fs::path source = SlotDir(profile, name), target = SlotDir(profile, S_SLOT_ACTIVE);
  if (source.empty() || target.empty()) return;
  if (source == target) { EnsureDirectory(target); return; }
  if (!RemoveTree(target)) return;
  if (EnsureDirectory(source) && EnsureDirectory(target)) CopyFiles(source, target);
}
void CSaveManager::ClearSlot(const string& name) {
  const fs::path path = SlotDir(GetActiveProfile(), name);
  if (!path.empty() && RemoveTree(path)) EnsureDirectory(path);
}
void CSaveManager::DeleteSlot(const string& name) {
  const fs::path path = SlotDir(GetActiveProfile(), name);
  if (!path.empty()) (void)RemoveTree(path);
}
void CSaveManager::PrepareSlot(const string& name) {
  const fs::path path = SlotDir(GetActiveProfile(), name);
  if (!path.empty()) EnsureDirectory(path);
}
void CSaveManager::GetSlotsList(list<string>* result) const {
  const fs::path root = ProfileDir(GetActiveProfile());
  if (!result || !IsDirectory(root)) return;
  std::error_code error;
  for (const auto& entry : fs::directory_iterator(root, error))
    if (!error && entry.path().filename() != S_SLOT_ACTIVE && IsDirectory(entry.path())) {
      const std::string name = EncodeName(entry.path().filename());
      if (!name.empty()) result->push_back(name);
    }
}
wstring CSaveManager::GetSlotFilePathW(const string& slot, const string& file) const {
  const fs::path path = SlotDir(GetActiveProfile(), slot);
  fs::path name;
  if (path.empty() || !DecodeName(file, &name)) return {};
  std::error_code error;
  const fs::file_status status = fs::symlink_status(path / name, error);
  if (error == std::errc::no_such_file_or_directory) error.clear();
  if (error || fs::is_symlink(status)) return {};
  try {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes((path / name).u8string());
  } catch (const std::range_error&) {
    return {};
  }
}
void CSaveManager::GetSlotScreenShot(const string& slot,
    CArray2D<NGfx::SPixel8888>* screenshot) {
  if (!screenshot) return;
  CFileStream file;
  const wstring path = GetSlotFilePathW(slot, S_SAVE_FILENAME);
  if (path.empty()) return;
  file.OpenRead(path.c_str());
  SSaveFileHeader header;
  ReadSaveFileHeader(file, &header);
  if (header.nMagic != N_SAVE_MAGIC_NUMBER && header.nMagic != N_SAVE_MAGIC_NUMBER_V0)
    throw std::runtime_error("invalid save magic");
  screenshot->SetSizes(N_SAVE_SCREENSHOT_X, N_SAVE_SCREENSHOT_Y);
  for (int y = 0; y < N_SAVE_SCREENSHOT_Y; ++y)
    for (int x = 0; x < N_SAVE_SCREENSHOT_X; ++x)
      (*screenshot)[y][x] = header.sScreenShot[y][x];
}
void CSaveManager::GetSlotTime(const string& slot, wstring* time) {
  int dateKey = 0, timeKey = 0;
  NMainLoop::GetSlotTime(slot, time, &dateKey, &timeKey);
}

void CreateProfile(const string& name) { GetSaveManager()->CreateProfile(name); }
void DeleteProfile(const string& name) { GetSaveManager()->DeleteProfile(name); }
void GetProfilesList(list<string>* result) { GetSaveManager()->GetProfilesList(result); }
void MakeDefaultProfile() {
  list<string> profiles;
  GetProfilesList(&profiles);
  if (profiles.empty()) {
    NGlobal::ResetVar("game_profile");
    CreateProfile(S2FileIO::EncodeLinuxProfileConfig(NGlobal::GetVar("game_profile").GetString()));
  }
}
string GetActiveProfile() {
  string active = S2FileIO::EncodeLinuxProfileConfig(NGlobal::GetVar("game_profile").GetString());
  list<string> profiles;
  GetProfilesList(&profiles);
  if (std::find(profiles.begin(), profiles.end(), active) == profiles.end()) {
    if (profiles.empty()) MakeDefaultProfile();
    else {
      std::wstring name;
      if (S2FileIO::DecodeLinuxProfileConfig(profiles.front(), &name))
        NGlobal::SetVar("game_profile", NGlobal::CValue(name));
    }
  }
  return S2FileIO::EncodeLinuxProfileConfig(NGlobal::GetVar("game_profile").GetString());
}
void SetActiveProfile(const string& encoded) {
  std::wstring name;
  if (S2FileIO::DecodeLinuxProfileConfig(encoded, &name) &&
      S2FileIO::IsSafeSaveComponent(name))
    NGlobal::SetVar("game_profile", NGlobal::CValue(name));
}
void GetSlotTime(const string& slot, wstring* time, int* dateKey, int* timeKey) {
  if (!time || !dateKey || !timeKey) return;
  const wstring wide = GetSaveManager()->GetSlotFilePathW(slot, S_SAVE_FILENAME);
  if (wide.empty()) return;
  std::string path;
  try {
    path = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wide);
  } catch (const std::range_error&) {
    return;
  }
  struct stat info;
  if (stat(path.c_str(), &info) != 0) return;
  struct tm local;
  if (!localtime_r(&info.st_mtime, &local)) return;
  *dateKey = ((local.tm_year * 12 + local.tm_mon) * 31 + local.tm_mday) * 24 + local.tm_hour;
  *timeKey = local.tm_min * 3660 + local.tm_sec;
  wchar_t date[64], clock[64];
  if (std::wcsftime(date, 64, L"%x", &local) &&
      std::wcsftime(clock, 64, L"%H:%M:%S", &local))
    *time = wstring(date) + L"<tab>" + clock;
}
#if defined(S2_FULL_GAME)
string GetQuickSaveSlot( bool bLoad )
{
	// Retail v1.2 0x637a70: save to the older slot, load the newer one.
	string first = S2FileIO::EncodeLinuxProfileConfig( NUI::GetDBString( 20241 ) );
	string second = S2FileIO::EncodeLinuxProfileConfig( NUI::GetDBString( 20242 ) );
	int firstDate = 0, firstTime = 0, secondDate = 0, secondTime = 0;
	wstring time;
	GetSlotTime( first, &time, &firstDate, &firstTime );
	GetSlotTime( second, &time, &secondDate, &secondTime );
	bool bFirstNewer = firstDate != secondDate ? firstDate > secondDate : firstTime > secondTime;
	return ( bLoad ? bFirstNewer : !bFirstNewer ) ? first : second;
}

bool IsValidCustomName(const string& encoded) {
  std::wstring name;
  if (!S2FileIO::DecodeLinuxProfileConfig(encoded, &name) ||
      !S2FileIO::IsSafeSaveComponent(name)) return false;
  for (int id = 20241; id <= 20244; ++id)
    if (name == NUI::GetDBString(id)) return false;
  return true;
}

#endif
} // namespace NMainLoop
#endif
