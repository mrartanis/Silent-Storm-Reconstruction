#include "PortableStructureChunks.h"

#include <fstream>

namespace S2FileIO {
namespace {
bool Fail(std::string* error, const char* message) {
  if (error) *error = message;
  return false;
}
} // namespace

bool S2_STRUCTURE_CALL DecodeStructureLength(const std::uint8_t* encoded,
                                              std::size_t encodedSize,
                                              std::uint64_t remaining,
                                              std::uint32_t* length) {
  if (!encoded || !length || !encodedSize) return false;
  const bool extended = (encoded[0] & 1) != 0;
  if (encodedSize != (extended ? 4u : 1u)) return false;
  std::uint32_t raw = encoded[0];
  if (extended) {
    raw |= std::uint32_t(encoded[1]) << 8;
    raw |= std::uint32_t(encoded[2]) << 16;
    raw |= std::uint32_t(encoded[3]) << 24;
  }
  const std::uint32_t decoded = raw >> 1;
  if (decoded > remaining) return false;
  *length = decoded;
  return true;
}

bool S2_STRUCTURE_CALL ScanStructureFile(const std::string& path,
                                         std::vector<StructureChunk>* chunks,
                                         std::string* error) {
  if (!chunks) return Fail(error, "missing output");
  chunks->clear();
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) return Fail(error, "cannot open structure file");
  const std::streamoff end = file.tellg();
  if (end < 0) return Fail(error, "cannot size structure file");
  const std::uint64_t size = static_cast<std::uint64_t>(end);
  std::uint64_t offset = 0;
  while (offset < size) {
    if (size - offset < 2) return Fail(error, "truncated chunk header");
    file.seekg(static_cast<std::streamoff>(offset));
    std::uint8_t id = 0, encoded[4] = {};
    if (!file.read(reinterpret_cast<char*>(&id), 1) ||
        !file.read(reinterpret_cast<char*>(encoded), 1))
      return Fail(error, "short chunk header");
    const std::size_t prefix = (encoded[0] & 1) ? 4 : 1;
    if (size - offset < 1 + prefix) return Fail(error, "truncated chunk length");
    if (prefix == 4 && !file.read(reinterpret_cast<char*>(encoded + 1), 3))
      return Fail(error, "short chunk length");
    const std::uint64_t payloadOffset = offset + 1 + prefix;
    std::uint32_t length = 0;
    if (!DecodeStructureLength(encoded, prefix, size - payloadOffset, &length))
      return Fail(error, "chunk length out of range");
    chunks->push_back({id, payloadOffset, length});
    offset = payloadOffset + length;
  }
  return true;
}

} // namespace S2FileIO
