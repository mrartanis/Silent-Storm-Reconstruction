#ifndef S2_FILEIO_PORTABLE_RENDER_STATE_WIRE_H
#define S2_FILEIO_PORTABLE_RENDER_STATE_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

// Explicit float32 fields, split because the shared scalar helper bounds each
// operation to sixteen fields. No host vector or renderer struct is copied.
template<std::size_t N>
inline bool DecodeRenderFloatFields(const std::uint8_t* source,
                                    std::size_t length, float* values) {
  if (!source || !values || N == 0 || N > 24 || length != N * 4) return false;
  for (std::size_t offset = 0; offset < N; offset += 16) {
    const std::size_t count = N - offset < 16 ? N - offset : 16;
    if (!DecodeStructureFloatFields(source + offset * 4, count * 4,
                                    values + offset, count)) return false;
  }
  return true;
}

template<std::size_t N>
inline bool EncodeRenderFloatFields(const float* values,
                                    std::uint8_t* destination,
                                    std::size_t length) {
  if (!values || !destination || N == 0 || N > 24 || length != N * 4)
    return false;
  for (std::size_t offset = 0; offset < N; offset += 16) {
    const std::size_t count = N - offset < 16 ? N - offset : 16;
    if (!EncodeStructureFloatFields(values + offset, count,
                                    destination + offset * 4, count * 4))
      return false;
  }
  return true;
}

struct RenderDirectionalFields {
  float color[3];
  float direction[3];
  bool rendered;
};

inline bool DecodeRenderDirectional(const std::uint8_t* source,
                                    std::size_t length,
                                    RenderDirectionalFields* value) {
  if (!source || !value || length != 28) return false;
  float fields[6];
  if (!DecodeRenderFloatFields<6>(source, 24, fields)) return false;
  for (int i = 0; i < 3; ++i) {
    value->color[i] = fields[i];
    value->direction[i] = fields[i + 3];
  }
  value->rendered = source[24] != 0;
  return true;
}

inline bool EncodeRenderDirectional(const RenderDirectionalFields& value,
                                    std::uint8_t* destination,
                                    std::size_t length) {
  if (!destination || length != 28) return false;
  const float fields[6] = {value.color[0], value.color[1], value.color[2],
                           value.direction[0], value.direction[1],
                           value.direction[2]};
  if (!EncodeRenderFloatFields<6>(fields, destination, 24)) return false;
  destination[24] = value.rendered ? 1 : 0;
  destination[25] = destination[26] = destination[27] = 0;
  return true;
}

} // namespace S2FileIO

#endif
