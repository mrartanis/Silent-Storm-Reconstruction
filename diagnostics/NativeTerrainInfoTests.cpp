#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/TerrainInfo.h"

#include <cstdio>

int main() {
  STerrainInfo data;
  data.nWidth = 17;
  data.nHeight = 9;
  data.heightMap.SetSizes(18, 10);
  for (int y = 0; y < 10; ++y)
    for (int x = 0; x < 18; ++x)
      data.heightMap[y][x] = static_cast<unsigned short>(x + 64 * y);
  CObj<CTerrainInfoHolder> holder = new CTerrainInfoHolder(data);
  if (holder->GetWritableInfo().heightMap[3][4] != 196) return 1;
  data.heightMap[3][4] = 0;
  if (holder->GetWritableInfo().heightMap[3][4] != 196) return 2;

  const CTRect<int> firstRect(8, 0, 9, 1);
  const CTRect<int> secondRect(0, 0, 1, 1);
  CVersioningBase* first = holder->GetRegionGeometry(firstRect);
  CVersioningBase* second = holder->GetRegionGeometry(secondRect);
  if (!first || !second || first == second) return 3;
  CAccessForDGPtr access;
  if (access.GetVersion(first) != 1 || access.GetVersion(second) != 1) return 4;
  holder->UpdateRegionGeometry(firstRect);
  if (access.GetVersion(first) != 2 || access.GetVersion(second) != 1) return 5;
  CVersioningBase* texture = holder->GetRegionTexture(firstRect);
  CVersioningBase* grass = holder->GetRegionGrass(firstRect);
  if (!texture || !grass || texture == first || grass == first ||
      access.GetVersion(texture) != 1 || access.GetVersion(grass) != 1) return 6;
  holder->UpdateRegionTexture(firstRect);
  holder->UpdateRegionGrass(firstRect);
  if (access.GetVersion(texture) != 2 || access.GetVersion(grass) != 2) return 7;

  std::printf("terrain=%d geometry=%d texture=%d grass=%d untouched=%d\n",
              holder->GetWritableInfo().heightMap[3][4],
              access.GetVersion(first), access.GetVersion(texture),
              access.GetVersion(grass), access.GetVersion(second));
  return 0;
}
