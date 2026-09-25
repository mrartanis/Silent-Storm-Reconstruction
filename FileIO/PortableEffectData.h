#ifndef S2_FILEIO_PORTABLE_EFFECT_DATA_H
#define S2_FILEIO_PORTABLE_EFFECT_DATA_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace S2FileIO {

struct EffectVec2 { float x = 0, y = 0; };
struct EffectVec3 { float x = 0, y = 0, z = 0; };

template<class T>
struct EffectKey {
  std::int16_t frame = 0;
  T value{};
};

struct EffectParticle {
  std::int16_t start = 0, end = 0;
  std::vector<EffectKey<EffectVec3>> position;
  std::vector<EffectKey<float>> rotation;
  std::vector<EffectKey<EffectVec2>> scale;
  std::vector<EffectKey<std::uint32_t>> color;
  std::vector<EffectKey<std::int16_t>> sprite;
};

struct EffectData {
  std::uint32_t payloadSize = 0;
  float endTime = 0;
  float frameRate = 0;
  std::vector<EffectParticle> particles;
};

// Input includes the 4-byte payload-size prefix from Effects.res. Track
// offsets are relative to the first byte after that prefix, not host pointers.
bool DecodeEffectData(const std::uint8_t* bytes, std::size_t length,
                      EffectData* output, std::string* error = nullptr);

} // namespace S2FileIO

#endif
