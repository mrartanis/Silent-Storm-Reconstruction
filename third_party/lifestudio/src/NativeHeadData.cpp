#include "NativeHeadData.h"
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
}

bool DecodeHeadVertices(const void *bytes, std::size_t size, HeadData *result)
{
  if (!result)
    return false;
  Reader reader(bytes, size);
  HeadData parsed;
  std::uint32_t magic;
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
}
