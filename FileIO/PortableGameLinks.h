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

} // namespace S2FileIO
