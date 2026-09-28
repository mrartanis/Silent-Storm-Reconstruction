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
#include "../DBFormat/DataScenario.h"
#include "../DBFormat/DataMisc.h"
#include "../FileIO/PortableGameDatabase.h"
#include "../FileIO/PortableSaveHeader.h"

#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>

static void AppendI32(std::vector<std::uint8_t>* bytes, std::int32_t value)
{
  const std::uint32_t bits = static_cast<std::uint32_t>(value);
  for (unsigned shift = 0; shift < 32; shift += 8)
    bytes->push_back(static_cast<std::uint8_t>(bits >> shift));
}

// A complete release-format database is a legitimate game mod. Patch one
// columnar value in a private copy without changing the chunk lengths.
static std::pair<int, int> WriteMapOverlay(const std::filesystem::path& source,
                                           const std::filesystem::path& destination)
{
  S2FileIO::PortableGameDatabase database;
  std::string error;
  if (!S2FileIO::LoadPortableGameDatabase(source.string(), &database, &error))
    throw std::runtime_error(error);
  const auto table = std::find_if(database.tables.begin(), database.tables.end(),
      [](const S2FileIO::GameDatabaseTable& item) { return item.tableId == 16; });
  if (table == database.tables.end()) throw std::runtime_error("GlobalMaps missing");
  const auto idName = std::find(table->intNames.begin(), table->intNames.end(), "ID");
  const auto zoneName = std::find(table->intNames.begin(), table->intNames.end(), "StartZoneID");
  if (idName == table->intNames.end() || zoneName == table->intNames.end())
    throw std::runtime_error("GlobalMaps columns missing");
  const std::size_t idIndex = static_cast<std::size_t>(idName - table->intNames.begin());
  const std::size_t zoneIndex = static_cast<std::size_t>(zoneName - table->intNames.begin());
  const std::vector<std::int32_t>* target = nullptr;
  int replacement = 0;
  for (const auto& row : table->intRows) {
    if (row.size() != table->intNames.size()) throw std::runtime_error("invalid map row");
    if (row[idIndex] == 3) target = &row;
    if (row[idIndex] == 4) replacement = row[zoneIndex];
  }
  if (!target || !replacement || (*target)[zoneIndex] == replacement)
    throw std::runtime_error("cannot choose map zone overlay");
  std::vector<std::uint8_t> pattern;
  for (std::int32_t value : *target) AppendI32(&pattern, value);
  std::ifstream input(source, std::ios::binary);
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
  if (bytes.empty()) throw std::runtime_error("cannot read source database");
  const auto found = std::search(bytes.begin(), bytes.end(), pattern.begin(), pattern.end());
  if (found == bytes.end() ||
      std::search(found + 1, bytes.end(), pattern.begin(), pattern.end()) != bytes.end())
    throw std::runtime_error("map row bytes are not unique");
  const std::size_t valueOffset = static_cast<std::size_t>(found - bytes.begin()) + 4 * zoneIndex;
  std::vector<std::uint8_t> patched;
  AppendI32(&patched, replacement);
  std::copy(patched.begin(), patched.end(), bytes.begin() + valueOffset);
  std::ofstream output(destination, std::ios::binary);
  output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (!output) throw std::runtime_error("cannot write mod database");
  output.close();
  if (!output) throw std::runtime_error("cannot close mod database");
  S2FileIO::PortableGameDatabase check;
  if (!S2FileIO::LoadPortableGameDatabase(destination.string(), &check, &error))
    throw std::runtime_error("patched mod database is invalid: " + error);
  return {(*target)[zoneIndex], replacement};
}

static int StartZoneId()
{
  NDb::CGlobalMap* map = NDb::GetGlobalMap(3);
  return map && IsValid(map->pStartZone) ? map->pStartZone->GetRecordID() : 0;
}

static std::map<int, int> ActionPoints()
{
  std::map<int, int> values;
  CDBTable<NDb::CRPGAP>* table = NDatabase::GetTable<NDb::CRPGAP>();
  CDBIterator<NDb::CRPGAP> cursor(*table);
  while (cursor.MoveNext())
    values.emplace(cursor.Get()->GetRecordID(), cursor.Get()->nAP);
  return values;
}

static std::map<int, int> ModActionPoints(const std::filesystem::path& source)
{
  S2FileIO::PortableGameDatabase database;
  std::string error;
  if (!S2FileIO::LoadPortableGameDatabase(source.string(), &database, &error))
    throw std::runtime_error(error);
  if (database.tables.size() != 1 || database.tables.front().tableId != 0x7e ||
      !database.relations.empty())
    throw std::runtime_error("expected shipped partial RPGAP mod");
  const auto& table = database.tables.front();
  const auto idName = std::find(table.intNames.begin(), table.intNames.end(), "ID");
  const auto apName = std::find(table.intNames.begin(), table.intNames.end(), "AP");
  if (idName == table.intNames.end() || apName == table.intNames.end())
    throw std::runtime_error("RPGAP mod columns missing");
  const std::size_t idIndex = static_cast<std::size_t>(idName - table.intNames.begin());
  const std::size_t apIndex = static_cast<std::size_t>(apName - table.intNames.begin());
  std::map<int, int> values;
  for (const auto& row : table.intRows) {
    if (row.size() != table.intNames.size() ||
        !values.emplace(row[idIndex], row[apIndex]).second)
      throw std::runtime_error("invalid RPGAP mod row");
  }
  return values;
}

static std::vector<std::string> RoundTripModSaveHeader(const std::filesystem::path& path)
{
  S2FileIO::SaveHeaderData header;
  header.magic = 0x828ca022;
  header.activeMods = static_cast<std::int32_t>(CModManager::GetActiveMods()->size());
  header.screenshot.resize(S2FileIO::kSaveScreenshotPixels);
  std::vector<std::uint8_t> wire(S2FileIO::kSaveHeaderWireSize);
  if (!S2FileIO::EncodeSaveHeader(header, wire.data(), wire.size()))
    throw std::runtime_error("cannot encode mod save header");
  {
    CFileStream output;
    output.OpenWrite(path.string().c_str());
    output.Write(wire.data(), static_cast<unsigned int>(wire.size()));
    for (const auto& mod : *CModManager::GetActiveMods())
      output.WriteString(mod.szDirectory);
  }
  {
    CFileStream input;
    input.OpenRead(path.string().c_str());
    input.Read(wire.data(), static_cast<unsigned int>(wire.size()));
    S2FileIO::SaveHeaderData decoded;
    if (!S2FileIO::DecodeSaveHeader(wire.data(), wire.size(), &decoded) ||
        decoded.magic != header.magic || decoded.activeMods != header.activeMods)
      throw std::runtime_error("cannot decode mod save header");
    std::vector<std::string> directories(decoded.activeMods);
    for (auto& directory : directories) input.ReadString(directory);
    if (input.GetPosition() != input.GetSize())
      throw std::runtime_error("mod save header has trailing bytes");
    return directories;
  }
}

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
  if (argc != 3 && argc != 4) return 2;
  const std::filesystem::path source = std::filesystem::absolute(argv[1]);
  const std::filesystem::path parent = std::filesystem::absolute(argv[2]);
  const std::filesystem::path shippedMod = argc == 4 ?
      std::filesystem::absolute(argv[3]) : std::filesystem::path();
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
    const auto zones = WriteMapOverlay(source, root / "MyMod" / "game.db");
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
        !NDb::GetGlobalMap(3) || StartZoneId() != zones.second)) result = 7;
    if (!result && ReadGlobalMarker() != "MOD-OVERRIDE") result = 13;
    const std::vector<std::string> savedMods = result ? std::vector<std::string>() :
        RoundTripModSaveHeader(root / "modded-save-header.bin");
    if (!result && (savedMods.size() != 1 || savedMods.front() != "mYmOd")) result = 15;
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
        !CModManager::GetActiveMods()->empty() || !NDb::GetGlobalMap(3) ||
        StartZoneId() != zones.first)) result = 10;
    if (!result && ReadGlobalMarker() != "BASE") result = 14;
    if (!result && !CModManager::Activate(savedMods)) result = 16;
    if (!result && (CModManager::GetBaseVersion() != 3 ||
        CModManager::GetActiveMods()->size() != 1 ||
        StartZoneId() != zones.second || ReadGlobalMarker() != "MOD-OVERRIDE")) result = 17;
    if (!result && !CModManager::Activate(std::vector<SModInfo>{})) result = 18;
    if (!result && (CModManager::GetBaseVersion() != 4 ||
        !CModManager::GetActiveMods()->empty() || StartZoneId() != zones.first ||
        ReadGlobalMarker() != "BASE")) result = 19;
    if (!result && !shippedMod.empty()) {
      const std::map<int, int> expected = ModActionPoints(shippedMod);
      const std::map<int, int> base = ActionPoints();
      if (expected.empty() || base.size() <= expected.size()) result = 20;
      std::filesystem::create_directory(root / "APMod");
      std::filesystem::copy_file(shippedMod, root / "APMod" / "game.db");
      std::ofstream description(root / "APMod" / "description.txt", std::ios::binary);
      description << "Original action-point mod";
      description.close();
      if (!result && !CModManager::Activate(std::vector<std::string>{"apmod"})) result = 21;
      const std::map<int, int> applied = result ? std::map<int, int>() : ActionPoints();
      if (!result && applied.size() != base.size()) result = 22;
      bool changed = false;
      for (const auto& item : base) {
        const auto override = expected.find(item.first);
        const int wanted = override == expected.end() ? item.second : override->second;
        if (!result && applied.at(item.first) != wanted) result = 23;
        if (override != expected.end() && item.second != wanted) changed = true;
      }
      if (!result && (!changed || StartZoneId() != zones.first ||
          ReadGlobalMarker() != "BASE")) result = 24;
      if (!result && !CModManager::Activate(std::vector<SModInfo>{})) result = 25;
      if (!result && ActionPoints() != base) result = 26;
    }
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
