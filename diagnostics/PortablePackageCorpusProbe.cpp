#include "../FileIO/PortablePackageIndex.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {
std::uint64_t AddByte(std::uint64_t hash, std::uint8_t value) {
  return (hash ^ value) * UINT64_C(1099511628211);
}

std::uint64_t AddU32(std::uint64_t hash, std::uint32_t value) {
  for (unsigned shift = 0; shift < 32; shift += 8)
    hash = AddByte(hash, static_cast<std::uint8_t>(value >> shift));
  return hash;
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: PortablePackageCorpusProbe <res-directory>\n");
    return 2;
  }
  const std::filesystem::path root(argv[1]);
  std::error_code error;
  if (!std::filesystem::is_directory(root, error) || error) return 3;
  std::vector<std::filesystem::path> archives;
  for (std::filesystem::directory_iterator it(root, error), end;
       !error && it != end; it.increment(error)) {
    if (it->is_regular_file(error) && it->path().extension() == ".res")
      archives.push_back(it->path());
    if (error) break;
  }
  if (error || archives.empty()) return 4;
  std::sort(archives.begin(), archives.end());
  std::uint64_t totalResources = 0;
  std::uint64_t totalBytes = 0;
  for (const auto& archive : archives) {
    S2FileIO::PortablePackageIndex package;
    std::string why;
    if (!package.Open(archive.string(), &why)) {
      std::fprintf(stderr, "%s: %s\n", archive.string().c_str(), why.c_str());
      return 5;
    }
    std::uint64_t hash = UINT64_C(14695981039346656037);
    std::uint64_t bytes = 0;
    for (const auto& entry : package.Entries()) {
      std::vector<std::uint8_t> data;
      if (!package.Read(entry.first, &data, &why)) {
        std::fprintf(stderr, "%s id %d: %s\n", archive.string().c_str(),
                     entry.first, why.c_str());
        return 6;
      }
      hash = AddU32(hash, static_cast<std::uint32_t>(entry.first));
      hash = AddU32(hash, static_cast<std::uint32_t>(data.size()));
      for (const auto value : data) hash = AddByte(hash, value);
      bytes += data.size();
    }
    totalResources += package.Entries().size();
    totalBytes += bytes;
    std::printf("%s entries=%zu bytes=%llu fnv64=%016llx\n",
                archive.filename().string().c_str(), package.Entries().size(),
                static_cast<unsigned long long>(bytes),
                static_cast<unsigned long long>(hash));
  }
  std::printf("total archives=%zu entries=%llu bytes=%llu\n", archives.size(),
              static_cast<unsigned long long>(totalResources),
              static_cast<unsigned long long>(totalBytes));
  return 0;
}
