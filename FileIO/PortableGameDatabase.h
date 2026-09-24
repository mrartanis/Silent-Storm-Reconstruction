#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace S2FileIO {

struct GameDatabaseColumn {
  std::string name;
  std::int32_t type = 0;
};

struct GameDatabaseTable {
  std::int32_t tableId = 0;
  std::uint32_t wireId = 0;
  std::vector<GameDatabaseColumn> columns;
  std::vector<std::string> intNames, floatNames, stringNames;
  std::vector<std::vector<std::int32_t>> intRows;
  std::vector<std::vector<float>> floatRows;
  std::vector<std::vector<std::wstring>> stringRows;
};

struct GameDatabaseRelation {
  std::string name;
  std::int32_t leftTableId = 0, rightTableId = 0;
  std::vector<std::pair<std::int32_t, std::int32_t>> links;
};

struct PortableGameDatabase {
  std::vector<GameDatabaseTable> tables;
  std::vector<GameDatabaseRelation> relations;
};

// Release v1 game.db columnar storage only. Does not instantiate game objects
// or implement the editor's ADO import path.
bool LoadPortableGameDatabase(const std::string& path, PortableGameDatabase* database,
                              std::string* error = nullptr);
bool LoadPortableGameDatabaseBytes(const std::uint8_t* bytes, std::size_t length,
                                   PortableGameDatabase* database,
                                   std::string* error = nullptr);
// Canonical value digest for comparing this decoder with the game's runtime
// CDBTableDataStorage. Excludes transient wire IDs and host representations.
bool HashGameDatabaseTable(const GameDatabaseTable& table, std::uint64_t* hash);
bool HashGameDatabaseRelations(const std::vector<GameDatabaseRelation>& relations,
                               std::uint64_t* hash);

} // namespace S2FileIO
