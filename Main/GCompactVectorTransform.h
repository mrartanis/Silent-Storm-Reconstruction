#pragma once
#include "GPixelFormat.h"

namespace NGScene {
// The MMX rows contain cyclic lanes: a=(z,y,x), b=(x,z,y), c=(y,x,z).
// Keep that layout when transforming packed normals and tangent vectors on x64.
inline CVec3 TransformCompactVector(const NGfx::SCompactVector* source,
                                   const NGfx::SCompactTransformer* transform) {
  const CVec3 src = NGfx::GetVector(*source);
  const float scale = 1.0f / 2048.0f;
  return CVec3(
    (src.x * transform->a.nX + src.y * transform->b.nX + src.z * transform->c.nX) * scale,
    (src.y * transform->a.nY + src.z * transform->b.nY + src.x * transform->c.nY) * scale,
    (src.z * transform->a.nZ + src.x * transform->b.nZ + src.y * transform->c.nZ) * scale);
}
}
