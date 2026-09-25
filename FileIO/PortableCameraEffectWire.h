#ifndef S2_FILEIO_PORTABLE_CAMERA_EFFECT_WIRE_H
#define S2_FILEIO_PORTABLE_CAMERA_EFFECT_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

struct CameraSloMoFields {
  std::int32_t ratio;
  std::int32_t enabled;
  std::uint32_t maxDuration;
};

struct CameraFOVEffectFields {
  std::int32_t ratio;
  std::int32_t enabled;
  std::int32_t springEnabled;
  std::uint32_t maxDuration;
  float fov;
  float placement[8];
  float roll;
};

inline bool DecodeCameraSloMo(const std::uint8_t* source,
                              std::size_t length,
                              CameraSloMoFields* value) {
  return source && value && length == 12 &&
         DecodeStructureScalar(source, 4, &value->ratio) &&
         DecodeStructureScalar(source + 4, 4, &value->enabled) &&
         DecodeStructureScalar(source + 8, 4, &value->maxDuration);
}

inline bool EncodeCameraSloMo(const CameraSloMoFields& value,
                              std::uint8_t* destination,
                              std::size_t length) {
  return destination && length == 12 &&
         EncodeStructureScalar(value.ratio, destination, 4) &&
         EncodeStructureScalar(value.enabled, destination + 4, 4) &&
         EncodeStructureScalar(value.maxDuration, destination + 8, 4);
}

inline bool DecodeCameraFOVEffect(const std::uint8_t* source,
                                  std::size_t length,
                                  CameraFOVEffectFields* value) {
  return source && value && length == 56 &&
         DecodeStructureScalar(source, 4, &value->ratio) &&
         DecodeStructureScalar(source + 4, 4, &value->enabled) &&
         DecodeStructureScalar(source + 8, 4, &value->springEnabled) &&
         DecodeStructureScalar(source + 12, 4, &value->maxDuration) &&
         DecodeStructureScalar(source + 16, 4, &value->fov) &&
         DecodeStructureCameraPos(source + 20, 32, value->placement) &&
         DecodeStructureScalar(source + 52, 4, &value->roll);
}

inline bool EncodeCameraFOVEffect(const CameraFOVEffectFields& value,
                                  std::uint8_t* destination,
                                  std::size_t length) {
  return destination && length == 56 &&
         EncodeStructureScalar(value.ratio, destination, 4) &&
         EncodeStructureScalar(value.enabled, destination + 4, 4) &&
         EncodeStructureScalar(value.springEnabled, destination + 8, 4) &&
         EncodeStructureScalar(value.maxDuration, destination + 12, 4) &&
         EncodeStructureScalar(value.fov, destination + 16, 4) &&
         EncodeStructureCameraPos(value.placement, destination + 20, 32) &&
         EncodeStructureScalar(value.roll, destination + 52, 4);
}

} // namespace S2FileIO

#endif
