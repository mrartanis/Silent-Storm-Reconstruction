#pragma once

#include "PortableGameDatabase.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace S2FileIO {

using GameDatabaseLink = std::pair<std::int32_t, std::int32_t>;

// Game-used Animation.SkeletonID references (table 2 -> table 13).
// Null/non-positive references are omitted; positive missing targets are
// included in links and counted as unresolved for parity diagnostics.
bool CollectAnimationSkeletonLinks(const PortableGameDatabase& database,
                                   std::vector<GameDatabaseLink>* links,
                                   std::size_t* unresolved);
std::uint64_t HashGameDatabaseLinks(std::vector<GameDatabaseLink> links);
// Diagnostic only: unlike HashGameDatabaseLinks, preserves traversal order.
std::uint64_t HashGameDatabaseLinksInOrder(const std::vector<GameDatabaseLink>& links);
// Emulates the original STLport hash_map<int,...> iteration after row-order
// insertion. The input must contain every record, including unlinked ones.
std::vector<std::int32_t> OrderGameDatabaseRecordIdsLikeStlport(
    const std::vector<std::int32_t>& rowIds);
std::vector<GameDatabaseLink> OrderGameDatabaseLinksLikeStlport(
    const std::vector<GameDatabaseLink>& rowOrder);
// Grouping by the game's (SkeletonID, Type) columns; each inner vector keeps
// the supplied link traversal order. IDs uniquely identify groups for hashing.
bool CollectAnimationSkeletonGroups(const PortableGameDatabase& database,
                                    const std::vector<GameDatabaseLink>& orderedLinks,
                                    std::vector<std::vector<std::int32_t>>* groups);
std::uint64_t HashAnimationGroupIds(std::vector<std::vector<std::int32_t>> groups);

} // namespace S2FileIO
