#include "../FileIO/PortablePackageIndex.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;

void U32(Bytes* bytes, std::uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8)
    bytes->push_back(static_cast<std::uint8_t>(value >> shift));
}

Bytes Chunk(std::uint8_t id, const Bytes& payload) {
  Bytes bytes{id, static_cast<std::uint8_t>(payload.size() * 2)};
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  return bytes;
}

Bytes Package(bool duplicate = false, bool invalidSpan = false) {
  Bytes map;
  auto addEntry = [&] {
    Bytes key;
    U32(&key, 17);
    Bytes value;
    U32(&value, 8);
    U32(&value, invalidSpan ? 4 : 3);
    const Bytes keyChunk = Chunk(1, key);
    const Bytes valueChunk = Chunk(2, value);
    map.insert(map.end(), keyChunk.begin(), keyChunk.end());
    map.insert(map.end(), valueChunk.begin(), valueChunk.end());
  };
  addEntry();
  if (duplicate) addEntry();
  const Bytes mapChunk = Chunk(1, map);
  const Bytes mainChunk = Chunk(1, mapChunk);
  Bytes file;
  U32(&file, 0x96948A22);
  U32(&file, 11);
  file.insert(file.end(), {0x12, 0x34, 0x56});
  file.insert(file.end(), mainChunk.begin(), mainChunk.end());
  file.insert(file.end(), {0, 0}); // retail indexes can have trailing padding
  return file;
}

bool Check(const std::filesystem::path& path, const Bytes& bytes, bool valid) {
  {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()))
      return false;
  }
  S2FileIO::PortablePackageIndex index;
  std::string error;
  if (index.Open(path.string(), &error) != valid) {
    std::cerr << "unexpected Open result: " << error << '\n';
    return false;
  }
  if (!valid) return true;
  std::vector<std::uint8_t> resource;
  return index.IndexOffset() == 11 && index.Entries().size() == 1 &&
         index.Read(17, &resource, &error) &&
         resource == Bytes({0x12, 0x34, 0x56}) &&
         !index.Read(18, &resource, &error);
}
} // namespace

int main() {
  const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto path = std::filesystem::temp_directory_path() /
      ("s2-package-index-test-" + std::to_string(suffix) + ".res");
  const bool good = Check(path, Package(), true);
  Bytes badSignature = Package();
  badSignature[0] = 0;
  const bool signature = Check(path, badSignature, false);
  const bool span = Check(path, Package(false, true), false);
  const bool duplicate = Check(path, Package(true), false);
  bool casePath = true;
#if !defined(_WIN32)
  const auto caseDir = path.parent_path() / (path.stem().string() + "-MiXeD");
  const auto actual = caseDir / "Fonts.res";
  std::error_code caseError;
  casePath = std::filesystem::create_directory(caseDir, caseError) &&
             Check(actual, Package(), true);
  if (casePath) {
    std::string requested = (path.parent_path() /
        (path.stem().string() + "-mixed") / "fONTS.RES").string();
    std::replace(requested.begin(), requested.end(), '/', '\\');
    S2FileIO::PortablePackageIndex index;
    Bytes bytes;
    casePath = index.Open(requested) && index.ResolvedPath() == actual.string() &&
               index.Read(17, &bytes) && bytes == Bytes({0x12, 0x34, 0x56});
    const auto conflict = caseDir / "fonts.res";
    if (casePath && Check(conflict, Package(), true)) {
      S2FileIO::PortablePackageIndex ambiguous;
      casePath = !ambiguous.Open(requested);
    } else casePath = false;
    std::filesystem::remove(conflict, caseError);
  }
  std::filesystem::remove(actual, caseError);
  std::filesystem::remove(caseDir, caseError);
  casePath = casePath && !caseError;
#endif
  std::error_code cleanupError;
  std::filesystem::remove(path, cleanupError);
  return good && signature && span && duplicate && casePath && !cleanupError ? 0 : 1;
}
