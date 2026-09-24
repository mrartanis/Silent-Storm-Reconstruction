#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <vector>

int main() {
  std::uint32_t length = 0;
  const std::uint8_t shortLength[] = {8};
  const std::uint8_t longLength[] = {0xf9, 0x0e, 0x00, 0x00};
  const std::uint8_t objectTable[] = {
      0x30, 0x31, 0x84, 0xa1, 0x01, 0x00, 0x00, 0x00, 0x01,
      0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff, 0x00};
  std::vector<S2FileIO::StructureObjectRecord> records;
  const std::uint8_t nested[] = {7, 8, 1, 2, 3, 4, 9, 0xf9, 0x0e, 0, 0};
  S2FileIO::StructureChunk chunk;
  return S2FileIO::DecodeStructureLength(shortLength, 1, 4, &length) && length == 4 &&
         S2FileIO::DecodeStructureLength(longLength, 4, 1916, &length) && length == 1916 &&
         !S2FileIO::DecodeStructureLength(shortLength, 1, 3, &length) &&
         !S2FileIO::DecodeStructureLength(longLength, 1, 1916, &length) &&
         !S2FileIO::DecodeStructureLength(nullptr, 0, 0, &length) &&
         S2FileIO::DecodeStructureChunkAt(nested, sizeof(nested), 0, &chunk) &&
         chunk.id == 7 && chunk.payloadOffset == 2 && chunk.length == 4 &&
         !S2FileIO::DecodeStructureChunkAt(nested, sizeof(nested), 6, &chunk) &&
         !S2FileIO::DecodeStructureChunkAt(nested, sizeof(nested), sizeof(nested), &chunk) &&
         !S2FileIO::DecodeStructureChunkAt(nested, 1, 0, &chunk) &&
         S2FileIO::DecodeStructureObjectTable(objectTable, sizeof(objectTable), &records) &&
         records.size() == 2 && records[0].typeId == 0xa1843130 &&
         records[0].wireId == 1 && records[0].valid &&
         records[1].typeId == 0x12345678 && records[1].wireId == 0xffffffff &&
         !records[1].valid &&
         !S2FileIO::DecodeStructureObjectTable(objectTable, sizeof(objectTable) - 1, &records) &&
         records.empty() &&
         !S2FileIO::DecodeStructureObjectTable(nullptr, 9, &records)
             ? 0 : 1;
}
