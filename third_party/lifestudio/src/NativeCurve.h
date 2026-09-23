#pragma once

#include <vector>

namespace NativeLifeStudio
{
// Nonuniform cubic Hermite interpolation used by LifeStudio sequence and
// macro-tree curves. Inputs outside the sampled domain are rejected.
bool EvaluateHermiteCurve(const std::vector<float> &x,
                          const std::vector<float> &y,
                          float position, float *value);
}
