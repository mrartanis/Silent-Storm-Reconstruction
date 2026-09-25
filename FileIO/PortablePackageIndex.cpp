#include "PortablePackageIndex.h"

#include <filesystem>
#include <fstream>
#include <limits>

namespace S2FileIO {
namespace {
constexpr std::uint32_t kLegacySignature = 0x95938921;
constexpr std::uint32_t kV1Signature = 0x96948A22;

std::uint32_t U32(const std::uint8_t* p) {
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

struct Chunk {
  std::uint8_t id = 0;
  std::size_t begin = 0;
  std::size_t end = 0;
};

bool NextChunk(const std::vector<std::uint8_t>& bytes, std::size_t limit,
               std::size_t* cursor, Chunk* out) {
  if (*cursor >= limit || limit - *cursor < 2) return false;
  const std::uint8_t id = bytes[(*cursor)++];
  std::uint32_t encoded = bytes[(*cursor)++];
  if (encoded & 1) {
    if (limit - *cursor < 3) return false;
    encoded |= std::uint32_t(bytes[(*cursor)++]) << 8;
    encoded |= std::uint32_t(bytes[(*cursor)++]) << 16;
    encoded |= std::uint32_t(bytes[(*cursor)++]) << 24;
  }
  const std::size_t length = encoded >> 1;
  if (length > limit - *cursor) return false;
  out->id = id;
  out->begin = *cursor;
  out->end = *cursor + length;
  *cursor = out->end;
  return true;
}

bool Fail(std::string* error, const char* message) {
  if (error) *error = message;
  return false;
}

bool EqualAsciiNoCase(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    unsigned char left = static_cast<unsigned char>(a[i]);
    unsigned char right = static_cast<unsigned char>(b[i]);
    if (left >= 'A' && left <= 'Z') left += 'a' - 'A';
    if (right >= 'A' && right <= 'Z') right += 'a' - 'A';
    if (left != right) return false;
  }
  return true;
}

bool ResolvePackagePath(const std::string& requested, std::string* resolved) {
#if defined(_WIN32)
  // Windows already handles the release's case-insensitive package lookup.
  *resolved = requested;
  return true;
#else
  // The game supplies Windows-style relative resource paths. Resolve each
  // component against its actual case on a case-sensitive host, preferring
  // an exact spelling and refusing ambiguous case-insensitive matches.
  std::string normalized = requested;
  for (char& ch : normalized) if (ch == '\\') ch = '/';
  const std::filesystem::path input(normalized);
  std::filesystem::path current = input.is_absolute() ? input.root_path() : ".";
  for (const auto& component : input.relative_path()) {
    if (component == ".") continue;
    if (component == "..") { current /= component; continue; }
    const std::filesystem::path exact = current / component;
    std::error_code error;
    if (std::filesystem::exists(exact, error) && !error) {
      current = exact;
      continue;
    }
    if (error) return false;
    std::filesystem::path match;
    const std::string wanted = component.string();
    for (std::filesystem::directory_iterator it(current, error), end;
         !error && it != end; it.increment(error)) {
      if (!EqualAsciiNoCase(it->path().filename().string(), wanted)) continue;
      if (!match.empty()) return false;
      match = it->path();
    }
    if (error || match.empty()) return false;
    current = match;
  }
  *resolved = current.string();
  return true;
#endif
}
} // namespace

bool PortablePackageIndex::Open(const std::string& path, std::string* error) {
  path_.clear();
  entries_.clear();
  indexOffset_ = 0;
  fileSize_ = 0;
  std::string resolvedPath;
  if (!ResolvePackagePath(path, &resolvedPath))
    return Fail(error, "cannot resolve package path");
  std::ifstream file(resolvedPath, std::ios::binary | std::ios::ate);
  if (!file) return Fail(error, "cannot open package");
  const std::streamoff size = file.tellg();
  if (size < 8 || static_cast<std::uint64_t>(size) >
      std::numeric_limits<std::uint32_t>::max())
    return Fail(error, "unsupported package size");
  fileSize_ = static_cast<std::uint64_t>(size);
  file.seekg(0);
  std::uint8_t header[8] = {};
  if (!file.read(reinterpret_cast<char*>(header), sizeof(header)))
    return Fail(error, "short package header");
  const std::uint32_t signature = U32(header);
  if (signature != kLegacySignature && signature != kV1Signature)
    return Fail(error, "wrong package signature");
  indexOffset_ = U32(header + 4);
  if (indexOffset_ < 8 || indexOffset_ >= fileSize_)
    return Fail(error, "index offset out of range");
  const std::size_t indexSize = static_cast<std::size_t>(fileSize_ - indexOffset_);
  std::vector<std::uint8_t> index(indexSize);
  file.seekg(indexOffset_);
  if (!file.read(reinterpret_cast<char*>(index.data()), index.size()))
    return Fail(error, "short package index");

  // CStructureSaver's top-level chunk 1 holds the main data. Its inner chunk
  // 1 holds unordered_map<FILE_ID,SFileInfo>. Keys (tag 1, u32) and values
  // (tag 2, two u32s) are interleaved; we materialize values in a host map.
  // CStructureSaver reads the first matching chunk and ignores padding after
  // it. Retail packages may leave zero-filled bytes after the main payload.
  std::size_t cursor = 0;
  Chunk main{};
  if (!NextChunk(index, index.size(), &cursor, &main) || main.id != 1)
    return Fail(error, "missing index main chunk");
  cursor = main.begin;
  Chunk map{};
  if (!NextChunk(index, main.end, &cursor, &map) || map.id != 1)
    return Fail(error, "missing file map");
  bool haveKey = false;
  std::int32_t key = 0;
  for (std::size_t cursor = map.begin; cursor < map.end; ) {
    Chunk chunk;
    if (!NextChunk(index, map.end, &cursor, &chunk))
      return Fail(error, "malformed file map");
    const std::size_t length = chunk.end - chunk.begin;
    if (!haveKey && chunk.id == 1 && length == 4) {
      key = static_cast<std::int32_t>(U32(index.data() + chunk.begin));
      haveKey = true;
    } else if (haveKey && chunk.id == 2 && length == 8) {
      PackageEntry entry{U32(index.data() + chunk.begin),
                         U32(index.data() + chunk.begin + 4)};
      if (entry.offset < 8 || entry.offset > indexOffset_ ||
          entry.length > indexOffset_ - entry.offset ||
          !entries_.emplace(key, entry).second)
        return Fail(error, "invalid or duplicate file entry");
      haveKey = false;
    } else return Fail(error, "unexpected file map field");
  }
  if (haveKey || entries_.empty()) return Fail(error, "incomplete or empty file map");
  path_ = resolvedPath;
  return true;
}

bool PortablePackageIndex::Read(std::int32_t id,
                                std::vector<std::uint8_t>* output,
                                std::string* error) const {
  if (!output || path_.empty()) return Fail(error, "package not open");
  const auto it = entries_.find(id);
  if (it == entries_.end()) return Fail(error, "resource ID not found");
  std::ifstream file(path_, std::ios::binary);
  if (!file) return Fail(error, "cannot reopen package");
  file.seekg(it->second.offset);
  output->resize(it->second.length);
  if (!output->empty() &&
      !file.read(reinterpret_cast<char*>(output->data()), output->size()))
    return Fail(error, "short resource read");
  return true;
}
} // namespace S2FileIO
