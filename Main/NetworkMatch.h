#pragma once
#include "wMain.h"
#include "rpgGlobal.h"

namespace S2Net {
struct NetworkMapChoice {
  int templateID=0,variantID=0;
  wstring name;
};
// Exactly the RANDOM sectors used by the campaign chapter maps, including
// their available tactical variants. No campaign missions or bases.
vector<NetworkMapChoice> GetNetworkEncounterMaps();
// Future team selection fills this same preset before the host builds the world.
struct NetworkMatchPreset {
  int mapVariant = 810;
  unsigned seed = 123;
  bool useEncounterDeployment = false;
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
  // -2: ongoing, -1: draw, 0/1: winner. Actions must finish first.
  int Winner() const;
};
NetworkMatch CreateNetworkMatch(const NetworkMatchPreset& preset);
}
