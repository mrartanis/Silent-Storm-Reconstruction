#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace NativeLifeStudio
{
struct InfluenceRecord
{
  std::uint32_t index;
  float componentA;
  float componentB;
};

struct VertexRecord
{
  std::uint32_t index;
  float sourcePosition[3];
  std::vector<InfluenceRecord> influences;
};

struct ImplicitVertexRecord
{
  std::uint32_t index;
  std::uint32_t type;
  std::uint32_t attribute;
  float sourcePosition[3];
};

struct HeadData
{
  std::uint32_t boneCount = 0;
  std::uint32_t vertexCount = 0;
  std::uint32_t explicitVertexCount = 0;
  std::vector<VertexRecord> vertices;
  std::vector<ImplicitVertexRecord> implicitVertices;
};

// Decode both vertex tables of the original 0xAD5A018D stream. This is the
// first stage of the native decoder, not yet the bone/muscle deformation.
bool DecodeHeadVertices(const void *bytes, std::size_t size, HeadData *result);
}
