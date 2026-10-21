#include "NetworkIdentity.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifndef S2_NETWORK_BUILD_ID
#define S2_NETWORK_BUILD_ID "s2-network-v1"
#endif
namespace S2Net {
Compatibility IdentifyGameData(const std::vector<std::string>&,const std::vector<std::string>& databases,const std::atomic<bool>& cancelled) {
  Compatibility result{S2_NETWORK_BUILD_ID,""};
  if(databases.empty())throw std::runtime_error("Game data is unavailable");
  for(auto database:databases) {
    if(cancelled.load())throw std::runtime_error("Cancelled");
    std::replace(database.begin(),database.end(),'\\','/');
    auto path=std::filesystem::path(database).parent_path()/"network-version.txt";
    std::ifstream input(path,std::ios::binary);
    std::string magic,build,data,extra;
    auto digest=[](const std::string& value){return value.size()==64 && value.find_first_not_of("0123456789abcdef")==std::string::npos;};
    if(!(input>>magic>>build>>data) || (input>>extra) || magic!="S2-NETWORK-1" || !digest(build) || !digest(data))
      throw std::runtime_error("Missing or invalid network-version.txt; rebuild/package the game data");
    if(build!=result.build)throw std::runtime_error("Game executable and network-version.txt are from different builds");
    // Ordered mod manifests follow the database overlay order. No resource
    // archives are opened during connection or when recreating a match.
    result.dataFingerprint+=data;
  }
  return result;
}
}
