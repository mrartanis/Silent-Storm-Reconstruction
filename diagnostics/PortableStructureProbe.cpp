#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc < 2 || argc > 5) return 2;
  bool showObjects = false, showNested = false, showShape = false;
  for (int i = 2; i < argc; ++i) {
    const std::string option(argv[i]);
    if (option == "--objects") showObjects = true;
    else if (option == "--nested") showNested = true;
    else if (option == "--shape") { showShape = true; showNested = true; }
    else return 2;
  }
  std::vector<S2FileIO::StructureChunk> chunks;
  std::string error;
  if (!S2FileIO::ScanStructureFile(argv[1], &chunks, &error)) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 3;
  }
  std::ifstream file(argv[1], std::ios::binary);
  std::vector<char> buffer(65536);
  std::map<std::uint32_t, std::uint32_t> typesByWireId;
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
    if (((showObjects || showNested) && chunk.id == 0) ||
        (showNested && chunk.id == 2)) {
      file.clear();
      file.seekg(static_cast<std::streamoff>(chunk.payloadOffset));
      std::vector<std::uint8_t> bytes(chunk.length);
      if (!file.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) return 5;
      if (chunk.id == 0) {
        std::vector<S2FileIO::StructureObjectRecord> records;
        if (!S2FileIO::DecodeStructureObjectTable(bytes.data(), bytes.size(), &records)) return 6;
        std::set<std::uint32_t> wireIds;
        std::size_t valid = 0;
        for (const auto& record : records) {
          wireIds.insert(record.wireId);
          typesByWireId[record.wireId] = record.typeId;
          if (record.valid) ++valid;
        }
        if (showObjects)
          std::printf("objects %zu valid %zu unique-ids %zu\n",
              records.size(), valid, wireIds.size());
      } else {
        std::size_t offset = 0, count = 0;
        while (offset < bytes.size()) {
          S2FileIO::StructureChunk inner;
          if (!S2FileIO::DecodeStructureChunkAt(bytes.data(), bytes.size(), offset, &inner))
            return 7;
          offset = static_cast<std::size_t>(inner.payloadOffset) + inner.length;
          ++count;
        }
        std::printf("nested-data %zu end %zu\n", count, offset);
        std::vector<S2FileIO::StructureObjectBody> bodies;
        if (!S2FileIO::IndexStructureObjectBodies(bytes.data(), bytes.size(), &bodies))
          return 8;
        std::set<std::uint32_t> types, bodyIds;
        std::size_t matched = 0;
        for (const auto& body : bodies) {
          bodyIds.insert(body.wireId);
          const auto found = typesByWireId.find(body.wireId);
          if (found != typesByWireId.end()) {
            ++matched;
            types.insert(found->second);
          }
        }
        std::printf("object-bodies %zu matched %zu unique-ids %zu types %zu\n",
            bodies.size(), matched, bodyIds.size(), types.size());
        if (showShape) {
          for (std::size_t i = 0; i < bodies.size() && i < 3; ++i) {
            const auto& body = bodies[i];
            const auto* payload = bytes.data() + body.bodyOffset;
            std::size_t at = 0;
            std::printf("body %zu wire %08x type %08x fields", i, body.wireId,
                typesByWireId[body.wireId]);
            while (at < body.bodyLength) {
              S2FileIO::StructureChunk field;
              if (!S2FileIO::DecodeStructureChunkAt(payload, body.bodyLength, at, &field))
                return 9;
              std::printf(" %u:%u", unsigned(field.id), field.length);
              if (i == 0 && (field.id == 2 || field.id == 5 || field.id == 6)) {
                const auto* fieldBytes = payload + field.payloadOffset;
                S2FileIO::StructureChunk first;
                if (field.length && S2FileIO::DecodeStructureChunkAt(
                    fieldBytes, field.length, 0, &first)) {
                  std::printf("[first=%u:%u", unsigned(first.id), first.length);
                  const auto* firstBytes = fieldBytes + first.payloadOffset;
                  std::size_t childAt = 0;
                  unsigned childCount = 0;
                  while (childAt < first.length && childCount < 4) {
                    S2FileIO::StructureChunk child;
                    if (!S2FileIO::DecodeStructureChunkAt(
                        firstBytes, first.length, childAt, &child)) break;
                    std::printf("/%u:%u", unsigned(child.id), child.length);
                    childAt = static_cast<std::size_t>(child.payloadOffset) + child.length;
                    ++childCount;
                  }
                  std::printf("]");
                }
              }
              at = static_cast<std::size_t>(field.payloadOffset) + field.length;
            }
            std::printf("\n");
          }
        }
      }
    }
  }
  return 0;
}
