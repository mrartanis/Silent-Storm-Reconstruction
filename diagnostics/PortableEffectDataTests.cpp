#include "../FileIO/PortableEffectData.h"
#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <string>
#include <vector>

int main() {
  std::vector<std::uint8_t> bytes(90, 0);
  auto put16 = [&](std::size_t offset, std::int16_t value) {
    return S2FileIO::EncodeStructureScalar(value, bytes.data() + offset, 2);
  };
  auto put32 = [&](std::size_t offset, std::uint32_t value) {
    return S2FileIO::EncodeStructureScalar(value, bytes.data() + offset, 4);
  };
  auto putFloat = [&](std::size_t offset, float value) {
    return S2FileIO::EncodeStructureScalar(value, bytes.data() + offset, 4);
  };
  if (!put32(0, 86) || !putFloat(4, 10) || !putFloat(8, 30) ||
      !put32(12, 1) || !put16(16, 2) || !put16(18, 9)) return 1;
  const std::uint32_t offsets[5] = {46, 60, 66, 76, 82};
  for (int i = 0; i < 5; ++i)
    if (!put16(20 + i * 6, 1) || !put32(22 + i * 6, offsets[i]))
      return 2;
  const float position[3] = {1, -2, 3};
  if (!put16(4 + 46, 3) ||
      !S2FileIO::EncodeStructureFloatFields(position, 3, bytes.data() + 4 + 48, 12) ||
      !put16(4 + 60, 4) || !putFloat(4 + 62, -0.25f) ||
      !put16(4 + 66, 5) || !putFloat(4 + 68, 2) ||
      !putFloat(4 + 72, 0.5f) || !put16(4 + 76, 6) ||
      !put32(4 + 78, 0x12345678) || !put16(4 + 82, 7) ||
      !put16(4 + 84, -8)) return 3;
  S2FileIO::EffectData decoded;
  std::string error;
  if (!S2FileIO::DecodeEffectData(bytes.data(), bytes.size(), &decoded, &error) ||
      decoded.payloadSize != 86 || decoded.endTime != 10 ||
      decoded.frameRate != 30 || decoded.particles.size() != 1) return 4;
  const auto& part = decoded.particles[0];
  if (part.start != 2 || part.end != 9 ||
      part.position.size() != 1 || part.position[0].frame != 3 ||
      part.position[0].value.y != -2 || part.rotation[0].value != -0.25f ||
      part.scale[0].value.x != 2 || part.scale[0].value.y != 0.5f ||
      part.color[0].value != 0x12345678 || part.sprite[0].value != -8)
    return 5;
  if (S2FileIO::DecodeEffectData(bytes.data(), 89, &decoded, &error))
    return 6;
  if (!put32(22, 86) ||
      S2FileIO::DecodeEffectData(bytes.data(), bytes.size(), &decoded, &error))
    return 7;
  return 0;
}
