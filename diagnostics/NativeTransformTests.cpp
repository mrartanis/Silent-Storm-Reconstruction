#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/HeadlessPlatform.h"
#include "../Misc/Geom.h"
#endif
#include "../Misc/Tools.h"
#include "../Main/Transform.h"

#include <cmath>
#include <cstdio>

namespace {
bool Near(float actual, float expected) {
  return std::isfinite(actual) && std::fabs(actual - expected) < 0.0001f;
}
}

int main() {
  SHMatrix identity;
  identity._11 = identity._12 = identity._13 = identity._14 = -5.0f;
  identity._21 = identity._22 = identity._23 = identity._24 = -5.0f;
  identity._31 = identity._32 = identity._33 = identity._34 = -5.0f;
  identity._41 = identity._42 = identity._43 = identity._44 = -5.0f;
  Identity(&identity);
  const float expectedIdentity[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f};
  const float actualIdentity[16] = {
      identity._11, identity._12, identity._13, identity._14,
      identity._21, identity._22, identity._23, identity._24,
      identity._31, identity._32, identity._33, identity._34,
      identity._41, identity._42, identity._43, identity._44};
  for (int i = 0; i != 16; ++i)
    if (GameFloatBits(actualIdentity[i]) != GameFloatBits(expectedIdentity[i]))
      return 14;

  const SFBTransform translation = MakeTransform(CVec3(3.0f, 4.0f, 5.0f));
  if (!Near(translation.forward._14, 3.0f) ||
      !Near(translation.forward._24, 4.0f) ||
      !Near(translation.forward._34, 5.0f) ||
      !Near(translation.backward._14, -3.0f) ||
      !Near(translation.backward._24, -4.0f) ||
      !Near(translation.backward._34, -5.0f)) return 1;

  CVec3 moved, restored;
  translation.forward.RotateHVector(&moved, CVec3(1.0f, 2.0f, 3.0f));
  translation.backward.RotateHVector(&restored, moved);
  if (!Near(moved.x, 4.0f) || !Near(moved.y, 6.0f) ||
      !Near(moved.z, 8.0f) || !Near(restored.x, 1.0f) ||
      !Near(restored.y, 2.0f) || !Near(restored.z, 3.0f)) return 2;

  SHMatrix rotation, inverse;
  MakeMatrix(&rotation, 0.25f, -0.5f, 0.125f, CVec3(2.0f, -3.0f, 4.0f));
  if (!InvertMatrix(&inverse, rotation)) return 3;
  CVec3 rotated, unrotated;
  rotation.RotateHVector(&rotated, CVec3(-1.0f, 2.0f, 0.5f));
  inverse.RotateHVector(&unrotated, rotated);
  if (!Near(rotated.x, 2.03938246f) ||
      !Near(rotated.y, -0.988380671f) ||
      !Near(rotated.z, 5.09628296f) ||
      !Near(unrotated.x, -1.0f) || !Near(unrotated.y, 2.0f) ||
      !Near(unrotated.z, 0.5f)) return 4;

  SBound box, transformed;
  box.BoxInit(CVec3(-1.0f, -2.0f, -3.0f), CVec3(1.0f, 2.0f, 3.0f));
  if (!Near(CalcRadius2(box, translation.forward), 14.0f)) return 5;
  TransformBound(&transformed, box, translation.forward);
  if (!Near(transformed.s.ptCenter.x, 3.0f) ||
      !Near(transformed.s.ptCenter.y, 4.0f) ||
      !Near(transformed.s.ptCenter.z, 5.0f) ||
      !Near(transformed.s.fRadius, std::sqrt(14.0f))) return 6;

  CTransformStack camera;
  camera.MakeProjective(1.0f, 90.0f, 0.1f, 100.0f,
                        CVec2(0.125f, -0.25f));
  const SHMatrix& projection = camera.GetProjection().forward;
  if (!Near(projection._11, 1.0f) ||
      !Near(projection._22, 1.0f) ||
      !Near(projection._13, 0.125f) ||
      !Near(projection._23, -0.25f) ||
      !Near(projection._43, 1.0f)) return 7;

  CTRect<float> cover;
  if (!camera.GetCoverRect(&cover, CVec3(0.0f, 0.0f, 5.0f), 1.0f) ||
      !std::isfinite(cover.x1) || !std::isfinite(cover.x2) ||
      !std::isfinite(cover.y1) || !std::isfinite(cover.y2) ||
      !Near(cover.x1, -0.0956955254f) ||
      !Near(cover.x2, 0.408195525f) ||
      !Near(cover.y1, -0.570194125f) ||
      !Near(cover.y2, -0.0365372747f) ||
      cover.x1 >= cover.x2 || cover.y1 >= cover.y2) return 8;

  float signedZero = GameFloatFromBits(0x80000000u);
  if (GameFloatBits(signedZero) != 0x80000000u ||
      FP_SIGN_BIT_CONST(signedZero) != 0x80000000u) return 9;
  FlipFloatSign(&signedZero);
  if (GameFloatBits(signedZero) != 0u) return 10;
  const DWORD nanPayload = 0x7fc12345u;
  float payload = GameFloatFromBits(nanPayload);
  if (bit_cast<DWORD>(payload) != nanPayload) return 11;
  FlipFloatSign(&payload);
  if (GameFloatBits(payload) != (nanPayload ^ 0x80000000u)) return 12;
  FlipFloatSign(&payload);
  if (GameFloatBits(payload) != nanPayload) return 13;

  std::printf("rotated=%.9g,%.9g,%.9g radius2=%.9g cover=%.9g,%.9g,%.9g,%.9g\n",
              rotated.x, rotated.y, rotated.z,
              CalcRadius2(box, translation.forward),
              cover.x1, cover.x2, cover.y1, cover.y2);
  return 0;
}
