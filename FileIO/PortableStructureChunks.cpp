#include "PortableStructureChunks.h"

#include <fstream>
#include <cstring>

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

bool S2_STRUCTURE_CALL DecodeStructureChunkAt(const std::uint8_t* bytes,
                                              std::size_t size,
                                              std::size_t offset,
                                              StructureChunk* chunk) {
  if (!bytes || !chunk || offset > size || size - offset < 2) return false;
  const std::uint8_t* encoded = bytes + offset + 1;
  const std::size_t prefix = (encoded[0] & 1) ? 4 : 1;
  if (size - offset < 1 + prefix) return false;
  const std::size_t payloadOffset = offset + 1 + prefix;
  std::uint32_t length = 0;
  if (!DecodeStructureLength(encoded, prefix, size - payloadOffset, &length))
    return false;
  *chunk = {bytes[offset], payloadOffset, length};
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

bool S2_STRUCTURE_CALL DecodeStructureObjectTable(
    const std::uint8_t* bytes, std::size_t length,
    std::vector<StructureObjectRecord>* records) {
  if (!records) return false;
  records->clear();
  if (length % 9 != 0 || (length && !bytes)) return false;
  records->reserve(length / 9);
  for (std::size_t offset = 0; offset < length; offset += 9) {
    std::uint32_t typeId = 0, wireId = 0;
    for (unsigned i = 0; i != 4; ++i) {
      typeId |= std::uint32_t(bytes[offset + i]) << (i * 8);
      wireId |= std::uint32_t(bytes[offset + 4 + i]) << (i * 8);
    }
    records->push_back({typeId, wireId, bytes[offset + 8] != 0});
  }
  return true;
}

bool S2_STRUCTURE_CALL IndexStructureObjectBodies(
    const std::uint8_t* bytes, std::size_t length,
    std::vector<StructureObjectBody>* bodies) {
  if (!bodies) return false;
  bodies->clear();
  if (length && !bytes) return false;
  std::size_t offset = 0;
  while (offset < length) {
    StructureChunk outer;
    if (!DecodeStructureChunkAt(bytes, length, offset, &outer) || outer.id != 1)
      return false;
    const std::size_t outerEnd = static_cast<std::size_t>(outer.payloadOffset) + outer.length;
    std::size_t childOffset = static_cast<std::size_t>(outer.payloadOffset);
    std::uint32_t wireId = 0;
    StructureChunk body;
    bool foundId = false, foundBody = false;
    while (childOffset < outerEnd) {
      StructureChunk child;
      if (!DecodeStructureChunkAt(bytes, outerEnd, childOffset, &child)) return false;
      if (child.id == 0) {
        if (foundId || child.length != 4) return false;
        const std::size_t at = static_cast<std::size_t>(child.payloadOffset);
        for (unsigned i = 0; i != 4; ++i)
          wireId |= std::uint32_t(bytes[at + i]) << (8 * i);
        foundId = true;
      } else if (child.id == 1) {
        if (foundBody) return false;
        body = child;
        foundBody = true;
      }
      childOffset = static_cast<std::size_t>(child.payloadOffset) + child.length;
    }
    if (!foundId || !foundBody) return false;
    bodies->push_back({wireId, body.payloadOffset, body.length});
    offset = outerEnd;
  }
  return true;
}

bool S2_STRUCTURE_CALL DecodeStructureUtf16(const std::uint8_t* bytes,
                                            std::size_t length,
                                            std::wstring* value) {
  if (!value || length % 2 || (length && !bytes)) return false;
  std::wstring decoded;
  decoded.reserve(length / 2);
  for (std::size_t offset = 0; offset < length; offset += 2) {
    const std::uint32_t unit = std::uint32_t(bytes[offset]) |
                               (std::uint32_t(bytes[offset + 1]) << 8);
    if (sizeof(wchar_t) == 4 && unit >= 0xd800 && unit <= 0xdbff &&
        offset + 3 < length) {
      const std::uint32_t low = std::uint32_t(bytes[offset + 2]) |
                                (std::uint32_t(bytes[offset + 3]) << 8);
      if (low >= 0xdc00 && low <= 0xdfff) {
        decoded.push_back(static_cast<wchar_t>(
            0x10000 + ((unit - 0xd800) << 10) + (low - 0xdc00)));
        offset += 2;
        continue;
      }
    }
    decoded.push_back(static_cast<wchar_t>(unit));
  }
  value->swap(decoded);
  return true;
}

bool S2_STRUCTURE_CALL EncodeStructureUtf16(const std::wstring& value,
                                            std::vector<std::uint8_t>* bytes) {
  if (!bytes) return false;
  std::vector<std::uint8_t> encoded;
  encoded.reserve(value.size() * 2);
  for (wchar_t character : value) {
    const std::uint32_t codepoint = static_cast<std::uint32_t>(character);
    if (codepoint > 0x10ffff) return false;
    const auto append = [&encoded](std::uint32_t unit) {
      encoded.push_back(static_cast<std::uint8_t>(unit));
      encoded.push_back(static_cast<std::uint8_t>(unit >> 8));
    };
    if (codepoint <= 0xffff) {
      append(codepoint);
    } else {
      const std::uint32_t supplementary = codepoint - 0x10000;
      append(0xd800 + (supplementary >> 10));
      append(0xdc00 + (supplementary & 0x3ff));
    }
  }
  bytes->swap(encoded);
  return true;
}

bool S2_STRUCTURE_CALL CopyStructureField(const std::uint8_t* source,
                                          std::size_t sourceSize,
                                          void* destination,
                                          std::size_t destinationSize) {
  if (sourceSize != destinationSize ||
      (sourceSize && (!source || !destination))) return false;
  if (sourceSize) std::memcpy(destination, source, sourceSize);
  return true;
}

} // namespace S2FileIO
