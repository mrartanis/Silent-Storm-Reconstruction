#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

namespace {
bool FindChild(const std::uint8_t* bytes, std::size_t size,
               const S2FileIO::StructureChunk& parent, std::uint8_t id,
               S2FileIO::StructureChunk* found) {
  const std::size_t end = static_cast<std::size_t>(parent.payloadOffset) + parent.length;
  if (!found || end > size) return false;
  for (std::size_t at = static_cast<std::size_t>(parent.payloadOffset); at < end;) {
    S2FileIO::StructureChunk child;
    if (!S2FileIO::DecodeStructureChunkAt(bytes, end, at, &child)) return false;
    if (child.id == id) { *found = child; return true; }
    at = static_cast<std::size_t>(child.payloadOffset) + child.length;
  }
  return false;
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: ProbeSavedExplosionPerks <game.sav>\n";
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
  constexpr std::size_t header = 8 + 320 * 200 * 4;
  if (bytes.size() <= header || bytes[0] != 0x22 || bytes[1] != 0xa0 ||
      bytes[2] != 0x8c || bytes[3] != 0x82 || bytes[4] || bytes[5] ||
      bytes[6] || bytes[7]) {
    std::cerr << "unsupported save header or active mods\n";
    return 1;
  }
  const std::uint8_t* data = bytes.data() + header;
  const std::size_t size = bytes.size() - header;
  S2FileIO::StructureChunk table, bodies;
  bool haveTable = false, haveBodies = false;
  for (std::size_t at = 0; at < size;) {
    S2FileIO::StructureChunk chunk;
    if (!S2FileIO::DecodeStructureChunkAt(data, size, at, &chunk)) return 1;
    if (chunk.id == 3) { std::cerr << "packed save not supported\n"; return 1; }
    if (chunk.id == 0) { table = chunk; haveTable = true; }
    if (chunk.id == 2) { bodies = chunk; haveBodies = true; }
    at = static_cast<std::size_t>(chunk.payloadOffset) + chunk.length;
  }
  if (!haveTable || !haveBodies) return 1;
  std::vector<S2FileIO::StructureObjectRecord> records;
  std::vector<S2FileIO::StructureObjectBody> objectBodies;
  if (!S2FileIO::DecodeStructureObjectTable(data + table.payloadOffset, table.length, &records) ||
      !S2FileIO::IndexStructureObjectBodies(data + bodies.payloadOffset, bodies.length, &objectBodies))
    return 1;
  std::map<std::uint32_t, std::uint32_t> types;
  for (const auto& record : records) types[record.wireId] = record.typeId;
  std::size_t count = 0;
  const std::uint8_t* payload = data + bodies.payloadOffset;
  for (const auto& object : objectBodies) {
    const auto type = types.find(object.wireId);
    if (type == types.end() || type->second != 0x52782130) continue;
    S2FileIO::StructureChunk objectChunk{1, object.bodyOffset, object.bodyLength};
    S2FileIO::StructureChunk modifier;
    if (!FindChild(payload, bodies.length, objectChunk, 11, &modifier) ||
        modifier.length != 12) {
      std::cerr << "tracker modifier tag missing or wrong size\n";
      return 1;
    }
    S2FileIO::StructureExplosionPerkFields value;
    if (!S2FileIO::DecodeStructureExplosionPerks(payload + modifier.payloadOffset,
                                                  modifier.length, &value)) return 1;
    std::cout << "tracker wire=" << object.wireId << " structure="
              << std::setprecision(9) << value.structureDamage << " area="
              << value.areaDamage << " critical=" << value.alwaysHumanCritical
              << " padding=" << std::hex << unsigned(payload[modifier.payloadOffset + 9])
              << "," << unsigned(payload[modifier.payloadOffset + 10]) << ","
              << unsigned(payload[modifier.payloadOffset + 11]) << std::dec << "\n";
    ++count;
  }
  std::cout << "trackers=" << count << "\n";
  return count ? 0 : 1;
}
