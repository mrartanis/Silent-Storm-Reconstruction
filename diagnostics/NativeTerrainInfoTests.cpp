#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/TerrainInfo.h"
#include "../DBFormat/DataRPG.h"
#include "../FileIO/Streams.h"

#include <cstdio>

int main(int argc, char** argv) {
  if (argc > 2) return 8;
  NDb::CMaterial* material = nullptr;
  NDb::CRPGArmor* armor = nullptr;
  if (argc == 2) {
    CFileStream database;
    database.OpenRead(argv[1]);
    NDatabase::Serialize(database, CStructureSaver::READ);
    auto* materials = NDatabase::GetTable<NDb::CMaterial>();
    auto* armors = NDatabase::GetTable<NDb::CRPGArmor>();
    if (!materials || !armors) return 9;
    CDBIterator<NDb::CMaterial> mi(*materials);
    CDBIterator<NDb::CRPGArmor> ai(*armors);
    if (!mi.MoveNext() || !ai.MoveNext()) return 10;
    material = mi.Get();
    armor = ai.Get();
  }
  STerrainInfo data;
  data.nWidth = 17;
  data.nHeight = 9;
  data.heightMap.SetSizes(18, 10);
  for (int y = 0; y < 10; ++y)
    for (int x = 0; x < 18; ++x)
      data.heightMap[y][x] = static_cast<unsigned short>(x + 64 * y);
  if (material && armor) {
    data.spots.push_back(STerrainSpot(material, CVec2(2.0f, 3.0f),
                                      CVec2(4.0f, 5.0f), 0.5f, SStepSound(armor)));
    data.soundmap.SetSizes(1, 1);
    data.soundmap[0][0] = SStepSound(armor);
  }
  CObj<CTerrainInfoHolder> holder = new CTerrainInfoHolder(data);
  if (holder->GetWritableInfo().heightMap[3][4] != 196) return 1;
  data.heightMap[3][4] = 0;
  if (holder->GetWritableInfo().heightMap[3][4] != 196) return 2;
  if (material && armor) {
    const STerrainInfo& copy = holder->GetWritableInfo();
    if (copy.spots.size() != 1 || copy.spots[0].pMaterial.GetPtr() != material ||
        copy.spots[0].stepSound.pArmor.GetPtr() != armor ||
        copy.GetStepSound(0, 0).pArmor.GetPtr() != armor) return 11;
    data.spots.clear();
    if (copy.spots.size() != 1) return 12;
  }

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

  CMemoryStream wire;
  {
    CStructureSaver saver(wire, CStructureSaver::WRITE);
    saver.Add(1, &holder->GetWritableInfo());
  }
  wire.Seek(0);
  STerrainInfo restored;
  {
    CStructureSaver saver(wire, CStructureSaver::READ);
    saver.Add(1, &restored);
  }
  if (restored.nWidth != 17 || restored.nHeight != 9 ||
      restored.heightMap[3][4] != 196) return 13;
  if (material && armor &&
      (restored.spots.size() != 1 ||
       restored.spots[0].pMaterial.GetPtr() != material ||
       restored.spots[0].stepSound.pArmor.GetPtr() != armor ||
       restored.GetStepSound(0, 0).pArmor.GetPtr() != armor)) return 14;

  std::printf("terrain=%d geometry=%d texture=%d grass=%d untouched=%d\n",
              holder->GetWritableInfo().heightMap[3][4],
              access.GetVersion(first), access.GetVersion(texture),
              access.GetVersion(grass), access.GetVersion(second));
  return 0;
}
