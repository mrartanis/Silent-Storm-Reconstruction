#include "NativeMMTreeData.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static std::uint32_t ReadU32(const char *bytes)
{
  const auto *p = reinterpret_cast<const unsigned char *>(bytes);
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

int main(int argc, char **argv)
{
  if (argc != 5)
  {
    std::fprintf(stderr, "usage: NativeMMTreeFilterEffects original.mma macro-name muscle-name output.mma\n");
    return 2;
  }
  std::error_code pathError;
  if (std::filesystem::exists(argv[4], pathError) &&
      std::filesystem::equivalent(argv[1], argv[4], pathError))
  {
    std::fprintf(stderr, "refusing to overwrite the original tree\n");
    return 2;
  }
  std::ifstream input(argv[1], std::ios::binary);
  if (!input) return 2;
  std::vector<char> bytes((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
  NativeLifeStudio::MMTreeRoot root;
  std::vector<NativeLifeStudio::MMTreeEffectSample> effects;
  if (!NativeLifeStudio::DecodeMMTreeRoot(bytes.data(), bytes.size(), &root) ||
      !NativeLifeStudio::ValidateMMTreeCurves(bytes.data(), bytes.size(), root) ||
      !NativeLifeStudio::EvaluateMMTreeMacro(bytes.data(), bytes.size(), root,
                                              argv[2], 0.5f, &effects))
    return 3;
  std::size_t retained = 0;
  std::size_t muted = 0;
  for (const auto &effect : effects)
  {
    if (effect.kind == 3 && effect.targetName == argv[3])
    {
      ++retained;
      continue;
    }
    const std::size_t offset = effect.operationOffset;
    if (offset > bytes.size() || bytes.size() - offset < 40) return 3;
    const std::uint32_t count = ReadU32(bytes.data() + offset + 36);
    if (count < 2 || count > 64 || bytes.size() - offset < 40 + 8 * count)
      return 3;
    const std::size_t yStart = offset + 40 + 4 * count;
    std::fill(bytes.begin() + yStart, bytes.begin() + yStart + 4 * count, 0);
    ++muted;
  }
  if (!retained || !muted ||
      !NativeLifeStudio::ValidateMMTreeCurves(bytes.data(), bytes.size(), root))
    return 3;
  std::ofstream output(argv[4], std::ios::binary | std::ios::trunc);
  if (!output) return 2;
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  if (!output) return 2;
  std::fprintf(stderr, "retained=%zu muted=%zu\n", retained, muted);
  return 0;
}
