#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

namespace {
struct Grid {
  std::uint32_t wireId = 0;
  std::uint32_t x = 0, y = 0, z = 0;
  float min[3] = {}, max[3] = {};
  std::array<std::uint8_t, 96> planeBox = {};
  std::vector<std::uint8_t> hp;
};

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

bool ReadU32Child(const std::uint8_t* bytes, std::size_t size,
                  const S2FileIO::StructureChunk& parent, std::uint8_t id,
                  std::uint32_t* value) {
  S2FileIO::StructureChunk child;
  return FindChild(bytes, size, parent, id, &child) && child.length == 4 &&
         S2FileIO::DecodeStructureScalar(bytes + child.payloadOffset, 4, value);
}

bool ReadGrids(const char* path, std::vector<Grid>* grids) {
  std::ifstream file(path, std::ios::binary);
  const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
  constexpr std::size_t header = 8 + 320 * 200 * 4;
  if (!grids || bytes.size() <= header || bytes[0] != 0x22 || bytes[1] != 0xa0 ||
      bytes[2] != 0x8c || bytes[3] != 0x82 || bytes[4] || bytes[5] ||
      bytes[6] || bytes[7]) return false;
  const std::uint8_t* data = bytes.data() + header;
  const std::size_t size = bytes.size() - header;
  S2FileIO::StructureChunk table, bodies;
  bool haveTable = false, haveBodies = false;
  for (std::size_t at = 0; at < size;) {
    S2FileIO::StructureChunk chunk;
    if (!S2FileIO::DecodeStructureChunkAt(data, size, at, &chunk) || chunk.id == 3)
      return false;  // packed saves are not supported by this probe
    if (chunk.id == 0) { table = chunk; haveTable = true; }
    if (chunk.id == 2) { bodies = chunk; haveBodies = true; }
    at = static_cast<std::size_t>(chunk.payloadOffset) + chunk.length;
  }
  if (!haveTable || !haveBodies) return false;
  std::vector<S2FileIO::StructureObjectRecord> records;
  std::vector<S2FileIO::StructureObjectBody> objectBodies;
  if (!S2FileIO::DecodeStructureObjectTable(data + table.payloadOffset, table.length, &records) ||
      !S2FileIO::IndexStructureObjectBodies(data + bodies.payloadOffset, bodies.length, &objectBodies))
    return false;
  std::map<std::uint32_t, std::uint32_t> types;
  for (const auto& record : records) types[record.wireId] = record.typeId;
  const std::uint8_t* payload = data + bodies.payloadOffset;
  bool printedFirstTransform = false;
  for (const auto& object : objectBodies) {
    const auto type = types.find(object.wireId);
    if (!printedFirstTransform && type != types.end() && type->second == 0x025a1130) {
      S2FileIO::StructureChunk transformObject{1, object.bodyOffset, object.bodyLength};
      S2FileIO::StructureChunk transform;
      float forward[16];
      if (!FindChild(payload, bodies.length, transformObject, 2, &transform) ||
          transform.length != 128 ||
          !S2FileIO::DecodeStructureFloatFields(payload + transform.payloadOffset, 64, forward, 16))
        return false;
      std::cout << "first_transform wire=" << object.wireId << " translation=("
                << forward[3] << "," << forward[7] << "," << forward[11]
                << ") basis=(" << forward[0] << "," << forward[1] << ","
                << forward[4] << "," << forward[5] << ")\n";
      printedFirstTransform = true;
    }
    if (type == types.end() || type->second != 0x02741121) continue; // CBuildingGrid
    S2FileIO::StructureChunk objectChunk{1, object.bodyOffset, object.bodyLength};
    S2FileIO::StructureChunk array, vector, blob, min, max, box;
    Grid grid;
    grid.wireId = object.wireId;
    std::uint32_t count = 0;
    if (!FindChild(payload, bodies.length, objectChunk, 2, &array) ||
        !FindChild(payload, bodies.length, objectChunk, 4, &min) ||
        !FindChild(payload, bodies.length, objectChunk, 5, &max) ||
        !FindChild(payload, bodies.length, objectChunk, 10, &box) || box.length != 96 ||
        !S2FileIO::DecodeStructureFloatFields(payload + min.payloadOffset, min.length, grid.min, 3) ||
        !S2FileIO::DecodeStructureFloatFields(payload + max.payloadOffset, max.length, grid.max, 3) ||
        !FindChild(payload, bodies.length, array, 2, &vector) ||
        !ReadU32Child(payload, bodies.length, array, 3, &grid.x) ||
        !ReadU32Child(payload, bodies.length, array, 4, &grid.y) ||
        !ReadU32Child(payload, bodies.length, array, 5, &grid.z) ||
        !ReadU32Child(payload, bodies.length, vector, 1, &count) ||
        !FindChild(payload, bodies.length, vector, 2, &blob) ||
        blob.length != count ||
        static_cast<std::uint64_t>(grid.x) * grid.y * grid.z != count)
      return false;
    float decodedBox[24];
    if (!S2FileIO::DecodeStructurePlaneBox(payload + box.payloadOffset, box.length, decodedBox))
      return false;
    for (std::size_t i = 0; i < 96; ++i) grid.planeBox[i] = payload[box.payloadOffset + i];
    grid.hp.assign(payload + blob.payloadOffset, payload + blob.payloadOffset + blob.length);
    grids->push_back(grid);
  }
  return !grids->empty();
}

void Print(const std::vector<Grid>& grids) {
  for (std::size_t i = 0; i < grids.size(); ++i) {
    std::uint64_t sum = 0, hash = UINT64_C(14695981039346656037);
    std::uint64_t boxHash = UINT64_C(14695981039346656037);
    for (const std::uint8_t byte : grids[i].planeBox)
      boxHash = (boxHash ^ byte) * UINT64_C(1099511628211);
    std::size_t positive = 0;
    std::size_t best = 0;
    double bestDistance = 1e100;
    std::uint32_t minX = grids[i].x, minY = grids[i].y, minZ = grids[i].z;
    std::uint32_t maxX = 0, maxY = 0, maxZ = 0;
    for (std::size_t j = 0; j < grids[i].hp.size(); ++j) {
      const std::uint8_t hp = grids[i].hp[j];
      sum += hp;
      positive += hp != 0;
      hash = (hash ^ hp) * UINT64_C(1099511628211);
      if (hp && grids[i].x && grids[i].y) {
        const std::uint32_t x = static_cast<std::uint32_t>(j % grids[i].x);
        const std::uint32_t y = static_cast<std::uint32_t>((j / grids[i].x) % grids[i].y);
        const std::uint32_t z = static_cast<std::uint32_t>(j / (grids[i].x * grids[i].y));
        if (x < minX) minX = x; if (x > maxX) maxX = x;
        if (y < minY) minY = y; if (y > maxY) maxY = y;
        if (z < minZ) minZ = z; if (z > maxZ) maxZ = z;
        const double dx = static_cast<double>(x) - grids[i].x / 2.0;
        const double dy = static_cast<double>(y) - grids[i].y / 2.0;
        const double dz = static_cast<double>(z) - grids[i].z / 2.0;
        const double distance = dx * dx + dy * dy + dz * dz;
        if (distance < bestDistance) { bestDistance = distance; best = j; }
      }
    }
    std::cout << "grid=" << i << " wire=" << grids[i].wireId << " size="
              << grids[i].x << "x" << grids[i].y << "x" << grids[i].z
              << " box=(" << grids[i].min[0] << "," << grids[i].min[1] << "," << grids[i].min[2]
              << ")-(" << grids[i].max[0] << "," << grids[i].max[1] << "," << grids[i].max[2] << ")"
              << " positive=" << positive << " hp_sum=" << sum
              << " hash=" << std::hex << hash << " plane_hash=" << boxHash << std::dec << "\n";
    if (positive) std::cout << "occupied_index_box=(" << minX << "," << minY << "," << minZ
                            << ")-(" << maxX << "," << maxY << "," << maxZ
                            << ") center_sample=(" << best % grids[i].x << ","
                            << (best / grids[i].x) % grids[i].y << ","
                            << best / (grids[i].x * grids[i].y) << ") hp="
                            << unsigned(grids[i].hp[best]) << "\n";
  }
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2 && argc != 3) {
    std::cerr << "usage: ProbeSavedBuildingVoxels <before.sav> [after.sav]\n";
    return 2;
  }
  std::vector<Grid> before, after;
  if (!ReadGrids(argv[1], &before)) { std::cerr << "cannot read first grid set\n"; return 1; }
  std::cout << "before grids=" << before.size() << "\n";
  Print(before);
  if (argc == 2) return 0;
  if (!ReadGrids(argv[2], &after) || after.size() != before.size()) {
    std::cerr << "cannot read or match second grid set\n";
    return 1;
  }
  std::cout << "after grids=" << after.size() << "\n";
  Print(after);
  std::size_t decreased = 0, increased = 0, newlyZero = 0;
  for (std::size_t i = 0; i < before.size(); ++i) {
    if (before[i].x != after[i].x || before[i].y != after[i].y ||
        before[i].z != after[i].z || before[i].hp.size() != after[i].hp.size()) {
      std::cerr << "grid dimensions changed at index " << i << "\n";
      return 1;
    }
    std::size_t gridDecreased = 0, gridIncreased = 0, gridNewlyZero = 0, boxChanged = 0;
    for (std::size_t j = 0; j < before[i].hp.size(); ++j) {
      gridDecreased += after[i].hp[j] < before[i].hp[j];
      gridIncreased += after[i].hp[j] > before[i].hp[j];
      gridNewlyZero += before[i].hp[j] > 0 && after[i].hp[j] == 0;
    }
    for (std::size_t j = 0; j < 96; ++j)
      boxChanged += before[i].planeBox[j] != after[i].planeBox[j];
    std::cout << "grid=" << i << " decreased=" << gridDecreased
              << " increased=" << gridIncreased << " newly_zero=" << gridNewlyZero
              << " plane_changed=" << boxChanged << "\n";
    decreased += gridDecreased; increased += gridIncreased; newlyZero += gridNewlyZero;
  }
  std::cout << "total decreased=" << decreased << " increased=" << increased
            << " newly_zero=" << newlyZero << "\n";
  return 0;
}
