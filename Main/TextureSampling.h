#ifndef S2_TEXTURE_SAMPLING_H
#define S2_TEXTURE_SAMPLING_H
#include <cmath>

namespace S2TextureSampling {
struct Rect { float x1, y1, x2, y2; };

// SHORT2 is signed and the backend preserves its integer value. Keep retail
// eighth-texel precision when possible; reduce the multiplier rather than
// clipping physical coordinates. Holder extent also keeps a normal atlas batch
// on one precision even when individual rectangles sample different tiles.
inline float UVPackingSteps(const Rect& physicalSource, int holderWidth, int holderHeight) {
  float extent = std::fmax(float(holderWidth), float(holderHeight));
  extent = std::fmax(extent, std::fabs(physicalSource.x1));
  extent = std::fmax(extent, std::fabs(physicalSource.y1));
  extent = std::fmax(extent, std::fabs(physicalSource.x2));
  extent = std::fmax(extent, std::fabs(physicalSource.y2));
  float steps = 8.0f;
  // Half-unit safety also covers the active Float2Int rounding mode.
  while (extent * steps > 32766.5f) steps *= 0.5f;
  return steps;
}

inline float TexelUVScale(int holderSize, bool unnormalizedNP2) {
  return 1.0f / (unnormalizedNP2 ? 1 : (holderSize > 0 ? holderSize : 1));
}

inline float PackedUVScale(float steps, int holderSize, bool unnormalizedNP2) {
  return TexelUVScale(holderSize, unnormalizedNP2) / steps;
}

inline float TexelScale(int physicalSize, int logicalSize) {
  return logicalSize > 0 ? float(physicalSize) / logicalSize : 1.0f;
}

// Keep the retail correction in logical texels, before physical density/placement.
inline void CorrectAxis(float target1, float target2, float& source1, float& source2) {
  const float targetWidth = std::fabs(target2 - target1);
  const float sourceWidth = std::fabs(source2 - source1);
  if (targetWidth != sourceWidth) {
    const float center = 0.5f * (source1 + source2);
    if (targetWidth > sourceWidth && sourceWidth > 0 && targetWidth > 1) {
      const float scale = (sourceWidth - 1) / sourceWidth * targetWidth / (targetWidth - 1);
      source1 = center + (source1 - center) * scale;
      source2 = center + (source2 - center) * scale;
    }
  }
}

inline Rect SampleRect(const Rect& roundedTarget, Rect logicalSource,
                       float densityX, float densityY, float placementX, float placementY) {
  CorrectAxis(roundedTarget.x1, roundedTarget.x2, logicalSource.x1, logicalSource.x2);
  CorrectAxis(roundedTarget.y1, roundedTarget.y2, logicalSource.y1, logicalSource.y2);
  return {placementX + logicalSource.x1 * densityX, placementY + logicalSource.y1 * densityY,
          placementX + logicalSource.x2 * densityX, placementY + logicalSource.y2 * densityY};
}
} // namespace S2TextureSampling
#endif
