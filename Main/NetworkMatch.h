#pragma once
#include "wMain.h"
#include "rpgGlobal.h"
#include "../DBFormat/DataLight.h"

namespace S2Net {
struct NetworkMapChoice {
  int templateID=0,variantID=0;
  wstring name;
};
// Large RANDOM campaign encounters only; the solo encounter list is unchanged.
vector<NetworkMapChoice> GetNetworkEncounterMaps();
bool IsNetworkMapLargeEnough(int width,int height);
vector<int> GetNetworkRecruitIDs();
vector<int> GetNetworkSupplyItems(int level);
void RefreshNetworkStore(NRPG::CGlobalPlayer* player);
CObj<NRPG::CUnit> CreateNetworkRecruit(int persona,int level);
// Future team selection fills this same preset before the host builds the world.
struct NetworkMatchPreset {
  int mapVariant = 810;
  unsigned seed = 123;
  bool useEncounterDeployment = false;
  bool prepareOnBase = false;
  int squadSize = 6, startingLevel = 1, supplyLevel = 1;
  vector<int> personas = {54, 53, 14, 34, 2, 3};
  struct Equipment {
    int spareClipsPerFighter=2,grenadesPerFighter=1;
    int grenadeRecord=21,medicalSupplies=2,mines=2,clearingTools=1;
  } equipment;
  struct Rules { unsigned firstSlot=0;int segmentMilliseconds=50; } rules;
};
struct NetworkMatch {
  int templateID=810,variantID=810;
  CObj<NRPG::CGlobalGame> game;
  CObj<NWorld::CWorld> world;
  CPtr<NWorld::CPlayer> players[2];
  vector<CPtr<NWorld::CUnit>> baseServices; // officer, medic, quartermaster
  CObj<NDb::CAmbientLightReal> ambient;
  // -2: ongoing, -1: draw, 0/1: winner. Actions must finish first.
  int Winner() const;
};
NetworkMatch CreateNetworkMatch(const NetworkMatchPreset& preset);
NetworkMatch CreateNetworkBase(const NetworkMatchPreset& preset,unsigned slot,NRPG::CGlobalPlayer* squad=nullptr);
NetworkMatch CreatePreparedNetworkMatch(const NetworkMatchPreset& preset,NRPG::CGlobalPlayer* first,NRPG::CGlobalPlayer* second);
}
