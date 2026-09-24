#include "../FileIO/PortableGameDatabase.h"
#include "../FileIO/PortableGameLinks.h"
#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
struct Hash {
  std::uint64_t value = UINT64_C(14695981039346656037);
  void Byte(std::uint8_t b) { value = (value ^ b) * UINT64_C(1099511628211); }
  void U32(std::uint32_t number) {
    for (unsigned i = 0; i != 4; ++i) Byte(static_cast<std::uint8_t>(number >> (8 * i)));
  }
  void String(const std::string& str) {
    U32(static_cast<std::uint32_t>(str.size()));
    for (unsigned char c : str) Byte(c);
  }
};
}

int main(int argc, char** argv) {
  if (argc != 2 && !(argc == 3 &&
      (std::string(argv[2]) == "--all" || std::string(argv[2]) == "--links"))) return 2;
  const bool showAll = argc == 3 && std::string(argv[2]) == "--all";
  const bool showLinks = argc == 3 && std::string(argv[2]) == "--links";
  S2FileIO::PortableGameDatabase database;
  std::string error;
  if (!S2FileIO::LoadPortableGameDatabase(argv[1], &database, &error)) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 3;
  }
  std::size_t intCells = 0, floatCells = 0, stringCells = 0, relationLinks = 0;
  std::size_t shapeErrors = 0;
  Hash hash;
  for (const auto& table : database.tables) {
    const std::size_t rowCount = table.intRows.size();
    if ((!table.floatNames.empty() && table.floatRows.size() != rowCount) ||
        (!table.stringNames.empty() && table.stringRows.size() != rowCount) ||
        table.columns.size() != table.intNames.size() + table.floatNames.size() +
                                table.stringNames.size())
      ++shapeErrors;
    for (const auto& row : table.intRows) if (row.size() != table.intNames.size()) ++shapeErrors;
    for (const auto& row : table.floatRows) if (row.size() != table.floatNames.size()) ++shapeErrors;
    for (const auto& row : table.stringRows) if (row.size() != table.stringNames.size()) ++shapeErrors;
    hash.U32(static_cast<std::uint32_t>(table.tableId));
    hash.U32(table.wireId);
    for (const auto& column : table.columns) {
      hash.String(column.name);
      hash.U32(static_cast<std::uint32_t>(column.type));
    }
    for (const auto& name : table.intNames) hash.String(name);
    for (const auto& name : table.floatNames) hash.String(name);
    for (const auto& name : table.stringNames) hash.String(name);
    for (const auto& row : table.intRows)
      for (std::int32_t value : row) { hash.U32(static_cast<std::uint32_t>(value)); ++intCells; }
    for (const auto& row : table.floatRows)
      for (float value : row) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, 4);
        hash.U32(bits);
        ++floatCells;
      }
    for (const auto& row : table.stringRows)
      for (const auto& value : row) {
        std::vector<std::uint8_t> wire;
        if (!S2FileIO::EncodeStructureUtf16(value, &wire)) return 4;
        hash.U32(static_cast<std::uint32_t>(wire.size()));
        for (std::uint8_t b : wire) hash.Byte(b);
        ++stringCells;
      }
  }
  for (const auto& relation : database.relations) {
    hash.String(relation.name);
    hash.U32(static_cast<std::uint32_t>(relation.leftTableId));
    hash.U32(static_cast<std::uint32_t>(relation.rightTableId));
    for (const auto& link : relation.links) {
      hash.U32(static_cast<std::uint32_t>(link.first));
      hash.U32(static_cast<std::uint32_t>(link.second));
      ++relationLinks;
    }
  }
  std::uint64_t relationsHash = 0;
  if (!S2FileIO::HashGameDatabaseRelations(database.relations, &relationsHash))
    return 7;
  std::printf("tables %zu relations %zu int-cells %zu float-cells %zu string-cells %zu relation-links %zu shape-errors %zu hash %016llx\n",
      database.tables.size(), database.relations.size(), intCells, floatCells,
      stringCells, relationLinks, shapeErrors, static_cast<unsigned long long>(hash.value));
  std::printf("relations-hash %016llx\n",
      static_cast<unsigned long long>(relationsHash));
  if (showLinks) {
    std::vector<S2FileIO::GameDatabaseLink> links;
    std::size_t unresolved = 0;
    if (!S2FileIO::CollectAnimationSkeletonLinks(database, &links, &unresolved)) return 8;
    std::printf("animation-skeleton-links %zu unresolved %zu hash %016llx order-hash %016llx\n",
        links.size(), unresolved,
        static_cast<unsigned long long>(S2FileIO::HashGameDatabaseLinks(links)),
        static_cast<unsigned long long>(S2FileIO::HashGameDatabaseLinksInOrder(links)));
  }
  for (std::size_t i = 0; i < database.tables.size() && (showAll || i < 5); ++i) {
    const auto& table = database.tables[i];
    std::uint64_t tableHash = 0;
    if (!S2FileIO::HashGameDatabaseTable(table, &tableHash)) return 6;
    std::printf("table %08x rows %zu int %zu float %zu string %zu columns %zu hash %016llx\n",
        static_cast<std::uint32_t>(table.tableId), table.intRows.size(),
        table.intNames.size(), table.floatNames.size(), table.stringNames.size(),
        table.columns.size(), static_cast<unsigned long long>(tableHash));
  }
  return shapeErrors ? 5 : 0;
}
