#include "NativeMMTreeData.h"
#include "NativeCurve.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

float ReadFloat(const unsigned char *p)
{
  const std::uint32_t bits = ReadU32(p);
  float value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
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
    if ((record.headerWords[0] == 3 || record.headerWords[0] == 4) &&
        record.preludeSize >= 8)
      record.runtimeType = ReadU32(bytes + payload + record.preludeSize - 8);
    if ((record.headerWords[0] == 3 && record.runtimeType != 0) ||
        (record.headerWords[0] == 4 &&
         (record.runtimeType < 1 || record.runtimeType > 3)))
      return Reject(offset, "unsupported effect runtime type");
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

bool DecodeMMTreeCurve(const void *bytes, std::size_t size,
                       const MMTreeOperationRecord &record, MMTreeCurve *result)
{
  if (!bytes || !result || record.offset > size || size - record.offset < 32 ||
      record.payloadSize > size - record.offset - 32)
    return false;
  const std::uint32_t count = record.serializedType;
  if (count < 2 || count > 64 || record.headerWords[2] != 16 + 8 * count ||
      record.payloadSize < record.headerWords[2])
    return false;
  const auto *p = static_cast<const unsigned char *>(bytes) + record.offset + 32;
  if (ReadU32(p + 4) != count)
    return false;
  MMTreeCurve curve;
  curve.x.reserve(count);
  curve.y.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i)
    curve.x.push_back(ReadFloat(p + 8 + 4 * i));
  for (std::uint32_t i = 0; i < count; ++i)
    curve.y.push_back(ReadFloat(p + 8 + 4 * (count + i)));
  curve.maximum = ReadFloat(p + 8 + 8 * count);
  curve.minimum = ReadFloat(p + 12 + 8 * count);
  if (!std::isfinite(curve.maximum) || !std::isfinite(curve.minimum) ||
      !(curve.minimum < curve.maximum))
    return false;
  for (std::size_t i = 0; i < curve.x.size(); ++i)
    if (!std::isfinite(curve.x[i]) || !std::isfinite(curve.y[i]) ||
        (i && !(curve.x[i] > curve.x[i - 1])))
      return false;
  *result = std::move(curve);
  return true;
}

namespace
{
bool ValidateCurvesInRecord(const void *bytes, std::size_t size,
                            const MMTreeOperationRecord &record)
{
  MMTreeCurve curve;
  if (!DecodeMMTreeCurve(bytes, size, record, &curve)) return false;
  for (const auto &child : record.children)
    if (!ValidateCurvesInRecord(bytes, size, child)) return false;
  return true;
}
}

bool ValidateMMTreeCurves(const void *bytes, std::size_t size,
                          const MMTreeRoot &root)
{
  if (!bytes) return false;
  for (const auto &operation : root.operations)
    if (!ValidateCurvesInRecord(bytes, size, operation)) return false;
  return true;
}

namespace
{
void IndexRecords(const MMTreeOperationRecord &record,
                  std::unordered_map<std::size_t, const MMTreeOperationRecord *> *offsets,
                  std::unordered_map<std::string, const MMTreeOperationRecord *> *macros)
{
  offsets->emplace(record.offset, &record);
  if (record.headerWords[0] == 1 && !record.name.empty())
    macros->emplace(record.name, &record);
  for (const auto &child : record.children)
    IndexRecords(child, offsets, macros);
}

bool EvaluateChildren(const void *bytes, std::size_t size,
                      const MMTreeOperationRecord &definition, float expression,
                      const std::unordered_map<std::size_t, const MMTreeOperationRecord *> &offsets,
                      std::vector<MMTreeEffectSample> *samples, unsigned depth)
{
  if (depth > 64) return false;
  for (const auto &operation : definition.children)
  {
    MMTreeCurve curve;
    float transformed = 0.0f;
    if (!DecodeMMTreeCurve(bytes, size, operation, &curve) ||
        !EvaluateHermiteCurve(curve.x, curve.y, expression, &transformed))
      return false;
    if (operation.headerWords[0] == 1)
    {
      const MMTreeOperationRecord *target = &operation;
      if (operation.referenceOffset)
      {
        const auto found = offsets.find(operation.referenceOffset);
        if (found == offsets.end()) return false;
        target = found->second;
      }
      if (!EvaluateChildren(bytes, size, *target, transformed,
                            offsets, samples, depth + 1))
        return false;
    }
    else if (operation.headerWords[0] == 3 || operation.headerWords[0] == 4)
    {
      const MMTreeOperationRecord *target = &operation;
      if (operation.referenceOffset)
      {
        const auto found = offsets.find(operation.referenceOffset);
        if (found == offsets.end()) return false;
        target = found->second;
      }
      samples->push_back({operation.offset, operation.headerWords[0], operation.headerWords[1],
                          operation.resolvedName, transformed, expression, target->runtimeType});
    }
    else
      return false;
  }
  return true;
}
}

bool EvaluateMMTreeMacro(const void *bytes, std::size_t size,
                         const MMTreeRoot &root, const std::string &macroName,
                         float expression, std::vector<MMTreeEffectSample> *result)
{
  if (!bytes || !result || !std::isfinite(expression)) return false;
  std::unordered_map<std::size_t, const MMTreeOperationRecord *> offsets;
  std::unordered_map<std::string, const MMTreeOperationRecord *> macros;
  for (const auto &operation : root.operations)
    IndexRecords(operation, &offsets, &macros);
  const auto found = macros.find(macroName);
  if (found == macros.end()) return false;
  std::vector<MMTreeEffectSample> samples;
  if (!EvaluateChildren(bytes, size, *found->second, expression,
                        offsets, &samples, 0))
    return false;
  *result = std::move(samples);
  return true;
}
}
