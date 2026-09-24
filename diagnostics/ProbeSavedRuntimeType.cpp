#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace {
bool FindChild(const std::uint8_t* bytes, std::size_t size,
               std::uint8_t id, S2FileIO::StructureChunk* found) {
  for (std::size_t at = 0; at < size;) {
    S2FileIO::StructureChunk child;
    if (!S2FileIO::DecodeStructureChunkAt(bytes, size, at, &child)) return false;
    if (child.id == id) { *found = child; return true; }
    at = static_cast<std::size_t>(child.payloadOffset) + child.length;
  }
  return false;
}

bool PrintParticleVectors(const std::uint8_t* bytes, std::size_t size) {
  S2FileIO::StructureChunk base;
  if (!FindChild(bytes, size, 1, &base)) return false;
  const auto* particle = bytes + base.payloadOffset;
  for (unsigned id = 2; id <= 5; ++id) {
    S2FileIO::StructureChunk vector, countChunk, blob;
    if (!FindChild(particle, base.length, static_cast<std::uint8_t>(id), &vector)) return false;
    const auto* vectorBytes = particle + vector.payloadOffset;
    std::uint32_t count = 0;
    if (!FindChild(vectorBytes, vector.length, 1, &countChunk) || countChunk.length != 4 ||
        !S2FileIO::DecodeStructureScalar(vectorBytes + countChunk.payloadOffset, 4, &count))
      return false;
    const std::size_t elementSize = id == 2 ? 4 : id == 5 ? 20 : 12;
    std::uint64_t hash = UINT64_C(14695981039346656037);
    if (count) {
      if (!FindChild(vectorBytes, vector.length, 2, &blob) ||
          blob.length != static_cast<std::uint64_t>(count) * elementSize) return false;
      for (std::size_t i = 0; i < blob.length; ++i)
        hash = (hash ^ vectorBytes[blob.payloadOffset + i]) * UINT64_C(1099511628211);
    }
    std::cout << " v" << id << "=" << count << ":" << std::hex << hash << std::dec;
  }
  return true;
}

bool PrintFields(const std::uint8_t* bytes, std::size_t size, int depth) {
  for (std::size_t at = 0; at < size;) {
    S2FileIO::StructureChunk field;
    if (!S2FileIO::DecodeStructureChunkAt(bytes, size, at, &field)) return false;
    std::cout << " " << unsigned(field.id) << ":" << field.length;
    if (depth > 0 && field.id == 1 && field.length > 0) {
      S2FileIO::StructureChunk nested;
      const auto* payload = bytes + field.payloadOffset;
      if (S2FileIO::DecodeStructureChunkAt(payload, field.length, 0, &nested)) {
        std::cout << " {";
        if (!PrintFields(payload, field.length, depth - 1)) return false;
        std::cout << " }";
      }
    }
    at = static_cast<std::size_t>(field.payloadOffset) + field.length;
  }
  return true;
}
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "usage: ProbeSavedRuntimeType <game.sav> <type-id-hex>\n";
    return 2;
  }
  char* end = nullptr;
  const unsigned long requested = std::strtoul(argv[2], &end, 16);
  if (!end || end == argv[2] || *end || requested > UINT32_MAX) return 2;
  std::ifstream file(argv[1], std::ios::binary);
  const std::vector<std::uint8_t> save((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
  constexpr std::size_t header = 8 + 320 * 200 * 4;
  if (save.size() <= header || save[0] != 0x22 || save[1] != 0xa0 ||
      save[2] != 0x8c || save[3] != 0x82) return 3;
  const std::uint8_t* data = save.data() + header;
  const std::size_t size = save.size() - header;
  S2FileIO::StructureChunk table, bodies;
  bool haveTable = false, haveBodies = false;
  for (std::size_t at = 0; at < size;) {
    S2FileIO::StructureChunk chunk;
    if (!S2FileIO::DecodeStructureChunkAt(data, size, at, &chunk) || chunk.id == 3)
      return 3;
    if (chunk.id == 0) { table = chunk; haveTable = true; }
    if (chunk.id == 2) { bodies = chunk; haveBodies = true; }
    at = static_cast<std::size_t>(chunk.payloadOffset) + chunk.length;
  }
  if (!haveTable || !haveBodies) return 3;
  std::vector<S2FileIO::StructureObjectRecord> records;
  std::vector<S2FileIO::StructureObjectBody> objects;
  if (!S2FileIO::DecodeStructureObjectTable(data + table.payloadOffset, table.length, &records) ||
      !S2FileIO::IndexStructureObjectBodies(data + bodies.payloadOffset, bodies.length, &objects))
    return 3;
  std::map<std::uint32_t, std::uint32_t> types;
  for (const auto& record : records) types[record.wireId] = record.typeId;
  unsigned matches = 0;
  const std::uint8_t* payload = data + bodies.payloadOffset;
  for (const auto& object : objects) {
    const auto found = types.find(object.wireId);
    if (found == types.end() || found->second != requested) continue;
    ++matches;
    if (matches > 10) continue;
    std::cout << "wire=" << object.wireId << " body=" << object.bodyLength << " fields";
    if (!PrintFields(payload + object.bodyOffset, object.bodyLength, 3)) return 3;
    if (requested == 0x10441191 &&
        !PrintParticleVectors(payload + object.bodyOffset, object.bodyLength)) return 3;
    std::cout << "\n";
  }
  std::cout << "matches=" << matches << "\n";
  return 0;
}
