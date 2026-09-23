#include "LifeStudioHeadAPIGDP.h"
#include "NativeFaceGenData.h"
#include <algorithm>
#include <cstdio>

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::fprintf(stderr, "usage: NativeFaceGenApiCheck face.gdp\n");
    return 2;
  }
  auto *gdp = LifeStudioHeadAPI::IGDPFile::Create(argv[1]);
  if (!gdp) return 3;
  if (gdp->ObjectsCount() < 1)
  {
    gdp->Destroy();
    return 3;
  }
  auto *object = gdp->Object(0);
  NativeLifeStudio::FaceGenData face;
  const bool ok = object && NativeLifeStudio::LoadFaceGenData(object, &face);
  if (ok)
    std::printf("heads=%zu combinations=%zu archetypes=%zu link-morphs=%zu link-outputs=%zu link-nonzero=%zu vertices=%u animation-muscles=%u morph-muscles=%u\n",
                face.heads.size(), face.combinations.size(), face.archetypes.size(),
                face.links.morphNames.size(), face.links.outputNames.size(),
                std::count_if(face.links.matrix.begin(), face.links.matrix.end(),
                              [](std::uint8_t value) { return value != 0; }),
                face.archetypes.front().animation.vertexCount,
                face.archetypes.front().animation.muscleCount,
                face.archetypes.front().morph.muscleCount);
  if (object) object->Destroy();
  gdp->Destroy();
  return ok ? 0 : 3;
}
