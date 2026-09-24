#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  std::vector<S2FileIO::StructureChunk> chunks;
  std::string error;
  if (!S2FileIO::ScanStructureFile(argv[1], &chunks, &error)) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 3;
  }
  std::ifstream file(argv[1], std::ios::binary);
  std::vector<char> buffer(65536);
  for (const auto& chunk : chunks) {
    file.seekg(static_cast<std::streamoff>(chunk.payloadOffset));
    std::uint32_t remaining = chunk.length;
    std::uint64_t hash = UINT64_C(14695981039346656037);
    while (remaining) {
      const std::size_t count = remaining < buffer.size() ? remaining : buffer.size();
      if (!file.read(buffer.data(), count)) return 4;
      for (std::size_t i = 0; i < count; ++i) {
        hash ^= static_cast<std::uint8_t>(buffer[i]);
        hash *= UINT64_C(1099511628211);
      }
      remaining -= static_cast<std::uint32_t>(count);
    }
    std::printf("%u %llu %u %016llx\n", unsigned(chunk.id),
        static_cast<unsigned long long>(chunk.payloadOffset), chunk.length,
        static_cast<unsigned long long>(hash));
  }
  return 0;
}
