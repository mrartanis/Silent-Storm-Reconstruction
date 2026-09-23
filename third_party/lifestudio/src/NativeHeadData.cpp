#include "NativeHeadData.h"
#include "NativeCurve.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace NativeLifeStudio
{
namespace
{
class Reader
{
  const unsigned char *data;
  std::size_t size;
public:
  Reader(const void *bytes, std::size_t length): data(static_cast<const unsigned char *>(bytes)), size(length) {}
  bool U32(std::size_t offset, std::uint32_t *value) const
  {
    if (!data || offset > size || size - offset < 4)
      return false;
    const unsigned char *p = data + offset;
    *value = std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
             (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
    return true;
  }
  bool F32(std::size_t offset, float *value) const
  {
    std::uint32_t bits;
    if (!U32(offset, &bits))
      return false;
    std::memcpy(value, &bits, sizeof(bits));
    return std::isfinite(*value);
  }
  bool Has(std::size_t offset, std::size_t length) const
  {
    return offset <= size && length <= size - offset;
  }
};

bool ReadFalloff(const Reader &reader, std::size_t xOffset,
                 std::size_t yOffset, MuscleRecord *muscle)
{
  muscle->falloffX[0] = 0.0f;
  for (int i = 1; i < 5; ++i)
    if (!reader.F32(xOffset + std::size_t(i - 1) * 4, &muscle->falloffX[i]))
      return false;
  for (int i = 0; i < 5; ++i)
    if (!reader.F32(yOffset + std::size_t(i) * 4, &muscle->falloffY[i]))
      return false;
  for (int i = 1; i < 5; ++i)
    if (!(muscle->falloffX[i] > muscle->falloffX[i - 1]))
      return false;
  return true;
}

bool ReconstructFalloff(const MuscleRecord &muscle, const VertexRecord &vertex,
                        float projection, float *weight)
{
  const float clamped = std::max(0.0f, std::min(1.0f, projection));
  float squared = 0.0f;
  for (int axis = 0; axis < 3; ++axis)
  {
    const float nearest = muscle.pointA[axis] +
                          clamped * (muscle.pointB[axis] - muscle.pointA[axis]);
    const float difference = vertex.sourcePosition[axis] - nearest;
    squared += difference * difference;
  }
  const float distance = std::sqrt(squared);
  if (distance > muscle.falloffX.back())
  {
    *weight = 0.0f;
    return true;
  }
  const std::vector<float> x(muscle.falloffX.begin(), muscle.falloffX.end());
  const std::vector<float> y(muscle.falloffY.begin(), muscle.falloffY.end());
  if (!EvaluateHermiteCurve(x, y, distance, weight)) return false;
  *weight = std::max(0.0f, std::min(1.0f, *weight));
  return true;
}

bool DecodeNeckAppendix(const Reader &reader, const unsigned char *raw,
                        std::size_t offset, std::size_t size, HeadData *head)
{
  const std::size_t maskBytes = (std::size_t(head->vertexCount) + 7) / 8;
  if (maskBytes > 125000 || size - offset != 8 * (20 + maskBytes) + 16 * 25)
    return false;
  std::size_t cursor = offset;
  for (int zone = 0; zone < 8; ++zone)
  {
    const std::string name = "Neck_Zone_" + std::to_string(zone);
    std::uint32_t vertexCount, marker;
    if (!reader.Has(cursor, 20 + maskBytes) || raw[cursor] != name.size() ||
        std::memcmp(raw + cursor + 1, name.data(), name.size()) != 0 ||
        !reader.U32(cursor + 12, &vertexCount) || vertexCount != head->vertexCount ||
        !reader.U32(cursor + 16, &marker) || marker != 0xffffffffu)
      return false;
    head->neckZones[zone].vertexMask.assign(raw + cursor + 20,
                                             raw + cursor + 20 + maskBytes);
    cursor += 20 + maskBytes;
  }
  for (int section = 0; section < 2; ++section)
    for (int zone = 0; zone < 8; ++zone)
    {
      const std::string name = std::string(section == 0 ? "Neck_Upper_" : "Neck_Lower_") +
                               std::to_string(zone);
      std::uint32_t vertexCount, marker, vertex;
      if (!reader.Has(cursor, 25) || raw[cursor] != name.size() ||
          std::memcmp(raw + cursor + 1, name.data(), name.size()) != 0 ||
          !reader.U32(cursor + 13, &vertexCount) || vertexCount != head->vertexCount ||
          !reader.U32(cursor + 17, &marker) || marker != 1 ||
          !reader.U32(cursor + 21, &vertex) || vertex >= head->vertexCount)
        return false;
      if (section == 0) head->neckZones[zone].upperVertex = vertex;
      else head->neckZones[zone].lowerVertex = vertex;
      cursor += 25;
    }
  return cursor == size;
}

bool DecodeSavedHeadVertices(const void *bytes, std::size_t size, HeadData *result)
{
  Reader reader(bytes, size);
  HeadData parsed;
  if (size < 36 || !reader.U32(4, &parsed.muscleCount) ||
      !reader.U32(8, &parsed.vertexCount) ||
      !reader.U32(12, &parsed.explicitVertexCount) ||
      !reader.U32(16, &parsed.boneCount) ||
      parsed.muscleCount > 10000 || parsed.boneCount > 10000 ||
      parsed.vertexCount > 1000000 || parsed.explicitVertexCount > parsed.vertexCount)
    return false;
  const unsigned char *raw = static_cast<const unsigned char *>(bytes);
  std::size_t cursor = 32;
  parsed.muscles.reserve(parsed.muscleCount);
  for (std::uint32_t i = 0; i < parsed.muscleCount; ++i)
  {
    std::uint32_t type;
    if (!reader.Has(cursor, 5) || !reader.U32(cursor, &type)) return false;
    const std::size_t length = raw[cursor + 4];
    if (!length || length > 64 || !reader.Has(cursor + 5, length + 108)) return false;
    MuscleRecord muscle;
    muscle.type = type;
    muscle.name.assign(reinterpret_cast<const char *>(raw + cursor + 5), length);
    for (char c : muscle.name) if (c < 32 || c > 126) return false;
    const std::size_t payload = cursor + 5 + length;
    for (int axis = 0; axis < 3; ++axis)
      if (!reader.F32(payload + axis * 4, &muscle.pointA[axis]) ||
          !reader.F32(payload + 12 + axis * 4, &muscle.pointB[axis]))
        return false;
    if (!ReadFalloff(reader, payload + 32, payload + 68, &muscle))
      return false;
    parsed.muscles.push_back(std::move(muscle));
    cursor = payload + 108;
  }
  // This field is 5 in the observed original DLL Save streams. The explicit
  // vertex records follow it in vertex-index order. A head with no explicit
  // vertices uses section 1 instead and goes straight to its implicit list.
  std::uint32_t vertexSection;
  if (!reader.U32(cursor, &vertexSection) ||
      (vertexSection != 5 && !(vertexSection == 1 && parsed.explicitVertexCount == 0)))
    return false;
  cursor += 4;
  std::vector<bool> seen(parsed.vertexCount, false);
  parsed.vertices.reserve(parsed.explicitVertexCount);
  for (std::uint32_t i = 0; i < parsed.explicitVertexCount; ++i)
  {
    VertexRecord vertex;
    std::uint32_t influenceCount;
    if (!reader.Has(cursor, 20) || !reader.U32(cursor, &vertex.index) ||
        vertex.index >= parsed.vertexCount || seen[vertex.index] ||
        !reader.F32(cursor + 4, &vertex.sourcePosition[0]) ||
        !reader.F32(cursor + 8, &vertex.sourcePosition[1]) ||
        !reader.F32(cursor + 12, &vertex.sourcePosition[2]) ||
        !reader.U32(cursor + 16, &influenceCount) ||
        influenceCount > (size - cursor - 20) / 8)
      return false;
    seen[vertex.index] = true;
    cursor += 20;
    vertex.influences.reserve(influenceCount);
    for (std::uint32_t n = 0; n < influenceCount; ++n, cursor += 8)
    {
      InfluenceRecord influence;
      if (!reader.U32(cursor, &influence.muscleIndex) ||
          influence.muscleIndex >= parsed.muscleCount ||
          !reader.F32(cursor + 4, &influence.componentA))
        return false;
      if (!ReconstructFalloff(parsed.muscles[influence.muscleIndex], vertex,
                              influence.componentA, &influence.componentB))
        return false;
      vertex.influences.push_back(influence);
    }
    parsed.vertices.push_back(std::move(vertex));
  }
  parsed.bones.reserve(parsed.boneCount);
  for (std::uint32_t i = 0; i < parsed.boneCount; ++i)
  {
    if (!reader.Has(cursor, 1)) return false;
    const std::size_t length = raw[cursor];
    if (!length || length > 64 || !reader.Has(cursor + 1, length + 108)) return false;
    BoneRecord bone;
    bone.name.assign(reinterpret_cast<const char *>(raw + cursor + 1), length);
    for (char c : bone.name) if (c < 32 || c > 126) return false;
    const std::size_t payload = cursor + 1 + length;
    std::uint32_t attachedCount;
    if (!reader.U32(payload, &attachedCount) ||
        attachedCount > parsed.muscleCount ||
        !reader.Has(payload + 108, std::size_t(attachedCount) * 4))
      return false;
    for (int n = 0; n < 12; ++n)
      if (!reader.F32(payload + 8 + n * 4, &bone.matrixA[n]) ||
          !reader.F32(payload + 56 + n * 4, &bone.matrixB[n]))
        return false;
    for (std::uint32_t n = 0; n < attachedCount; ++n)
    {
      std::uint32_t index;
      if (!reader.U32(payload + 108 + std::size_t(n) * 4, &index) ||
          index >= parsed.muscleCount) return false;
      bone.muscleIndices.push_back(index);
    }
    cursor = payload + 108 + std::size_t(attachedCount) * 4;
    parsed.bones.push_back(std::move(bone));
  }
  const std::size_t implicitCount = parsed.vertexCount - parsed.explicitVertexCount;
  if (cursor > size || implicitCount > (size - cursor) / 16) return false;
  const std::size_t trailingOffset = cursor + implicitCount * 16;
  // The game's saved two-segment head 11 carries eight vertex-zone masks
  // plus eight upper/lower landmark pairs after the standard vertex table.
  if (trailingOffset != size &&
      !DecodeNeckAppendix(reader, raw, trailingOffset, size, &parsed))
    return false;
  parsed.hasNeckAppendix = trailingOffset != size;
  parsed.implicitVertices.reserve(implicitCount);
  for (std::size_t n = 0; n < implicitCount; ++n, cursor += 16)
  {
    ImplicitVertexRecord vertex;
    if (!reader.U32(cursor, &vertex.index) || vertex.index >= parsed.vertexCount ||
        seen[vertex.index] ||
        !reader.F32(cursor + 4, &vertex.sourcePosition[0]) ||
        !reader.F32(cursor + 8, &vertex.sourcePosition[1]) ||
        !reader.F32(cursor + 12, &vertex.sourcePosition[2]))
      return false;
    seen[vertex.index] = true;
    parsed.implicitVertices.push_back(vertex);
  }
  for (bool found : seen) if (!found) return false;
  *result = std::move(parsed);
  return true;
}
}

bool DecodeHeadVertices(const void *bytes, std::size_t size, HeadData *result)
{
  if (!result)
    return false;
  Reader reader(bytes, size);
  HeadData parsed;
  std::uint32_t magic;
  if (reader.U32(0, &magic) && magic == 0x37D30DC0u)
    return DecodeSavedHeadVertices(bytes, size, result);
  if (size < 28 || !reader.U32(0, &magic) || magic != 0xAD5A018Du ||
      !reader.U32(4, &parsed.muscleCount) ||
      !reader.U32(8, &parsed.vertexCount) ||
      !reader.U32(12, &parsed.explicitVertexCount) ||
      !reader.U32(16, &parsed.boneCount) ||
      parsed.muscleCount > 10000 || parsed.boneCount > 10000 || parsed.vertexCount > 1000000 ||
      parsed.explicitVertexCount > parsed.vertexCount ||
      parsed.muscleCount > (size - 28) / 172)
    return false;
  parsed.muscles.reserve(parsed.muscleCount);
  const unsigned char *raw = static_cast<const unsigned char *>(bytes);
  for (std::uint32_t i = 0; i < parsed.muscleCount; ++i)
  {
    const std::size_t offset = 24 + std::size_t(i) * 172;
    MuscleRecord muscle;
    if (!reader.U32(offset, &muscle.type))
      return false;
    const unsigned char *name = raw + offset + 4;
    const unsigned char *end = static_cast<const unsigned char *>(std::memchr(name, 0, 64));
    if (!end || end == name)
      return false;
    for (const unsigned char *p = name; p != end; ++p)
      if (*p < 32 || *p > 126)
        return false;
    muscle.name.assign(reinterpret_cast<const char *>(name), end - name);
    for (int axis = 0; axis < 3; ++axis)
      if (!reader.F32(offset + 68 + axis * 4, &muscle.pointA[axis]) ||
          !reader.F32(offset + 80 + axis * 4, &muscle.pointB[axis]))
        return false;
    if (!ReadFalloff(reader, offset + 96, offset + 132, &muscle))
      return false;
    parsed.muscles.push_back(std::move(muscle));
  }
  std::size_t cursor = 28 + std::size_t(parsed.muscleCount) * 172;
  parsed.vertices.reserve(parsed.explicitVertexCount);
  std::vector<bool> seen(parsed.vertexCount, false);
  for (std::uint32_t n = 0; n < parsed.explicitVertexCount; ++n)
  {
    VertexRecord vertex;
    std::uint32_t influenceCount;
    if (!reader.Has(cursor, 28) ||
        !reader.U32(cursor, &vertex.index) || vertex.index >= parsed.vertexCount ||
        seen[vertex.index] ||
        !reader.F32(cursor + 12, &vertex.sourcePosition[0]) ||
        !reader.F32(cursor + 16, &vertex.sourcePosition[1]) ||
        !reader.F32(cursor + 20, &vertex.sourcePosition[2]) ||
        !reader.U32(cursor + 24, &influenceCount) ||
        influenceCount > 1000000 ||
        !reader.Has(cursor + 28, std::size_t(influenceCount) * 12))
      return false;
    seen[vertex.index] = true;
    cursor += 28;
    vertex.influences.reserve(influenceCount);
    for (std::uint32_t i = 0; i < influenceCount; ++i, cursor += 12)
    {
      InfluenceRecord influence;
      if (!reader.U32(cursor, &influence.muscleIndex) || influence.muscleIndex >= parsed.muscleCount ||
          !reader.F32(cursor + 4, &influence.componentA) ||
          !reader.F32(cursor + 8, &influence.componentB))
        return false;
      vertex.influences.push_back(influence);
    }
    parsed.vertices.push_back(std::move(vertex));
  }
  const std::size_t implicitCount = parsed.vertexCount - parsed.explicitVertexCount;
  if (implicitCount > size / 24)
    return false;
  const std::size_t implicitStart = size - implicitCount * 24;
  if (implicitStart < cursor)
    return false;
  parsed.bones.reserve(parsed.boneCount);
  for (std::uint32_t i = 0; i < parsed.boneCount; ++i)
  {
    if (!reader.Has(cursor, 164))
      return false;
    BoneRecord bone;
    const unsigned char *name = raw + cursor;
    const unsigned char *end = static_cast<const unsigned char *>(std::memchr(name, 0, 64));
    if (!end || end == name)
      return false;
    for (const unsigned char *p = name; p != end; ++p)
      if (*p < 32 || *p > 126)
        return false;
    bone.name.assign(reinterpret_cast<const char *>(name), end - name);
    std::uint32_t attachedCount;
    if (!reader.U32(cursor + 64, &attachedCount) || attachedCount > parsed.muscleCount ||
        !reader.Has(cursor + 164, std::size_t(attachedCount) * 4))
      return false;
    for (int n = 0; n < 12; ++n)
      if (!reader.F32(cursor + 68 + n * 4, &bone.matrixA[n]) ||
          !reader.F32(cursor + 116 + n * 4, &bone.matrixB[n]))
        return false;
    bone.muscleIndices.reserve(attachedCount);
    for (std::uint32_t n = 0; n < attachedCount; ++n)
    {
      std::uint32_t index;
      if (!reader.U32(cursor + 164 + std::size_t(n) * 4, &index) ||
          index >= parsed.muscleCount)
        return false;
      bone.muscleIndices.push_back(index);
    }
    cursor += 164 + std::size_t(attachedCount) * 4;
    if (cursor > implicitStart)
      return false;
    parsed.bones.push_back(std::move(bone));
  }
  if (cursor != implicitStart)
    return false;
  parsed.implicitVertices.reserve(implicitCount);
  for (std::size_t n = 0; n < implicitCount; ++n)
  {
    const std::size_t offset = implicitStart + n * 24;
    ImplicitVertexRecord vertex;
    if (!reader.U32(offset, &vertex.index) || vertex.index >= parsed.vertexCount ||
        seen[vertex.index] ||
        !reader.U32(offset + 4, &vertex.type) ||
        !reader.U32(offset + 8, &vertex.attribute) ||
        !reader.F32(offset + 12, &vertex.sourcePosition[0]) ||
        !reader.F32(offset + 16, &vertex.sourcePosition[1]) ||
        !reader.F32(offset + 20, &vertex.sourcePosition[2]))
      return false;
    seen[vertex.index] = true;
    parsed.implicitVertices.push_back(vertex);
  }
  for (bool found : seen)
    if (!found)
      return false;
  *result = std::move(parsed);
  return true;
}

namespace
{
void StoreU32(char *bytes, std::uint32_t value)
{
  for (int i = 0; i < 4; ++i)
    bytes[i] = static_cast<char>(value >> (i * 8));
}

void StoreF32(char *bytes, float value)
{
  std::uint32_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  StoreU32(bytes, bits);
}

void AppendU32(std::vector<char> *bytes, std::uint32_t value)
{
  char word[4];
  StoreU32(word, value);
  bytes->insert(bytes->end(), word, word + 4);
}

void AppendF32(std::vector<char> *bytes, float value)
{
  char word[4];
  StoreF32(word, value);
  bytes->insert(bytes->end(), word, word + 4);
}

bool ValidName(const std::string &name)
{
  if (name.empty() || name.size() > 64) return false;
  for (unsigned char ch : name)
    if (ch < 32 || ch > 126) return false;
  return true;
}
}

bool EncodeSavedHead(const HeadData &head, std::vector<char> *result)
{
  if (!result || head.hasNeckAppendix || head.muscleCount != head.muscles.size() ||
      head.boneCount != head.bones.size() ||
      head.explicitVertexCount != head.vertices.size() ||
      head.vertices.size() + head.implicitVertices.size() != head.vertexCount ||
      head.muscleCount > 10000 || head.boneCount > 10000 ||
      head.vertexCount > 1000000) return false;
  std::vector<char> bytes;
  AppendU32(&bytes, 0x37D30DC0u);
  AppendU32(&bytes, head.muscleCount);
  AppendU32(&bytes, head.vertexCount);
  AppendU32(&bytes, head.explicitVertexCount);
  AppendU32(&bytes, head.boneCount);
  bytes.insert(bytes.end(), 12, 0);
  for (const auto &muscle : head.muscles)
  {
    if (!ValidName(muscle.name)) return false;
    AppendU32(&bytes, muscle.type);
    bytes.push_back(static_cast<char>(muscle.name.size()));
    bytes.insert(bytes.end(), muscle.name.begin(), muscle.name.end());
    std::array<char, 108> payload{};
    for (int axis = 0; axis < 3; ++axis)
    {
      StoreF32(payload.data() + axis * 4, muscle.pointA[axis]);
      StoreF32(payload.data() + 12 + axis * 4, muscle.pointB[axis]);
    }
    for (int knot = 1; knot < 5; ++knot)
      StoreF32(payload.data() + 32 + (knot - 1) * 4, muscle.falloffX[knot]);
    for (int knot = 0; knot < 5; ++knot)
      StoreF32(payload.data() + 68 + knot * 4, muscle.falloffY[knot]);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
  }
  AppendU32(&bytes, 5);
  for (const auto &vertex : head.vertices)
  {
    if (vertex.index >= head.vertexCount ||
        vertex.influences.size() > head.muscleCount) return false;
    AppendU32(&bytes, vertex.index);
    for (float coordinate : vertex.sourcePosition) AppendF32(&bytes, coordinate);
    AppendU32(&bytes, static_cast<std::uint32_t>(vertex.influences.size()));
    for (const auto &influence : vertex.influences)
    {
      if (influence.muscleIndex >= head.muscleCount) return false;
      AppendU32(&bytes, influence.muscleIndex);
      AppendF32(&bytes, influence.componentA);
    }
  }
  for (const auto &bone : head.bones)
  {
    if (!ValidName(bone.name) || bone.muscleIndices.size() > head.muscleCount)
      return false;
    bytes.push_back(static_cast<char>(bone.name.size()));
    bytes.insert(bytes.end(), bone.name.begin(), bone.name.end());
    std::array<char, 108> payload{};
    StoreU32(payload.data(), static_cast<std::uint32_t>(bone.muscleIndices.size()));
    StoreU32(payload.data() + 4, 0xFFFFFFFFu);
    for (int element = 0; element < 12; ++element)
    {
      StoreF32(payload.data() + 8 + element * 4, bone.matrixA[element]);
      StoreF32(payload.data() + 56 + element * 4, bone.matrixB[element]);
    }
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    for (std::uint32_t index : bone.muscleIndices)
    {
      if (index >= head.muscleCount) return false;
      AppendU32(&bytes, index);
    }
  }
  for (const auto &vertex : head.implicitVertices)
  {
    if (vertex.index >= head.vertexCount) return false;
    AppendU32(&bytes, vertex.index);
    for (float coordinate : vertex.sourcePosition) AppendF32(&bytes, coordinate);
  }
  HeadData decoded;
  if (!DecodeHeadVertices(bytes.data(), bytes.size(), &decoded)) return false;
  *result = std::move(bytes);
  return true;
}
}
