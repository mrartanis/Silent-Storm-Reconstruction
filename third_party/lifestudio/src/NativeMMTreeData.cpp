#include "NativeMMTreeData.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <unordered_map>
#include <utility>

namespace NativeLifeStudio
{
namespace
{
std::uint32_t ReadU32(const unsigned char *p)
{
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

bool Reject(std::size_t offset, const char *reason)
{
  if (std::getenv("S2_MMTREE_TRACE"))
    std::fprintf(stderr, "MMLF reject at %zu: %s\n", offset, reason);
  return false;
}

bool ParseRecord(const unsigned char *bytes, std::size_t offset,
                 std::size_t limit, unsigned depth, std::size_t *recordCount,
                 std::unordered_map<std::size_t, std::pair<std::uint32_t, std::string>> *definitions,
                 MMTreeOperationRecord *result)
{
  if (depth > 64 || *recordCount >= 100000 || offset > limit || limit - offset < 32)
    return Reject(offset, "header boundary/depth");
  MMTreeOperationRecord record;
  record.offset = offset;
  for (int i = 0; i < 8; ++i)
    record.headerWords[i] = ReadU32(bytes + offset + i * 4);
  record.payloadSize = record.headerWords[4];
  record.preludeSize = record.headerWords[3];
  record.referenceOffset = record.headerWords[5];
  const std::uint32_t nameOffset = record.headerWords[2];
  if (record.payloadSize > limit - offset - 32 ||
      record.preludeSize > record.payloadSize ||
      nameOffset > record.preludeSize ||
      (nameOffset != record.preludeSize && record.preludeSize - nameOffset < 4))
    return Reject(offset, "payload/prelude boundary");
  const std::size_t payload = offset + 32;
  if (nameOffset < record.preludeSize)
  {
    const std::uint32_t nameLength = ReadU32(bytes + payload + nameOffset);
    if (nameLength > record.preludeSize - nameOffset - 4)
      return Reject(offset, "name boundary");
    const char *name = reinterpret_cast<const char *>(bytes + payload + nameOffset + 4);
    for (std::uint32_t i = 0; i < nameLength; ++i)
      if (static_cast<unsigned char>(name[i]) < 32)
        return Reject(offset, "name control byte");
    record.name.assign(name, nameLength);
  }
  if (!record.name.empty())
  {
    if (record.referenceOffset != 0)
      return Reject(offset, "named record has reference");
    record.resolvedName = record.name;
    definitions->emplace(offset, std::make_pair(record.headerWords[0], record.name));
  }
  else
  {
    const auto target = definitions->find(record.referenceOffset);
    if (target == definitions->end() || target->second.first != record.headerWords[0])
      return Reject(offset, "unresolved reference");
    record.resolvedName = target->second.second;
  }
  if (depth && record.payloadSize >= 8)
    record.serializedType = ReadU32(bytes + payload + 4);
  const std::size_t end = payload + record.payloadSize;
  std::size_t childOffset = payload + record.preludeSize;
  ++*recordCount;
  while (childOffset < end)
  {
    MMTreeOperationRecord child;
    if (!ParseRecord(bytes, childOffset, end, depth + 1, recordCount,
                     definitions, &child))
      return false;
    childOffset += 32 + child.payloadSize;
    record.children.push_back(std::move(child));
  }
  if (childOffset != end)
    return false;
  *result = std::move(record);
  return true;
}
}

bool DecodeMMTreeRoot(const void *bytes, std::size_t size, MMTreeRoot *result)
{
  if (!bytes || !result || size < 104 ||
      size > std::numeric_limits<std::uint32_t>::max())
    return false;
  const auto *p = static_cast<const unsigned char *>(bytes);
  if (ReadU32(p) != 0x464C4D4Du || ReadU32(p + 4) != 4 ||
      ReadU32(p + 28) != size - 32 || ReadU32(p + 32) != 1 ||
      ReadU32(p + 48) != size - 64)
    return false;
  MMTreeOperationRecord root;
  std::size_t recordCount = 0;
  std::unordered_map<std::size_t, std::pair<std::uint32_t, std::string>> definitions;
  if (!ParseRecord(p, 32, size, 0, &recordCount, &definitions, &root) ||
      32 + 32 + root.payloadSize != size || root.children.empty())
    return false;
  MMTreeRoot parsed;
  parsed.name = std::move(root.name);
  parsed.preludeSize = root.preludeSize;
  parsed.operations = std::move(root.children);
  *result = std::move(parsed);
  return true;
}
}
