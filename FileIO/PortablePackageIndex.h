#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace S2FileIO {

// Resolve Windows-style game resource paths on a case-sensitive host.
// An ambiguous case-insensitive component is rejected.
bool ResolveGameResourcePath(const std::string& requested, std::string* resolved);

// The .res wire format uses 32-bit little-endian offsets, lengths and IDs.
// These are values, not serialized C++ pointers or container nodes.
struct PackageEntry {
  std::uint32_t offset = 0;
  std::uint32_t length = 0;
};
static_assert(sizeof(PackageEntry) == 8, "package entry must be two 32-bit values");

class PortablePackageIndex {
public:
  bool Open(const std::string& path, std::string* error = nullptr);
  bool Read(std::int32_t id, std::vector<std::uint8_t>* output,
            std::string* error = nullptr) const;
  const std::map<std::int32_t, PackageEntry>& Entries() const { return entries_; }
  std::uint32_t IndexOffset() const { return indexOffset_; }
  std::uint64_t FileSize() const { return fileSize_; }
  const std::string& ResolvedPath() const { return path_; }

private:
  std::string path_;
  std::map<std::int32_t, PackageEntry> entries_;
  std::uint32_t indexOffset_ = 0;
  std::uint64_t fileSize_ = 0;
};

} // namespace S2FileIO
