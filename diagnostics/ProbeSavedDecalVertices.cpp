#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

namespace {
bool FindChild(const std::uint8_t* bytes, std::size_t size,
               const S2FileIO::StructureChunk& parent, std::uint8_t id,
               S2FileIO::StructureChunk* found) {
  const std::size_t end = static_cast<std::size_t>(parent.payloadOffset) + parent.length;
  if (end > size) return false;
  for (std::size_t at = static_cast<std::size_t>(parent.payloadOffset); at < end;) {
    S2FileIO::StructureChunk child;
    if (!S2FileIO::DecodeStructureChunkAt(bytes, end, at, &child)) return false;
    if (child.id == id) { *found = child; return true; }
    at = static_cast<std::size_t>(child.payloadOffset) + child.length;
  }
  return false;
}

bool VectorCount(const std::uint8_t* bytes, std::size_t size,
                 const S2FileIO::StructureChunk& vector,
                 std::size_t elementSize, std::uint32_t* count) {
  S2FileIO::StructureChunk countChunk, dataChunk;
  if (!FindChild(bytes, size, vector, 1, &countChunk) || countChunk.length != 4) return false;
  if (!S2FileIO::DecodeStructureScalar(bytes + countChunk.payloadOffset, 4, count)) return false;
  if (!*count) return !FindChild(bytes, size, vector, 2, &dataChunk);
  return FindChild(bytes, size, vector, 2, &dataChunk) &&
         dataChunk.length == static_cast<std::uint64_t>(*count) * elementSize;
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: ProbeSavedDecalVertices <game.sav>\n";
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
  // v1.x header: two int32s plus the fixed 320x200x4 screenshot. This probe
  // accepts no active mods and uncompressed structure chunks only.
  constexpr std::size_t header = 8 + 320 * 200 * 4;
  if (bytes.size() <= header ||
      (bytes[0] != 0x22 || bytes[1] != 0xa0 || bytes[2] != 0x8c || bytes[3] != 0x82) ||
      bytes[4] || bytes[5] || bytes[6] || bytes[7]) {
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
  std::uint64_t objects = 0, vertices = 0, weights = 0;
  for (const auto& object : objectBodies) {
    const auto type = types.find(object.wireId);
    if (type == types.end() ||
        (type->second != 0x004c2170 && type->second != 0x006c2160)) continue;
    ++objects;
    const std::uint8_t* payload = data + bodies.payloadOffset;
    S2FileIO::StructureChunk objectChunk{1, object.bodyOffset, object.bodyLength};
    S2FileIO::StructureChunk base, objectData, vertexVector, weightVector;
    if (!FindChild(payload, bodies.length, objectChunk, 1, &base) ||
        !FindChild(payload, bodies.length, base, 3, &objectData) ||
        !FindChild(payload, bodies.length, objectData, 2, &vertexVector) ||
        !FindChild(payload, bodies.length, objectData, 4, &weightVector)) return 1;
    std::uint32_t vertexCount = 0, weightCount = 0;
    if (!VectorCount(payload, bodies.length, vertexVector, 32, &vertexCount) ||
        !VectorCount(payload, bodies.length, weightVector, 20, &weightCount)) return 1;
    vertices += vertexCount;
    weights += weightCount;
    std::cout << "decal wire=" << object.wireId << " type=0x" << std::hex << type->second
              << std::dec << " vertices=" << vertexCount << " weights=" << weightCount << "\n";
  }
  std::cout << "decal_objects=" << objects << " vertices=" << vertices
            << " weights=" << weights << "\n";
  return 0;
}
