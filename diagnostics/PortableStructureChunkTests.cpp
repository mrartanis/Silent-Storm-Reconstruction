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
  const std::uint8_t utf16[] = {
      0x41, 0x00, 0x16, 0x04, 0x3d, 0xd8, 0x00, 0xde,
      0x00, 0xd8, 0x00, 0x00};
  std::wstring wide;
  std::vector<std::uint8_t> encoded;
  if (!S2FileIO::DecodeStructureUtf16(utf16, sizeof(utf16), &wide) ||
      wide.size() != (sizeof(wchar_t) == 2 ? 6u : 5u) ||
      wide[0] != L'A' || wide[1] != 0x416 ||
      wide[2] != (sizeof(wchar_t) == 2 ? 0xd83d : 0x1f600) ||
      !S2FileIO::EncodeStructureUtf16(wide, &encoded) ||
      encoded != std::vector<std::uint8_t>(utf16, utf16 + sizeof(utf16)) ||
      S2FileIO::DecodeStructureUtf16(utf16, sizeof(utf16) - 1, &wide) ||
      !S2FileIO::DecodeStructureUtf16(nullptr, 0, &wide) || !wide.empty())
    return 1;
  if (sizeof(wchar_t) == 4) {
    wide.push_back(static_cast<wchar_t>(0x110000));
    if (S2FileIO::EncodeStructureUtf16(wide, &encoded)) return 1;
  }
  const std::uint8_t field[] = {0x12, 0x34, 0x56, 0x78};
  std::uint8_t copied[] = {0, 0, 0, 0};
  if (!S2FileIO::CopyStructureField(field, sizeof(field), copied, sizeof(copied)) ||
      copied[0] != 0x12 || copied[3] != 0x78 ||
      S2FileIO::CopyStructureField(field, 3, copied, sizeof(copied)) ||
      S2FileIO::CopyStructureField(field, sizeof(field), copied, 3) ||
      S2FileIO::CopyStructureField(nullptr, sizeof(field), copied, sizeof(copied)) ||
      !S2FileIO::CopyStructureField(nullptr, 0, nullptr, 0))
    return 1;
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
