#include "PortableGameLinks.h"

#include <algorithm>
#include <map>
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

std::vector<GameDatabaseLink> OrderGameDatabaseLinksLikeStlport(
    const std::vector<GameDatabaseLink>& rowOrder) {
  // Jan03 STLport: hash_map() requests 100 buckets, rounded up to prime 193;
  // operator[] prepends, and resize() moves old chains front-to-back while
  // prepending each node into the new bucket.
  constexpr std::size_t primes[] = {
      193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157,
      98317, 196613, 393241, 786433, 1572869, 3145739,
      6291469, 12582917, 25165843, 50331653, 100663319,
      201326611, 402653189, 805306457, 1610612741,
      3221225473ULL, 4294967291ULL};
  std::size_t primeIndex = 0;
  std::vector<std::vector<GameDatabaseLink>> buckets(primes[primeIndex]);
  std::size_t count = 0;
  for (const auto& link : rowOrder) {
    if (count + 1 > buckets.size() && primeIndex + 1 < sizeof(primes) / sizeof(primes[0])) {
      std::vector<std::vector<GameDatabaseLink>> next(primes[++primeIndex]);
      for (const auto& chain : buckets)
        for (const auto& previous : chain) {
          auto& target = next[static_cast<std::uint32_t>(previous.first) % next.size()];
          target.insert(target.begin(), previous);
        }
      buckets.swap(next);
    }
    auto& chain = buckets[static_cast<std::uint32_t>(link.first) % buckets.size()];
    chain.insert(chain.begin(), link);
    ++count;
  }
  std::vector<GameDatabaseLink> result;
  result.reserve(rowOrder.size());
  for (const auto& chain : buckets)
    result.insert(result.end(), chain.begin(), chain.end());
  return result;
}

bool CollectAnimationSkeletonGroups(const PortableGameDatabase& database,
                                    const std::vector<GameDatabaseLink>& orderedLinks,
                                    std::vector<std::vector<std::int32_t>>* groups) {
  if (!groups) return false;
  groups->clear();
  const auto* animations = FindTable(database, 2);
  if (!animations) return false;
  const std::size_t idColumn = FindColumn(animations->intNames, "ID");
  const std::size_t typeColumn = FindColumn(animations->stringNames, "Type");
  if (idColumn == animations->intNames.size() ||
      typeColumn == animations->stringNames.size() ||
      animations->intRows.size() != animations->stringRows.size()) return false;
  std::map<std::int32_t, std::wstring> types;
  for (std::size_t i = 0; i < animations->intRows.size(); ++i) {
    if (animations->intRows[i].size() != animations->intNames.size() ||
        animations->stringRows[i].size() != animations->stringNames.size() ||
        !types.emplace(animations->intRows[i][idColumn],
                       animations->stringRows[i][typeColumn]).second) return false;
  }
  std::map<std::pair<std::int32_t, std::wstring>, std::vector<std::int32_t>> byKey;
  for (const auto& link : orderedLinks) {
    const auto type = types.find(link.first);
    if (type == types.end()) return false;
    byKey[{link.second, type->second}].push_back(link.first);
  }
  for (const auto& entry : byKey) groups->push_back(entry.second);
  return true;
}

std::uint64_t HashAnimationGroupIds(std::vector<std::vector<std::int32_t>> groups) {
  std::sort(groups.begin(), groups.end(), [](const auto& left, const auto& right) {
    const auto leftMin = left.empty() ? INT32_MAX : *std::min_element(left.begin(), left.end());
    const auto rightMin = right.empty() ? INT32_MAX : *std::min_element(right.begin(), right.end());
    return leftMin < rightMin;
  });
  std::uint64_t hash = UINT64_C(14695981039346656037);
  const auto u32 = [&hash](std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      hash = (hash ^ static_cast<std::uint8_t>(value >> (8 * i))) * UINT64_C(1099511628211);
  };
  u32(static_cast<std::uint32_t>(groups.size()));
  for (const auto& group : groups) {
    u32(static_cast<std::uint32_t>(group.size()));
    for (std::int32_t id : group) u32(static_cast<std::uint32_t>(id));
  }
  return hash;
}

} // namespace S2FileIO
