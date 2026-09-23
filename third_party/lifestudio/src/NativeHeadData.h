#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
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
  std::array<float, 5> falloffX{};
  std::array<float, 5> falloffY{};
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

struct NeckZoneRecord
{
  std::vector<std::uint8_t> vertexMask;
  std::uint32_t upperVertex = 0;
  std::uint32_t lowerVertex = 0;
};

struct HeadData
{
  std::uint32_t muscleCount = 0;
  std::uint32_t boneCount = 0;
  std::uint32_t vertexCount = 0;
  std::uint32_t explicitVertexCount = 0;
  bool hasNeckAppendix = false; // saved two-segment head's Neck_Zone/Upper/Lower data
  std::array<NeckZoneRecord, 8> neckZones{};
  std::vector<MuscleRecord> muscles;
  std::vector<VertexRecord> vertices;
  std::vector<BoneRecord> bones;
  std::vector<ImplicitVertexRecord> implicitVertices;
};

// Decode named muscles, weighted vertices, bones and implicit vertices of the
// original 0xAD5A018D resource stream or the 0x37D30DC0 IAnimator::Save
// stream. Animation evaluation is performed by the x64 bridge, not here.
bool DecodeHeadVertices(const void *bytes, std::size_t size, HeadData *result);
// Serialize the compact IAnimator::Save representation used by committed
// game heads. Unlike a raw original resource, this can be loaded directly.
bool EncodeSavedHead(const HeadData &head, std::vector<char> *result);
}
