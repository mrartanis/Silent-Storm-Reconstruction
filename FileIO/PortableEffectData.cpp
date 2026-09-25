#include "PortableEffectData.h"
#include "PortableStructureChunks.h"

#include <utility>

namespace S2FileIO {
namespace {
bool Fail(std::string* error, const char* message) {
  if (error) *error = message;
  return false;
}

struct Track {
  std::int16_t count = 0;
  std::uint32_t offset = 0;
};

bool ReadTrack(const std::uint8_t* payload, std::size_t length,
               std::size_t offset, Track* track) {
  return payload && track && offset <= length && length - offset >= 6 &&
         DecodeStructureScalar(payload + offset, 2, &track->count) &&
         DecodeStructureScalar(payload + offset + 2, 4, &track->offset);
}

bool ReadValue(const std::uint8_t* bytes, EffectVec3* value) {
  float fields[3];
  if (!DecodeStructureFloatFields(bytes, 12, fields, 3)) return false;
  *value = {fields[0], fields[1], fields[2]};
  return true;
}
bool ReadValue(const std::uint8_t* bytes, EffectVec2* value) {
  float fields[2];
  if (!DecodeStructureFloatFields(bytes, 8, fields, 2)) return false;
  *value = {fields[0], fields[1]};
  return true;
}
bool ReadValue(const std::uint8_t* bytes, float* value) {
  return DecodeStructureScalar(bytes, 4, value);
}
bool ReadValue(const std::uint8_t* bytes, std::uint32_t* value) {
  return DecodeStructureScalar(bytes, 4, value);
}
bool ReadValue(const std::uint8_t* bytes, std::int16_t* value) {
  return DecodeStructureScalar(bytes, 2, value);
}

template<class T>
bool ReadKeys(const std::uint8_t* payload, std::size_t length,
              const Track& track, std::size_t stride,
              std::vector<EffectKey<T>>* result) {
  if (!result || track.count < 0 || track.offset > length ||
      static_cast<std::size_t>(track.count) >
          (length - track.offset) / stride) return false;
  std::vector<EffectKey<T>> decoded(static_cast<std::size_t>(track.count));
  for (std::size_t i = 0; i < decoded.size(); ++i) {
    const std::uint8_t* key = payload + track.offset + i * stride;
    if (!DecodeStructureScalar(key, 2, &decoded[i].frame) ||
        !ReadValue(key + 2, &decoded[i].value)) return false;
  }
  *result = std::move(decoded);
  return true;
}
} // namespace

bool DecodeEffectData(const std::uint8_t* bytes, std::size_t length,
                      EffectData* output, std::string* error) {
  if (!bytes || !output || length < 16)
    return Fail(error, "effect file too short");
  std::uint32_t payloadSize = 0;
  if (!DecodeStructureScalar(bytes, 4, &payloadSize) ||
      payloadSize < 12 || payloadSize > length - 4)
    return Fail(error, "invalid effect payload size");
  const std::uint8_t* payload = bytes + 4;
  std::int32_t count = 0;
  EffectData decoded;
  decoded.payloadSize = payloadSize;
  if (!DecodeStructureScalar(payload, 4, &decoded.endTime) ||
      !DecodeStructureScalar(payload + 4, 4, &decoded.frameRate) ||
      !DecodeStructureScalar(payload + 8, 4, &count) || count < 0 ||
      static_cast<std::size_t>(count) > (payloadSize - 12) / 34)
    return Fail(error, "invalid effect particle count");
  decoded.particles.resize(static_cast<std::size_t>(count));
  for (std::size_t i = 0; i < decoded.particles.size(); ++i) {
    const std::size_t base = 12 + i * 34;
    auto& particle = decoded.particles[i];
    if (!DecodeStructureScalar(payload + base, 2, &particle.start) ||
        !DecodeStructureScalar(payload + base + 2, 2, &particle.end))
      return Fail(error, "invalid effect particle header");
    Track tracks[5];
    for (std::size_t j = 0; j < 5; ++j)
      if (!ReadTrack(payload, payloadSize, base + 4 + j * 6,
                     &tracks[j]))
        return Fail(error, "invalid effect track descriptor");
    if (!ReadKeys(payload, payloadSize, tracks[0], 14,
                  &particle.position) ||
        !ReadKeys(payload, payloadSize, tracks[1], 6,
                  &particle.rotation) ||
        !ReadKeys(payload, payloadSize, tracks[2], 10,
                  &particle.scale) ||
        !ReadKeys(payload, payloadSize, tracks[3], 6,
                  &particle.color) ||
        !ReadKeys(payload, payloadSize, tracks[4], 4,
                  &particle.sprite))
      return Fail(error, "effect track outside payload");
  }
  *output = std::move(decoded);
  if (error) error->clear();
  return true;
}

} // namespace S2FileIO
