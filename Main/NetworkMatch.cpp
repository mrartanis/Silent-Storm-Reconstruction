#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "NetworkMatch.h"
#include "aiGrid.h"
#include "aiCommander.h"
#include "rpgDiplomacy.h"
#include "RPGUnit.h"
#include "RPGUnitMission.h"
#include "RPGItem.h"
#include "RPGStore.h"
#include "rpgPerk.h"
#include "RPGMerc.h"
#include "wUnitServer.h"
#include "../DBFormat/DataRPG.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataMap.h"
#include "../DBFormat/DataMisc.h"
#include "ChapterInfo.h"
#include "GlobalInfo.h"
#include <algorithm>
#include <queue>
#include <map>
#include <set>
#include <stdexcept>

namespace NWorld {
void CWorld::PrepareNetworkDeployment(int count) {
  // Native standing edges (including their reciprocal edge) avoid crossing
  // walls merely because adjacent tiles happen to be individually passable.
  auto* net = dynamic_cast<NAI::CPathNetwork*>(GetPathNetwork());
  if (!net || count < 1) throw std::runtime_error("missing network map");
  vector<NAI::SPathPlace> largest;
  vector<NAI::SPathPlace> sideAreas[2];
  double bestSideScore[2]={1e30,1e30};
  double bestSpawnScore=1e30;
  if(bNetworkEncounterDeployment && networkSpawnPositions[0].empty()) {
    // Same fallback as solo GetDeployWithNumber for maps without an authored
    // entrance marker; SetOnLayer supplies the actual terrain height.
    NAI::SPosition entrance;net->SetOnLayer(&entrance,0,CVec3(FP_GRID_STEP,8*FP_GRID_STEP,0));
    networkSpawnPositions[0].push_back(entrance.GetCP());
  }
  if(bNetworkEncounterDeployment && networkSpawnPositions[1].empty())
    throw std::runtime_error("Encounter has no player or enemy spawn positions");
  vector<vector<bool>> seen(net->GetNumLayers());
  std::map<std::uint32_t,vector<NAI::SPathPlace>> links;
  auto canonical=[](NAI::SPathPlace p){p.SetIntegral(1);p.SetPose(NAI::CM_STAND);p.SetDirection(0);p.SetMoving(0);p.SetFinal(0);return p;};
  for(int layer=0;layer<net->GetNumLayers();++layer) {
    auto* nodes=net->GetLayer(layer);
    seen[layer].resize(nodes->tiles.GetXSize()*nodes->tiles.GetYSize());
    for(const auto& entry:nodes->transitions)for(const auto& edge:entry.second.links)
      links[canonical(entry.first).GetBits()].push_back(canonical(edge.dst));
    for(const auto& ladder:nodes->ladders)if(ladder.bConsistent) {
      auto bottom=canonical(ladder.placeOnBottom),top=canonical(ladder.placeOnTop);
      links[bottom.GetBits()].push_back(top);links[top.GetBits()].push_back(bottom);
    }
  }
  for(auto& entry:links)std::sort(entry.second.begin(),entry.second.end(),[](auto a,auto b){return a.GetBits()<b.GetBits();});
  for (int layer = 0; layer < net->GetNumLayers(); ++layer) {
    auto& grid = net->GetLayer(layer)->tiles;
    int width = grid.GetXSize(), height = grid.GetYSize();
    auto& visited=seen[layer];
    auto place = [layer](int x, int y) {
      NAI::SPathPlace p(x, y, layer, 0, NAI::CM_STAND, 0);
      p.SetIntegral(1); return p;
    };
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
      if (visited[y*width+x] || !(grid[y][x].nPassable & NAI::CP_STAND)) continue;
      vector<NAI::SPathPlace> component;
      std::queue<NAI::SPathPlace> pending;
      pending.push(place(x,y)); visited[y*width+x] = true;
      while (!pending.empty()) {
        auto p = pending.front(); pending.pop(); component.push_back(p);
        auto& current=net->GetLayer(p.GetLayer())->tiles;
        auto& currentSeen=seen[p.GetLayer()];
        int currentWidth=current.GetXSize(),currentHeight=current.GetYSize();
        auto add=[&](NAI::SPathPlace target) {
          if(target.GetLayer()>=net->GetNumLayers())return;
          auto& next=net->GetLayer(target.GetLayer())->tiles;
          if(target.GetX()>=next.GetXSize() || target.GetY()>=next.GetYSize())return;
          auto index=target.GetY()*next.GetXSize()+target.GetX();
          if(seen[target.GetLayer()][index] || !(next[target.GetY()][target.GetX()].nPassable & NAI::CP_STAND))return;
          seen[target.GetLayer()][index]=true;pending.push(target);
        };
        for (int dir = 0; dir < 8; ++dir) {
          int nx = p.GetX()+NAI::nMoveShift[dir][0], ny = p.GetY()+NAI::nMoveShift[dir][1];
          if (nx < 0 || ny < 0 || nx >= currentWidth || ny >= currentHeight || currentSeen[ny*currentWidth+nx]) continue;
          if (!(current[p.GetY()][p.GetX()].nMoveStand & (1<<dir)) ||
              !(current[ny][nx].nMoveStand & (1<<((dir+4)%8)))) continue;
          auto next=p;next.SetXY(nx,ny);add(next);
        }
        auto extra=links.find(p.GetBits());if(extra!=links.end())for(auto target:extra->second)add(target);
      }
      if(bNetworkEncounterDeployment && component.size()>=static_cast<size_t>(count)) {
        double nearest[2]={1e30,1e30};
        for(auto p:component) {
          const auto point=net->GetCP(p);
          for(int side=0;side<2;++side)for(size_t index=0;index<(side?networkSpawnPositions[side].size():1);++index) {
            const auto& target=networkSpawnPositions[side][index];
            auto delta=point-target;
            nearest[side]=(std::min)(nearest[side],double(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z));
          }
        }
        double score=nearest[0]+nearest[1];
        for(int side=0;side<2;++side)if(nearest[side]<=16 &&
          (nearest[side]<bestSideScore[side] || (nearest[side]==bestSideScore[side] && component.size()>sideAreas[side].size()))) {
          bestSideScore[side]=nearest[side];sideAreas[side]=component;
        }
        if(component.size()>=static_cast<size_t>(2*count) && nearest[0]<=16 && nearest[1]<=16 &&
           (score<bestSpawnScore || (score==bestSpawnScore && component.size()>largest.size()))) {
          bestSpawnScore=score;largest.swap(component);
        }
      } else if(!bNetworkEncounterDeployment && component.size()>largest.size())largest.swap(component);
    }
  }
  if(bNetworkEncounterDeployment) {
    // A closed door can split the standing graph at load time. Preserve the
    // authored spawn areas in that case; opening doors joins them during play.
    if(!largest.empty())sideAreas[0]=sideAreas[1]=largest;
    if(sideAreas[0].empty() || sideAreas[1].empty())
      throw std::runtime_error("Encounter spawn area cannot fit a squad");
  } else {
    if(largest.size()<static_cast<size_t>(2*count))
      throw std::runtime_error("map has no connected deployment area for both squads");
    sideAreas[0]=sideAreas[1]=largest;
  }
  auto distance = [net](const NAI::SPathPlace& a, const NAI::SPathPlace& b) {
    CVec3 d = net->GetCP(a)-net->GetCP(b); return d.x*d.x+d.y*d.y+d.z*d.z;
  };
  // The fixed arena is small enough to select the actual most distant pair.
  // Sorting also makes equal-distance choices independent of hash iteration.
  std::sort(largest.begin(),largest.end(),[](auto a,auto b){return a.GetBits()<b.GetBits();});
  for(auto& area:sideAreas)std::sort(area.begin(),area.end(),[](auto a,auto b){return a.GetBits()<b.GetBits();});
  vector<CVec3> points;for(auto p:largest)points.push_back(net->GetCP(p));
  auto a=sideAreas[0].front(),b=sideAreas[1].front();float maximum=-1;
  if(!bNetworkEncounterDeployment) {
    for(size_t i=0;i<largest.size();++i)for(size_t j=i+1;j<largest.size();++j) {
      auto d=points[i]-points[j];float squared=d.x*d.x+d.y*d.y+d.z*d.z;
      if(squared>maximum){maximum=squared;a=largest[j];b=largest[i];}
    }
  } else {
    auto nearest=[&](int side,const CVec3& target) {
      auto best=sideAreas[side].front();float minimum=1e30f;
      for(auto p:sideAreas[side]) {auto delta=net->GetCP(p)-target;float d=delta.x*delta.x+delta.y*delta.y+delta.z*delta.z;
        if(d<minimum){minimum=d;best=p;}}
      return best;
    };
    a=nearest(0,networkSpawnPositions[0].front());
    // Choose an authored enemy area apart from the player's entrance, then
    // place the opposing squad at actual enemy positions within that area.
    maximum=-1;
    for(const auto& target:networkSpawnPositions[1]) {
      auto p=nearest(1,target);auto delta=net->GetCP(p)-target;
      if(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z>16)continue;
      float d=distance(a,p);if(d>maximum){maximum=d;b=p;}
    }
  }
  vector<NAI::SPathPlace> used;
  vector<CVec3> reachableSpawns[2];
  if(bNetworkEncounterDeployment)for(int side=0;side<2;++side) {
    for(auto target:networkSpawnPositions[side]) {
      float minimum=1e30f;
      for(auto p:sideAreas[side]){auto d=net->GetCP(p)-target;minimum=(std::min)(minimum,d.x*d.x+d.y*d.y+d.z*d.z);}
      if(minimum<=16)reachableSpawns[side].push_back(target);
    }
    const auto center=net->GetCP(side?b:a);
    if(side)std::stable_sort(reachableSpawns[side].begin(),reachableSpawns[side].end(),[&](auto p,auto q){
      auto x=p-center,y=q-center;return x.x*x.x+x.y*x.y+x.z*x.z<y.x*y.x+y.y*y.y+y.z*y.z;});
  }
  for (int side = 0; side < 2; ++side) {
    auto anchor = side ? b : a;
    networkDeployment[side].clear();
    for(int fighter=0;fighter<count;++fighter) {
      CVec3 target=net->GetCP(anchor);
      if(bNetworkEncounterDeployment) {
        const auto& authored=reachableSpawns[side];
        target=authored[std::min<size_t>(fighter,authored.size()-1)];
      }
      auto candidates=sideAreas[side];
      std::stable_sort(candidates.begin(),candidates.end(),[&](auto p,auto q){
        auto x=net->GetCP(p)-target,y=net->GetCP(q)-target;
        return x.x*x.x+x.y*x.y+x.z*x.z<y.x*y.x+y.y*y.y+y.z*y.z;});
      for(auto p:candidates) {
        bool clear=true;for(auto q:used)if(distance(p,q)<1.0f){clear=false;break;}
        if(!clear || !net->IsNativePassable(p) || net->IsLocked(p))continue;
        p.SetDirection(net->GetClosestDir(p,side?a:b));
        networkDeployment[side].push_back(p);used.push_back(p);break;
      }
    }
    if (networkDeployment[side].size() != static_cast<size_t>(count))
      throw std::runtime_error("not enough unoccupied network deployment positions");
  }
}
}

namespace S2Net {
bool IsNetworkMapLargeEnough(int width,int height) { return width>=32 && height>=32 && static_cast<long long>(width)*height>=2048; }
vector<NetworkMapChoice> GetNetworkEncounterMaps() {
  std::map<int,wstring> regions;
  auto* globalTable=NDatabase::GetTable<NDb::CGlobalMap>();
  if(globalTable) {
    CDBIterator<NDb::CGlobalMap> maps(*globalTable);
    while(maps.MoveNext()) {
      int id=maps.Get()->GetRecordID();
      if(!NGScene::CResourceFileOpener::DoesExist("Globals",id))continue;
      vector<SGlobalSector> sectors;NGScene::CResourceOpener file("Globals",id);file->Add(4,&sectors);
      for(const auto& sector:sectors)if(auto* name=NDb::GetString(sector.nDescriptionID))
        if(!name->szStr.empty())regions.emplace(sector.nTemplate,name->szStr);
    }
  }
  auto* chapters=NDatabase::GetTable<NDb::CChapterMap>();
  vector<NetworkMapChoice> choices;std::set<int> seen;
  if(!chapters)return choices;
  vector<int> ids;CDBIterator<NDb::CChapterMap> chapter(*chapters);
  while(chapter.MoveNext())ids.push_back(chapter.Get()->GetRecordID());
  std::sort(ids.begin(),ids.end());
  for(int id:ids) {
    if(!NGScene::CResourceFileOpener::DoesExist("Chapters",id))continue;
    vector<SChapterSector> sectors;NGScene::CResourceOpener file("Chapters",id);file->Add(4,&sectors);
    int ordinal=0;
    for(const auto& sector:sectors) {
      if(sector.eType!=RANDOM || sector.pointsSet.empty())continue;
      auto* map=NDb::GetTemplate(sector.nTemplate);if(!map || !IsNetworkMapLargeEnough(map->nWidth,map->nHeight))continue;
      ++ordinal;vector<NDb::CTemplVariant*> variants;
      for(size_t index=0;index<map->variants.size();++index) {
        auto* variant=map->variants[index].GetPtr();
        // Zero-weight variants cannot be rolled by the solo encounter loader.
        if(IsValid(variant) && !variant->bNoAttack && map->roulette.GetSectorValue(static_cast<int>(index))>0)
          variants.push_back(variant);
      }
      std::sort(variants.begin(),variants.end(),[](auto p,auto q){return p->GetRecordID()<q->GetRecordID();});
      for(size_t index=0;index<variants.size();++index) {
        auto* variant=variants[index];if(!seen.insert(variant->GetRecordID()).second)continue;
        NetworkMapChoice choice;choice.templateID=sector.nTemplate;choice.variantID=variant->GetRecordID();
        choice.name=regions.count(id)?regions[id]+L" \u2014 "+std::to_wstring(ordinal):
          L"\u0421\u043b\u0443\u0447\u0430\u0439\u043d\u043e\u0435 \u0441\u0442\u043e\u043b\u043a\u043d\u043e\u0432\u0435\u043d\u0438\u0435 "+std::to_wstring(choices.size()+1);
        if(regions.count(id) && variants.size()>1)choice.name+=L"."+std::to_wstring(index+1);
        choice.name+=L" \u2014 "+std::to_wstring(map->nWidth)+L" \u00d7 "+std::to_wstring(map->nHeight);
        choices.push_back(choice);
      }
    }
  }
  return choices;
}
namespace {
template<class T,class Predicate> T* FirstRecord(Predicate acceptable) {
  auto* table=NDatabase::GetTable<T>();
  if(!table) throw std::runtime_error("missing network equipment table");
  CDBIterator<T> it(*table); T* best=nullptr;
  while(it.MoveNext()) {
    auto* entry=it.Get();
    if(entry && acceptable(entry) && (!best || entry->GetRecordID()<best->GetRecordID())) best=entry;
  }
  if(!best) throw std::runtime_error(std::string("missing usable network equipment: ")+typeid(T).name());
  return best;
}
void SupplementEquipment(NRPG::CGlobalPlayer* squad,const NetworkMatchPreset::Equipment& equipment) {
  auto* grenade=NDb::GetRPGGrenade(equipment.grenadeRecord);
  if(!grenade || !IsValid(grenade->pItem))throw std::runtime_error("network preset grenade is unavailable");
  auto* aid=FirstRecord<NDb::CRPGFirstAid>([](auto* p){return IsValid(p->pItem) && p->effect==NDb::FAE_NORMAL && p->nTotalHealVP>0;});
  auto* mine=FirstRecord<NDb::CRPGMine>([](auto* p){return IsValid(p->pItem) && IsValid(p->pExplosion);});
  auto* tool=FirstRecord<NDb::CRPGTool>([](auto* p){return IsValid(p->pItem) && p->bCanUseForMineCleaning;});
  auto place=[squad](CDBRecord* record,int preferred) {
    CObj<NRPG::IInventoryItem> item=NRPG::CreateItem(record);
    if(!item) throw std::runtime_error("cannot create network preset item");
    for(size_t shift=0;shift<squad->mercs.size();++shift) {
      size_t index=(preferred+shift)%squad->mercs.size();
      auto* inventory=squad->mercs[index]->GetInventory();
      if(inventory->Place(CTPoint<int>(-1,-1),item)) return;
    }
    throw std::runtime_error("network preset equipment does not fit backpacks");
  };
  for(size_t i=0;i<squad->mercs.size();++i) {
    auto* inventory=squad->mercs[i]->GetInventory();
    auto* weapon=dynamic_cast<NRPG::IWeaponItemInfo*>(inventory->Get(NDb::SLOT_1));
    if(!weapon || !weapon->GetInnerClip()) throw std::runtime_error("network persona has no loaded weapon");
    auto* clip=weapon->GetInnerClip();
    for(int spare=0;spare<equipment.spareClipsPerFighter;++spare) {
      CObj<NRPG::IInventoryItem> item=NRPG::CreateClipItem(clip->GetDBClip(),clip->GetDBAmmo());
      if(!inventory->Place(CTPoint<int>(-1,-1),item))
        throw std::runtime_error("network spare ammunition does not fit");
    }
    for(int count=0;count<equipment.grenadesPerFighter;++count)place(grenade,static_cast<int>(i));
  }
  for(int count=0;count<equipment.medicalSupplies;++count)place(aid,2);
  for(int count=0;count<equipment.mines;++count)place(mine,5);
  for(int count=0;count<equipment.clearingTools;++count)place(tool,5);
}
}
namespace {
void ValidatePreset(const NetworkMatchPreset& p) {
  if(p.squadSize<1 || p.squadSize>6 || p.startingLevel<1 || p.startingLevel>20 || p.supplyLevel<1 || p.supplyLevel>14 ||
     p.rules.firstSlot>1 || p.rules.segmentMilliseconds!=50)throw std::runtime_error("unsupported network match rules");
}
bool HumanEquipment(NDb::CRPGItem* item) {
  if(!item || !IsValid(item->pSuccessor))return false;
  auto* record=item->pSuccessor.GetPtr();
  if(auto* weapon=dynamic_cast<NDb::CRPGWeapon*>(record))return weapon->nPanzerkleinType==0 && IsValid(weapon->pWeaponType) && weapon->pWeaponType->eStoreWeaponType!=NDb::SWT_PK_WEAPON;
  if(auto* melee=dynamic_cast<NDb::CRPGMeleeWeapon*>(record))return melee->nPanzerkleinType==0 && melee->GetRecordID()!=1;
  if(auto* grenade=dynamic_cast<NDb::CRPGGrenade*>(record))return grenade->nPanzerkleinWeapon==0;
  if(auto* grenade=dynamic_cast<NDb::CRPGEngGrenade*>(record))return grenade->nPanzerkleinWeapon==0;
  return dynamic_cast<NDb::CRPGMineDetector*>(record) || dynamic_cast<NDb::CRPGFirstAid*>(record) || dynamic_cast<NDb::CRPGMine*>(record) || dynamic_cast<NDb::CRPGTool*>(record) || dynamic_cast<NDb::CRPGPicklock*>(record);
}
void AddSquadPlayers(NetworkMatch& match) {
  for(int slot=0;slot<2;++slot) {
    CObj<NAI::CSequenceCommander> commander=new NAI::CSequenceCommander(match.world);
    match.players[slot]=dynamic_cast<NWorld::CPlayer*>(match.world->AddPlayer(slot?L"Client":L"Host",match.game->players[slot],commander));
    commander->SetPlayer(match.players[slot]);
  }
}
}
vector<int> GetNetworkRecruitIDs() {
  vector<int> result;auto* table=NDatabase::GetTable<NDb::CRPGPers>();if(!table)return result;
  CDBIterator<NDb::CRPGPers> it(*table);
  while(it.MoveNext()) {auto* p=it.Get();if(p && p->bCanHired && IsValid(p->pClass) && IsValid(p->pSide) && p->pSide->GetRecordID()==1 && !IsValid(p->pPanzerklein))result.push_back(p->GetRecordID());}
  std::sort(result.begin(),result.end());return result;
}
vector<int> GetNetworkSupplyItems(int level) {
  if(level<1 || level>14)throw std::runtime_error("invalid network supply level");
  std::set<int> allowed;std::set<int> ammoGroups;
  auto* items=NDatabase::GetTable<NDb::CRPGItem>();if(!items)return {};
  CDBIterator<NDb::CRPGItem> it(*items);
  while(it.MoveNext()) {
    auto* item=it.Get();if(!HumanEquipment(item))continue;
    bool unlocked=level==14;
    if(!unlocked) {
      auto* stores=NDatabase::GetTable<NDb::CRPGStoreItem>();CDBIterator<NDb::CRPGStoreItem> rows(*stores);
      while(rows.MoveNext()) {auto* row=rows.Get();if(row->pItem.GetPtr()==item && IsValid(row->pSide) && row->pSide->GetRecordID()==1 && row->nRating<=level)unlocked=true;}
    }
    if(!unlocked)continue;
    allowed.insert(item->GetRecordID());
    if(auto* w=dynamic_cast<NDb::CRPGWeapon*>(item->pSuccessor.GetPtr()))if(IsValid(w->pInnerClip))ammoGroups.insert(w->pInnerClip->nAmmoGroup);
  }
  CDBIterator<NDb::CRPGItem> clips(*items);
  while(clips.MoveNext())if(auto* clip=dynamic_cast<NDb::CRPGClip*>(clips.Get()->pSuccessor.GetPtr()))
    if(ammoGroups.count(clip->nAmmoGroup))allowed.insert(clips.Get()->GetRecordID());
  return vector<int>(allowed.begin(),allowed.end());
}
void RefreshNetworkStore(NRPG::CGlobalPlayer* player) {
  if(!player || player->networkSupplyLevel<1)return;
  if(!player->pStore)player->pStore=new NRPG::CStore(player);
  auto* store=player->pStore.GetPtr();const int level=player->networkSupplyLevel;
  if(level<14) {store->Update(level,player->mercs);return;}
  // One renewable row per item, independent of campaign factions and prices.
  // Taking any item replenishes its row after the transfer completes.
  auto allowed=GetNetworkSupplyItems(14);
  for(int id:allowed) {
    NRPG::SStoreItem* row=nullptr;
    for(auto& entry:store->ItemsSet())if(entry.pRPGItem && entry.pRPGItem->GetRecordID()==id && entry.eType==NRPG::SStoreItem::CONST_QUANTITY){row=&entry;break;}
    if(!row) {NRPG::SStoreItem entry;entry.nRating=0;entry.eType=NRPG::SStoreItem::CONST_QUANTITY;entry.fQuantity=1;entry.pRPGItem=NDb::GetRPGItem(id);store->ItemsSet().push_back(entry);row=&store->ItemsSet().back();}
    row->fQuantity=1;
    while(row->itemsList.size()<1) {CObj<NRPG::IInventoryItem> item=NRPG::CreateItem(row->pRPGItem->pSuccessor);if(!item)break;row->itemsList.push_back(item);}
  }
  store->Update(0,player->mercs);
}
CObj<NRPG::CUnit> CreateNetworkRecruit(int persona,int level) {
  auto recruits=GetNetworkRecruitIDs();if(std::find(recruits.begin(),recruits.end(),persona)==recruits.end() || level<1 || level>20)throw std::runtime_error("invalid network recruit");
  CObj<NRPG::CUnit> unit=NRPG::CreateMerc(NDb::GetPers(persona),nullptr,true);
  // A fresh network recruit brings no campaign loadout or pre-spent perks.
  auto* inv=unit->GetInventory();for(int slot=0;slot<NDb::N_SLOTS;++slot)inv->TakeOff(static_cast<NDb::ESlot>(slot));
  auto backpack=inv->GetItems();for(const auto& item:backpack)inv->Take(item.pItem);
  unit->SetXPLevel(level);unit->fXP=unit->GetXPForSkill(NDb::ST_LEVEL,level);
  unit->Skills(NDb::ST_LEVEL).SetXPPart(level);unit->Skills(NDb::ST_LEVEL).SetProgress(0);unit->Skills(NDb::ST_LEVEL).Reset();
  auto* tree=unit->GetPerksTree();tree->AddPerkPoints(-tree->GetPerkPoints());tree->AddPerkPoints(level-1);
  return unit;
}
NetworkMatch CreatePreparedNetworkMatch(const NetworkMatchPreset& preset,NRPG::CGlobalPlayer* first,NRPG::CGlobalPlayer* second) {
  ValidatePreset(preset);
  if(!first || !second || first==second || first->mercs.empty() || second->mercs.empty())throw std::runtime_error("invalid independent network squads");
  NetworkMatch match;auto* variant=NDb::GetTemplVariant(preset.mapVariant);
  if(!variant || !IsValid(variant->pTemplate) || variant->bNoAttack)throw std::runtime_error("Network map is unavailable or prohibits combat");
  match.variantID=preset.mapVariant;match.templateID=variant->pTemplate->GetRecordID();
  match.game=NRPG::CreateGlobalGame();match.game->nCurrentTemplateID=match.templateID;
  match.game->players.push_back(first);match.game->players.push_back(second);
  first->networkSupplyLevel=second->networkSupplyLevel=0;
  match.world = new NWorld::CWorld(match.game);
  match.world->bNetworkArena = true;
  match.world->bNetworkEncounterDeployment=preset.useEncounterDeployment;
  CObj<NWorld::CPostWorldCreateInfo> post;
  match.world->CreateRandom(preset.mapVariant, vector<string>(), true,
    list<CPtr<NScenario::CScenarioClue>>(), 0, &post, SRandomSeed(preset.seed));
  if (!post || !post->scripts.empty()) throw std::runtime_error("network map construction failed");
  match.world->PrepareNetworkDeployment(static_cast<int>((std::max)(first->mercs.size(),second->mercs.size())));
  for (int slot = 0; slot < 2; ++slot) {
    CObj<NAI::CSequenceCommander> commander = new NAI::CSequenceCommander(match.world);
    match.players[slot] = dynamic_cast<NWorld::CPlayer*>(match.world->AddPlayer(
      slot ? L"Client" : L"Host", match.game->players[slot], commander));
    commander->SetPlayer(match.players[slot]);
  }
  for (int slot = 0; slot < 2; ++slot) {
    NWorld::CPlayer::CUnitSet units;
    match.players[slot]->GetUnits(&units);
    vector<CPtr<NRPG::IUnitMission>> rpg;
    for (auto unit : units) rpg.push_back(static_cast<NWorld::CUnitServer*>(unit.GetPtr())->GetUnitRPG());
    match.world->GetDiplomacy()->SetDiplomacyState(slot, rpg, 1-slot, NDb::DS_ENEMY);
  }
  match.world->RunPostInit(post);
  match.world->GivePlayerTurn(match.players[preset.rules.firstSlot]);
  match.ambient=match.world->GetDefaultLight();
  return match;
}
NetworkMatch CreateNetworkMatch(const NetworkMatchPreset& preset) {
  ValidatePreset(preset);if(preset.personas.empty())throw std::runtime_error("empty network squad");
  CObj<NRPG::CGlobalPlayer> first=NRPG::CreateGlobalPlayer(preset.personas);
  if(first->mercs.size()!=preset.personas.size())throw std::runtime_error("network preset contains an unavailable persona");
  SupplementEquipment(first,preset.equipment);
  CMemoryStream copy;{CStructureSaver save(copy,CStructureSaver::WRITE);save.Add(1,&first);}copy.Seek(0);
  CObj<NRPG::CGlobalPlayer> second;{CStructureSaver load(copy,CStructureSaver::READ);load.Add(1,&second);}
  return CreatePreparedNetworkMatch(preset,first,second);
}
NetworkMatch CreateNetworkBase(const NetworkMatchPreset& preset,unsigned slot,NRPG::CGlobalPlayer* existing) {
  ValidatePreset(preset);if(slot>1)throw std::runtime_error("invalid base owner");
  NetworkMatch match;match.variantID=5246;auto* variant=NDb::GetTemplVariant(match.variantID);
  if(!variant || !variant->pTemplate)throw std::runtime_error("Network base is unavailable");
  match.templateID=variant->pTemplate->GetRecordID();match.game=NRPG::CreateGlobalGame();match.game->nCurrentTemplateID=match.templateID;
  match.game->nHotSeatTechLevel=preset.supplyLevel;
  for(unsigned side=0;side<2;++side) {
    CObj<NRPG::CGlobalPlayer> squad=side==slot?existing:nullptr;
    if(!squad) {
      squad=NRPG::CreateGlobalPlayer(vector<int>());
      if(side==slot)for(int i=0;i<preset.squadSize;++i)squad->mercs.push_back(CreateNetworkRecruit(preset.personas.at(i),preset.startingLevel));
    }
    if(side==slot){squad->networkSupplyLevel=preset.supplyLevel;RefreshNetworkStore(squad);}
    match.game->players.push_back(squad);
  }
  match.world=new NWorld::CWorld(match.game);match.world->bNetworkArena=true;match.world->bNetworkEncounterDeployment=true;
  CObj<NWorld::CPostWorldCreateInfo> post;
  match.world->CreateRandom(match.variantID,vector<string>(),true,list<CPtr<NScenario::CScenarioClue>>(),0,&post,SRandomSeed(preset.seed));
  if(!post || !post->scripts.empty())throw std::runtime_error("network base construction failed");
  if(match.world->networkSpawnPositions[0].empty())match.world->networkSpawnPositions[0].push_back(CVec3(FP_GRID_STEP,8*FP_GRID_STEP,0));
  match.world->networkSpawnPositions[1]=match.world->networkSpawnPositions[0];
  match.world->PrepareNetworkDeployment(preset.squadSize);
  match.world->networkDeployment[1]=match.world->networkDeployment[0];
  AddSquadPlayers(match);match.world->RunPostInit(post);
  vector<CPtr<NRPG::IUnitMission>> staff;
  for(const char* name:{"BOSS","MED","STORB"}) {
    auto* unit=match.world->GetUnitServer(name);
    if(!unit)throw std::runtime_error("Missing network base service NPC");
    unit->SetCanTalk(true);match.baseServices.push_back(unit);staff.push_back(unit->GetUnitRPG());
  }
  for(int side=0;side<2;++side) {
    match.world->GetDiplomacy()->SetDiplomacyState(2,staff,side,NDb::DS_ALLY);
    vector<CPtr<NRPG::IUnitMission>> party;
    for(auto unit:match.players[side]->GetPlayerUnits())party.push_back(static_cast<NWorld::CUnitServer*>(unit.GetPtr())->GetUnitRPG());
    match.world->GetDiplomacy()->SetDiplomacyState(side,party,2,NDb::DS_ALLY);
  }
  match.ambient=match.world->GetDefaultLight();
  return match;
}
int NetworkMatch::Winner() const {
  if (!world || world->IsExecuting()) return -2;
  bool alive0 = players[0] && players[0]->HasAlivePeople();
  bool alive1 = players[1] && players[1]->HasAlivePeople();
  if (alive0 && alive1) return -2;
  return alive0 ? 0 : alive1 ? 1 : -1;
}
}
