#include "NativeCurve.h"
#include "NativeMMTreeData.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

static const NativeLifeStudio::MMTreeOperationRecord *Find(
    const NativeLifeStudio::MMTreeOperationRecord &record, std::size_t offset)
{
  if (record.offset == offset) return &record;
  for (const auto &child : record.children)
    if (const auto *found = Find(child, offset)) return found;
  return nullptr;
}

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    std::fprintf(stderr, "usage: NativeMMTreeCurveEvaluate tree.mma record-offset expression\n");
    return 2;
  }
  errno = 0;
  char *end = nullptr;
  const unsigned long long parsedOffset = std::strtoull(argv[2], &end, 10);
  if (errno || end == argv[2] || *end) return 2;
  errno = 0;
  end = nullptr;
  const float position = std::strtof(argv[3], &end);
  if (errno || end == argv[3] || *end || !std::isfinite(position)) return 2;
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) return 2;
  const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  NativeLifeStudio::MMTreeRoot root;
  if (!NativeLifeStudio::DecodeMMTreeRoot(bytes.data(), bytes.size(), &root)) return 3;
  const NativeLifeStudio::MMTreeOperationRecord *record = nullptr;
  for (const auto &operation : root.operations)
    if ((record = Find(operation, static_cast<std::size_t>(parsedOffset)))) break;
  if (!record) return 3;
  NativeLifeStudio::MMTreeCurve curve;
  float value = 0.0f;
  if (!NativeLifeStudio::DecodeMMTreeCurve(bytes.data(), bytes.size(), *record, &curve) ||
      !NativeLifeStudio::EvaluateHermiteCurve(curve.x, curve.y, position, &value))
    return 3;
  std::printf("offset=%zu,input=%.9g,value=%.9g,minimum=%.9g,maximum=%.9g\n",
              record->offset, position, value, curve.minimum, curve.maximum);
  return 0;
}
