#include "../FileIO/PortableGameLinks.h"

#include <cstdint>
#include <vector>

int main() {
  S2FileIO::PortableGameDatabase database;
  database.tables.resize(2);
  auto& animations = database.tables[0];
  animations.tableId = 2;
  animations.intNames = {"ID", "SkeletonID"};
  animations.intRows = {{12, 7}, {10, 0}, {11, 9}, {13, 7}};
  auto& skeletons = database.tables[1];
  skeletons.tableId = 13;
  skeletons.intNames = {"ID"};
  skeletons.intRows = {{7}};
  std::vector<S2FileIO::GameDatabaseLink> links;
  std::size_t unresolved = 0;
  if (!S2FileIO::CollectAnimationSkeletonLinks(database, &links, &unresolved) ||
      links.size() != 3 || unresolved != 1 ||
      links[0] != S2FileIO::GameDatabaseLink(12, 7) ||
      links[1] != S2FileIO::GameDatabaseLink(11, 9) ||
      links[2] != S2FileIO::GameDatabaseLink(13, 7)) return 1;
  auto reordered = links;
  reordered.push_back(reordered.front());
  reordered.erase(reordered.begin());
  if (S2FileIO::HashGameDatabaseLinks(links) !=
      S2FileIO::HashGameDatabaseLinks(reordered)) return 1;
  if (S2FileIO::HashGameDatabaseLinksInOrder(links) ==
      S2FileIO::HashGameDatabaseLinksInOrder(reordered)) return 1;
  animations.intRows.push_back({12, 7});
  if (S2FileIO::CollectAnimationSkeletonLinks(database, &links, &unresolved)) return 1;
  return 0;
}
