#include "../FileIO/PortableSaveHeader.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) return 3;
  std::vector<std::uint8_t> wire(S2FileIO::kSaveHeaderWireSize);
  file.read(reinterpret_cast<char*>(wire.data()), wire.size());
  if (!file) return 4;
  S2FileIO::SaveHeaderData header;
  if (!S2FileIO::DecodeSaveHeader(wire.data(), wire.size(), &header)) return 5;
  std::uint64_t hash = 14695981039346656037ull;
  for (const auto& pixel : header.screenshot)
    for (std::uint8_t channel : {pixel.red, pixel.green, pixel.blue, pixel.alpha}) {
      hash ^= channel;
      hash *= 1099511628211ull;
    }
  std::cout << "magic=" << std::hex << static_cast<std::uint32_t>(header.magic)
            << std::dec << " mods=" << header.activeMods
            << " pixels=" << header.screenshot.size()
            << " fnv64=" << std::hex << std::setfill('0') << std::setw(16)
            << hash << '\n';
  return 0;
}
