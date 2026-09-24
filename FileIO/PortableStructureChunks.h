#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace S2FileIO {

#if defined(_MSC_VER)
#define S2_STRUCTURE_CALL __cdecl
#else
#define S2_STRUCTURE_CALL
#endif

struct StructureChunk {
  std::uint8_t id = 0;
  std::uint64_t payloadOffset = 0;
  std::uint32_t length = 0;
};

struct StructureObjectRecord {
  std::uint32_t typeId = 0;
  std::uint32_t wireId = 0;
  bool valid = false;
};

// CStructureSaver stores a byte tag followed by a 1- or 4-byte LE value:
// encoded = (payload length << 1) | (extended ? 1 : 0).
// `remaining` is the number of file bytes after the encoded length field.
bool S2_STRUCTURE_CALL DecodeStructureLength(const std::uint8_t* encoded,
                                              std::size_t encodedSize,
                                              std::uint64_t remaining,
                                              std::uint32_t* length);
// Decode one nested chunk in an already-loaded byte span. Returns false at
// the end or for a truncated/out-of-range chunk; offset is relative to bytes.
bool S2_STRUCTURE_CALL DecodeStructureChunkAt(const std::uint8_t* bytes,
                                              std::size_t size,
                                              std::size_t offset,
                                              StructureChunk* chunk);
bool S2_STRUCTURE_CALL ScanStructureFile(const std::string& path,
                                         std::vector<StructureChunk>* chunks,
                                         std::string* error = nullptr);
// Chunk 0 contains fixed nine-byte descriptors, independent of host pointer size.
bool S2_STRUCTURE_CALL DecodeStructureObjectTable(
    const std::uint8_t* bytes, std::size_t length,
    std::vector<StructureObjectRecord>* records);

// Wide strings on disk are UTF-16LE code units, not host wchar_t arrays.
// Unpaired surrogates are preserved as code units for legacy round trips.
bool S2_STRUCTURE_CALL DecodeStructureUtf16(const std::uint8_t* bytes,
                                            std::size_t length,
                                            std::wstring* value);
bool S2_STRUCTURE_CALL EncodeStructureUtf16(const std::wstring& value,
                                            std::vector<std::uint8_t>* bytes);


} // namespace S2FileIO
