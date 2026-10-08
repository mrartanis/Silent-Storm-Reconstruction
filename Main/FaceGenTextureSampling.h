#pragma once

#include <cmath>

namespace S2FaceGenTextureSampling {
// FaceGen writes fixed logical subrects. An offline HD layer must use its
// logical-sized mip, otherwise the bake copies only the upper-left HD corner.
// Density-one layers keep the original level and the retail clipping behavior.
inline int LogicalSourceMip(int width, int height, float densityX,
                            float densityY, int mipCount) {
  if (width <= 0 || height <= 0 || mipCount <= 1 ||
      !std::isfinite(densityX) || !std::isfinite(densityY) ||
      densityX < 1.0f || densityY < 1.0f)
    return 0;
  const float logicalWidth = width / densityX;
  const float logicalHeight = height / densityY;
  for (int mip = 0; mip < mipCount; ++mip) {
    if (std::fabs(width - logicalWidth) < 0.001f &&
        std::fabs(height - logicalHeight) < 0.001f)
      return mip;
    width = width > 1 ? width / 2 : 1;
    height = height > 1 ? height / 2 : 1;
  }
  return 0;
}

// The retail CPU source-over operation, shared by the real bake and its
// independent atlas/alpha fixtures. Source RGB is straight, not premultiplied.
template<class Image>
void BlendLayer(Image& dst, const Image& src, int x0, int y0,
                int x1, int y1, float weight) {
  const int width = x1-x0 < src.GetXSize() ? x1-x0 : src.GetXSize();
  const int height = y1-y0 < src.GetYSize() ? y1-y0 : src.GetYSize();
  for (int y=0; y<height; ++y)
    for (int x=0; x<width; ++x) {
      auto& target = dst[y1-1-y][x0+x];
      const auto& source = src[y][x];
      const float alpha = source.a * weight * (1.0f/255.0f);
      const float inverse = 1.0f-alpha;
      const int r = int(source.r*alpha + target.r*inverse + 0.5f);
      const int g = int(source.g*alpha + target.g*inverse + 0.5f);
      const int b = int(source.b*alpha + target.b*inverse + 0.5f);
      target.r = r < 0 ? 0 : (r > 255 ? 255 : r);
      target.g = g < 0 ? 0 : (g > 255 ? 255 : g);
      target.b = b < 0 ? 0 : (b > 255 ? 255 : b);
      target.a = 255;
    }
}
}
