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

// CStructureSaver stores a byte tag followed by a 1- or 4-byte LE value:
// encoded = (payload length << 1) | (extended ? 1 : 0).
// `remaining` is the number of file bytes after the encoded length field.
bool S2_STRUCTURE_CALL DecodeStructureLength(const std::uint8_t* encoded,
                                              std::size_t encodedSize,
                                              std::uint64_t remaining,
                                              std::uint32_t* length);
bool S2_STRUCTURE_CALL ScanStructureFile(const std::string& path,
                                         std::vector<StructureChunk>* chunks,
                                         std::string* error = nullptr);


} // namespace S2FileIO
