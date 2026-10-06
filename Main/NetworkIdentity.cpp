#include "NetworkIdentity.h"
#include "../FileIO/PortablePackageIndex.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

#ifndef S2_NETWORK_BUILD_ID
#define S2_NETWORK_BUILD_ID "s2-network-v1"
#endif
namespace S2Net {
namespace {
struct Fingerprint {
  std::uint64_t value=14695981039346656037ull;
  void Bytes(const void* raw,std::size_t count) {
    auto* bytes=static_cast<const unsigned char*>(raw);
    for(std::size_t i=0;i<count;++i){value^=bytes[i];value*=1099511628211ull;}
  }
  void Text(const std::string& s) {Bytes(s.data(),s.size());unsigned char zero=0;Bytes(&zero,1);}
};
void Check(const std::atomic<bool>& cancelled) {if(cancelled.load())throw std::runtime_error("Cancelled");}
std::string Normalize(std::string path){std::replace(path.begin(),path.end(),'\\','/');return path;}
std::string Lower(std::string s){for(auto& c:s)if(c>='A' && c<='Z')c+=32;return s;}
void File(Fingerprint& hash,const std::filesystem::path& file,const std::atomic<bool>& cancelled) {
  std::ifstream input(file,std::ios::binary);if(!input)throw std::runtime_error("Cannot read game data: "+file.string());
  char block[65536];
  while(input){Check(cancelled);input.read(block,sizeof(block));hash.Bytes(block,static_cast<std::size_t>(input.gcount()));}
  if(!input.eof())throw std::runtime_error("Error reading game data: "+file.string());
}
}
Compatibility IdentifyGameData(const std::vector<std::string>& directories,const std::vector<std::string>& databases,const std::atomic<bool>& cancelled) {
  Fingerprint hash;
  for(const auto& db:databases){Check(cancelled);hash.Text("database");File(hash,Normalize(db),cancelled);}
  std::map<std::string,std::vector<S2FileIO::PortablePackageIndex>> packages;
  std::map<std::string,std::filesystem::path> loose;
  // Common tactical rules also live in loose Lua files beside each database.
  // Treat later overlays in the same order as the resource/database loader.
  for(const auto& database:databases) {
    auto scripts=std::filesystem::path(Normalize(database)).parent_path()/"scripts";
    if(!std::filesystem::exists(scripts))continue;
    for(const auto& file:std::filesystem::recursive_directory_iterator(scripts)) {
      Check(cancelled);if(file.is_regular_file())loose["scripts/"+Lower(file.path().lexically_relative(scripts).generic_string())]=file.path();
    }
  }
  for(const auto& raw:directories) {
    auto dir=std::filesystem::path(Normalize(raw));
    if(!std::filesystem::exists(dir))throw std::runtime_error("Resource directory unavailable");
    for(const auto& file:std::filesystem::recursive_directory_iterator(dir)) {
      Check(cancelled);if(!file.is_regular_file())continue;
      auto name=Lower(file.path().lexically_relative(dir).generic_string());
      if(Lower(file.path().extension().string())==".res") {
        S2FileIO::PortablePackageIndex index;std::string error;
        if(!index.Open(file.path().string(),&error))throw std::runtime_error(error);
        packages[name].push_back(std::move(index));
      }else loose[name]=file.path();
    }
  }
  // The last registered package overrides individual records, just as the
  // engine's resource loader does. Host paths and filenames' case are ignored.
  for(const auto& package:packages) {
    hash.Text(package.first);
    std::map<std::int32_t,const S2FileIO::PortablePackageIndex*> effective;
    for(const auto& index:package.second)for(const auto& record:index.Entries())effective[record.first]=&index;
    for(const auto& record:effective) {
      Check(cancelled);S2Net::Bytes id;Put32(id,static_cast<std::uint32_t>(record.first));hash.Bytes(id.data(),id.size());
      S2Net::Bytes data;std::string error;
      if(!record.second->Read(record.first,&data,&error))throw std::runtime_error(error);
      hash.Bytes(data.data(),data.size());
    }
  }
  for(const auto& file:loose){hash.Text(file.first);File(hash,file.second,cancelled);}
  std::ostringstream text;text<<std::hex<<std::setw(16)<<std::setfill('0')<<hash.value;
  return {S2_NETWORK_BUILD_ID,text.str()};
}
}
