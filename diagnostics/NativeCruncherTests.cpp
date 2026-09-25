#include "../FileIO/StdAfx.h"
#include "../FileIO/Cruncher.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static bool Check(const std::vector<std::uint8_t>& bytes, bool stored) {
  CMemoryStream source, packed, restored;
  source.Write(bytes.data(), static_cast<unsigned>(bytes.size()));
  CNetCompressor encoder;
  if (stored) encoder.StorePack(source, packed);
  else encoder.Pack(source, packed);
  packed.Seek(0);
  CNetCompressor decoder;
  decoder.Unpack(packed, restored);
  if (restored.GetSize() != static_cast<int>(bytes.size())) return false;
  return std::memcmp(restored.GetBuffer(), bytes.data(), bytes.size()) == 0;
}

int main(int argc, char** argv) {
  try {
    std::vector<std::uint8_t> sample(4096);
    for (std::size_t i = 0; i < sample.size(); ++i)
      sample[i] = static_cast<std::uint8_t>((i % 71 < 48) ? i % 11 : i * 37);
    if (!Check(sample, false) || !Check(sample, true)) return 1;
    if (argc == 2) {
      std::ifstream input(argv[1], std::ios::binary);
      if (!input) return 2;
      const std::vector<std::uint8_t> original(
          (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
      if (original.empty() || !Check(original, false) || !Check(original, true)) return 3;
      std::printf("native-cruncher original-data bytes %zu\n", original.size());
    } else if (argc != 1) {
      return 4;
    }
    return 0;
  } catch (const SFileIOError& error) {
    std::fprintf(stderr, "NativeCruncherTests: %s\n", error.szError.c_str());
  }
  return 5;
}
