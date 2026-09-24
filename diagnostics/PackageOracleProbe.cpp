#include "../FileIO/StdAfx.h"
#include "../FileIO/FilesPackage.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv) {
  if (argc < 3) return 2;
  for (int i = 2; i < argc; ++i) {
    // CPackageStream owns the package through CPtr and releases it on exit.
    // Reopen for each ID instead of retaining a dangling raw pointer.
    IFilesPackage* package = OpenFilesPackage(argv[1]);
    if (!package) return 3;
    const int id = std::atoi(argv[i]);
    if (!DoesFileExist(package, id)) return 4;
    CPackageStream stream(package, id);
    const unsigned int length = stream.GetSize();
    std::vector<std::uint8_t> bytes(length);
    if (length) stream.Read(bytes.data(), length);
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (std::uint8_t value : bytes) {
      hash ^= value;
      hash *= UINT64_C(1099511628211);
    }
    std::printf("%d %u %016llx\n", id, length,
                static_cast<unsigned long long>(hash));
  }
  return 0;
}
