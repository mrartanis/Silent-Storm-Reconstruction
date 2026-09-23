#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NativeLifeStudio
{
struct MuscleRecord
{
  std::uint32_t type;
  std::string name;
  float pointA[3];
  float pointB[3];
};

struct BoneRecord
{
  std::string name;
  float matrixA[12];
  float matrixB[12];
  std::vector<std::uint32_t> muscleIndices;
};

struct InfluenceRecord
{
  std::uint32_t muscleIndex;
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
  std::uint32_t muscleCount = 0;
  std::uint32_t boneCount = 0;
  std::uint32_t vertexCount = 0;
  std::uint32_t explicitVertexCount = 0;
  std::vector<MuscleRecord> muscles;
  std::vector<VertexRecord> vertices;
  std::vector<BoneRecord> bones;
  std::vector<ImplicitVertexRecord> implicitVertices;
};

// Decode named muscles, weighted vertices, bones and implicit vertices of the
// original 0xAD5A018D stream. Animation evaluation is performed by the
// partial x64 bridge, not by this decoder.
bool DecodeHeadVertices(const void *bytes, std::size_t size, HeadData *result);
}
