#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#endif
#include "../Misc/Geom.h"
#include "../Misc/2Darray.h"
#include "../Main/BetaSpline.h"

#include <cmath>
#include <cstdio>

namespace {
bool Near(float actual, float expected) {
  return std::isfinite(actual) && std::fabs(actual - expected) < 0.0001f;
}

float Height(int x, int y) {
  return 0.125f * x * x + 0.25f * y * y - 0.0625f * x * y +
         0.5f * x - 0.75f * y;
}
}

int main() {
  CBetaSpline spline;
  spline.Init(1.25f, 0.75f);
  CArray2D<float> grid(7, 7);
  for (int y = 0; y < 7; ++y)
    for (int x = 0; x < 7; ++x)
      grid[y][x] = Height(x, y);

  if (!Near(spline.Ave(grid, 0, 0), Height(0, 0)) ||
      !Near(spline.Value(grid, 6, 6), Height(6, 6))) return 1;

  CVec3 points[16];
  for (int y = 0; y < 4; ++y)
    for (int x = 0; x < 4; ++x)
      points[y * 4 + x] = CVec3(float(x), float(y), Height(x, y));
  const CVec3 point = spline.Value(0.3f, 0.6f, points);
  CVec3 du, dv;
  spline.Derivative(du, dv, 0.3f, 0.6f, points);
  const float average = spline.Ave(grid, 3, 3);
  const float knot = spline.Value(grid, 3, 3);
  if (!Near(point.x, 1.21322346f) ||
      !Near(point.y, 1.52093554f) ||
      !Near(point.z, 0.234747231f) ||
      !Near(du.z, 0.747696161f) ||
      !Near(dv.z, -0.0673760623f) ||
      !Near(average, 2.9366281f) ||
      !Near(knot, 2.01249027f)) return 2;

  std::printf("point=%.9g,%.9g,%.9g derivative=%.9g,%.9g average=%.9g knot=%.9g\n",
              point.x, point.y, point.z, du.z, dv.z, average, knot);
  return 0;
}
