#ifndef S2_FILEIO_PORTABLE_MESH_WIRE_H
#define S2_FILEIO_PORTABLE_MESH_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct LoadVertexFields {
  float position[3], normal[3], texcoord[2], tangentU[3], tangentV[3];
  std::uint32_t color;
};

inline bool DecodeLoadVertex(const std::uint8_t* source, std::size_t length,
                             LoadVertexFields* value) {
  if (!source || !value || length != 60) return false;
  LoadVertexFields result{};
  float* fields[] = {&result.position[0], &result.position[1], &result.position[2],
                     &result.normal[0], &result.normal[1], &result.normal[2],
                     &result.texcoord[0], &result.texcoord[1],
                     &result.tangentU[0], &result.tangentU[1], &result.tangentU[2],
                     &result.tangentV[0], &result.tangentV[1], &result.tangentV[2]};
  for (std::size_t i = 0; i < 14; ++i)
    if (!DecodeStructureScalar(source + 4 * i, 4, fields[i])) return false;
  if (!DecodeStructureScalar(source + 56, 4, &result.color)) return false;
  *value = result;
  return true;
}

inline bool EncodeLoadVertex(const LoadVertexFields& value,
                             std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 60) return false;
  const float* fields[] = {&value.position[0], &value.position[1], &value.position[2],
                           &value.normal[0], &value.normal[1], &value.normal[2],
                           &value.texcoord[0], &value.texcoord[1],
                           &value.tangentU[0], &value.tangentU[1], &value.tangentU[2],
                           &value.tangentV[0], &value.tangentV[1], &value.tangentV[2]};
  for (std::size_t i = 0; i < 14; ++i)
    if (!EncodeStructureScalar(*fields[i], destination + 4 * i, 4)) return false;
  return EncodeStructureScalar(value.color, destination + 56, 4);
}

struct LoadVertexWeightFields {
  float weight;
  std::int32_t vertex, bone;
};

inline bool DecodeLoadVertexWeight(const std::uint8_t* source, std::size_t length,
                                   LoadVertexWeightFields* value) {
  if (!source || !value || length != 12) return false;
  LoadVertexWeightFields result{};
  if (!DecodeStructureScalar(source, 4, &result.weight) ||
      !DecodeStructureScalar(source + 4, 4, &result.vertex) ||
      !DecodeStructureScalar(source + 8, 4, &result.bone)) return false;
  *value = result;
  return true;
}

inline bool EncodeLoadVertexWeight(const LoadVertexWeightFields& value,
                                   std::uint8_t* destination, std::size_t length) {
  return destination && length == 12 &&
      EncodeStructureScalar(value.weight, destination, 4) &&
      EncodeStructureScalar(value.vertex, destination + 4, 4) &&
      EncodeStructureScalar(value.bone, destination + 8, 4);
}

} // namespace S2FileIO

#endif
