#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/wHeightLayers.h"
#include "../FileIO/Streams.h"

#include <cstdio>

int main() {
  CObj<NWorld::CHeightLayers> cache = new NWorld::CHeightLayers;
  auto* terrain = cache->GetTerrainLayer();
  terrain->heights.SetSizes(3, 2);
  terrain->heights.FillZero();
  terrain->heights[1][2] = 6.25f;
  if (!cache->HasTerrain() || cache->GetLayer(0) != terrain) return 1;

  auto* floor = cache->GetCreateLayer(2);
  if (floor == terrain || floor->heights[1][2] != 6.25f) return 2;
  floor->heights[1][2] = 9.5f;
  if (terrain->heights[1][2] != 6.25f || cache->GetLayer(3) != floor ||
      cache->GetRealFloor(3) != 2) return 3;

  CMemoryStream wire;
  {
    CStructureSaver saver(wire, CStructureSaver::WRITE);
    saver.Add(1, &cache);
  }
  wire.Seek(0);
  CObj<NWorld::CHeightLayers> restored;
  {
    CStructureSaver saver(wire, CStructureSaver::READ);
    saver.Add(1, &restored);
  }
  if (!restored || !restored->HasTerrain() ||
      restored->GetTerrainLayer()->heights[1][2] != 6.25f ||
      restored->GetLayer(3)->heights[1][2] != 9.5f) return 4;
  std::printf("height terrain=%.2f floor=%.2f mapped=%d\n",
              restored->GetTerrainLayer()->heights[1][2],
              restored->GetLayer(3)->heights[1][2], restored->GetRealFloor(3));
  return 0;
}
