#include "PortableGameLinks.h"

#include <algorithm>
#include <set>
#include <string>

namespace S2FileIO {
namespace {
const GameDatabaseTable* FindTable(const PortableGameDatabase& database, std::int32_t id) {
  for (const auto& table : database.tables)
    if (table.tableId == id) return &table;
  return nullptr;
}

std::size_t FindColumn(const std::vector<std::string>& names, const char* name) {
  const auto found = std::find(names.begin(), names.end(), name);
  return found == names.end() ? names.size() : static_cast<std::size_t>(found - names.begin());
}
}

bool CollectAnimationSkeletonLinks(const PortableGameDatabase& database,
                                   std::vector<GameDatabaseLink>* links,
                                   std::size_t* unresolved) {
  if (!links || !unresolved) return false;
  links->clear();
  *unresolved = 0;
  const auto* animations = FindTable(database, 2);
  const auto* skeletons = FindTable(database, 13);
  if (!animations || !skeletons) return false;
  const std::size_t animationId = FindColumn(animations->intNames, "ID");
  const std::size_t skeletonId = FindColumn(animations->intNames, "SkeletonID");
  const std::size_t targetId = FindColumn(skeletons->intNames, "ID");
  if (animationId == animations->intNames.size() ||
      skeletonId == animations->intNames.size() ||
      targetId == skeletons->intNames.size()) return false;
  std::set<std::int32_t> targetIds;
  for (const auto& row : skeletons->intRows) {
    if (row.size() != skeletons->intNames.size()) return false;
    if (!targetIds.insert(row[targetId]).second) return false;
  }
  std::set<std::int32_t> sourceIds;
  for (const auto& row : animations->intRows) {
    if (row.size() != animations->intNames.size()) return false;
    if (!sourceIds.insert(row[animationId]).second) return false;
    if (row[skeletonId] <= 0) continue;
    links->emplace_back(row[animationId], row[skeletonId]);
    *unresolved += targetIds.count(row[skeletonId]) == 0;
  }
  return true;
}

std::uint64_t HashGameDatabaseLinksInOrder(const std::vector<GameDatabaseLink>& links) {
  std::uint64_t hash = UINT64_C(14695981039346656037);
  const auto u32 = [&hash](std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      hash = (hash ^ static_cast<std::uint8_t>(value >> (8 * i))) * UINT64_C(1099511628211);
  };
  u32(static_cast<std::uint32_t>(links.size()));
  for (const auto& link : links) {
    u32(static_cast<std::uint32_t>(link.first));
    u32(static_cast<std::uint32_t>(link.second));
  }
  return hash;
}

std::uint64_t HashGameDatabaseLinks(std::vector<GameDatabaseLink> links) {
  std::sort(links.begin(), links.end());
  return HashGameDatabaseLinksInOrder(links);
}

} // namespace S2FileIO
