#include "../FileIO/PortableEffectData.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {
std::uint64_t Byte(std::uint64_t hash, std::uint8_t value) {
  return (hash ^ value) * UINT64_C(1099511628211);
}
template<class T>
void Value(std::uint64_t* hash, T value) {
  std::uint8_t bytes[sizeof(T)] = {};
  S2FileIO::EncodeStructureScalar(value, bytes, sizeof(bytes));
  for (std::uint8_t byte : bytes) *hash = Byte(*hash, byte);
}
template<class T, class F>
void Keys(std::uint64_t* hash, const std::vector<S2FileIO::EffectKey<T>>& keys,
          F addValue) {
  Value(hash, static_cast<std::uint32_t>(keys.size()));
  for (const auto& key : keys) {
    Value(hash, key.frame);
    addValue(hash, key.value);
  }
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: PortableEffectCorpusProbe Effects.res\n");
    return 2;
  }
  S2FileIO::PortablePackageIndex package;
  std::string error;
  if (!package.Open(argv[1], &error)) {
    std::fprintf(stderr, "open: %s\n", error.c_str());
    return 3;
  }
  std::uint64_t hash = UINT64_C(14695981039346656037);
  std::uint64_t particles = 0, keys = 0;
  for (const auto& entry : package.Entries()) {
    std::vector<std::uint8_t> bytes;
    S2FileIO::EffectData effect;
    if (!package.Read(entry.first, &bytes, &error) ||
        !S2FileIO::DecodeEffectData(bytes.data(), bytes.size(), &effect, &error)) {
      std::fprintf(stderr, "effect %d: %s\n", entry.first, error.c_str());
      return 4;
    }
    Value(&hash, entry.first);
    Value(&hash, effect.endTime);
    Value(&hash, effect.frameRate);
    Value(&hash, static_cast<std::uint32_t>(effect.particles.size()));
    particles += effect.particles.size();
    for (const auto& p : effect.particles) {
      Value(&hash, p.start); Value(&hash, p.end);
      keys += p.position.size() + p.rotation.size() + p.scale.size() +
              p.color.size() + p.sprite.size();
      Keys(&hash, p.position, [](std::uint64_t* h, S2FileIO::EffectVec3 v) {
        Value(h, v.x); Value(h, v.y); Value(h, v.z);
      });
      Keys(&hash, p.rotation, [](std::uint64_t* h, float v) {
        Value(h, v);
      });
      Keys(&hash, p.scale, [](std::uint64_t* h, S2FileIO::EffectVec2 v) {
        Value(h, v.x); Value(h, v.y);
      });
      Keys(&hash, p.color, [](std::uint64_t* h, std::uint32_t v) {
        Value(h, v);
      });
      Keys(&hash, p.sprite, [](std::uint64_t* h, std::int16_t v) {
        Value(h, v);
      });
    }
  }
  std::printf("effects=%zu particles=%llu keys=%llu fnv64=%016llx\n",
              package.Entries().size(),
              static_cast<unsigned long long>(particles),
              static_cast<unsigned long long>(keys),
              static_cast<unsigned long long>(hash));
  return 0;
}
