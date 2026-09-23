#include "NativeCurve.h"
#include <cmath>

namespace NativeLifeStudio
{
bool EvaluateHermiteCurve(const std::vector<float> &x,
                          const std::vector<float> &y,
                          float position, float *value)
{
  if (!value || !std::isfinite(position) || x.size() < 2 || x.size() != y.size())
    return false;
  for (std::size_t i = 0; i < x.size(); ++i)
    if (!std::isfinite(x[i]) || !std::isfinite(y[i]) ||
        (i && !(x[i] > x[i - 1])))
      return false;
  if (position < x.front() || position > x.back())
    return false;
  std::size_t segment = 0;
  while (segment + 2 < x.size() && position > x[segment + 1])
    ++segment;
  const float width = x[segment + 1] - x[segment];
  const float t = (position - x[segment]) / width;
  const float t2 = t * t;
  const float t3 = t2 * t;
  const std::size_t left = segment ? segment - 1 : segment;
  const std::size_t right = segment + 2 < x.size() ? segment + 2 : segment + 1;
  const float slopeA = (y[segment + 1] - y[left]) / (x[segment + 1] - x[left]);
  const float slopeB = (y[right] - y[segment]) / (x[right] - x[segment]);
  const float evaluated = (2 * t3 - 3 * t2 + 1) * y[segment] +
                          (t3 - 2 * t2 + t) * width * slopeA +
                          (-2 * t3 + 3 * t2) * y[segment + 1] +
                          (t3 - t2) * width * slopeB;
  if (!std::isfinite(evaluated))
    return false;
  *value = evaluated;
  return true;
}
}
