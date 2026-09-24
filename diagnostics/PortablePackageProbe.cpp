#include "../FileIO/PortablePackageIndex.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  S2FileIO::PortablePackageIndex package;
  std::string error;
  if (!package.Open(argv[1], &error)) {
    std::fprintf(stderr, "package open failed: %s\n", error.c_str());
    return 3;
  }
  std::printf("size=%llu index=%u entries=%zu\n",
      static_cast<unsigned long long>(package.FileSize()),
      package.IndexOffset(), package.Entries().size());
  if (argc == 3 && std::string(argv[2]) == "--list") {
    for (const auto& item : package.Entries())
      std::printf("%d %u %u\n", item.first, item.second.offset,
                  item.second.length);
    return 0;
  }
  for (int i = 2; i < argc; ++i) {
    const int id = std::atoi(argv[i]);
    std::vector<std::uint8_t> bytes;
    if (!package.Read(id, &bytes, &error)) {
      std::fprintf(stderr, "resource %d: %s\n", id, error.c_str());
      return 4;
    }
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (std::uint8_t value : bytes) {
      hash ^= value;
      hash *= UINT64_C(1099511628211);
    }
    std::printf("%d %zu %016llx\n", id, bytes.size(),
        static_cast<unsigned long long>(hash));
  }
  return 0;
}
