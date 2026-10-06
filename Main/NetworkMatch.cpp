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
#include "wUnitServer.h"
#include "../DBFormat/DataRPG.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataMap.h"
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
      auto* map=NDb::GetTemplate(sector.nTemplate);if(!map)continue;
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
NetworkMatch CreateNetworkMatch(const NetworkMatchPreset& preset) {
  if(preset.personas.empty() || preset.rules.firstSlot>1 || preset.rules.segmentMilliseconds!=50)
    throw std::runtime_error("unsupported network match rules");
  NetworkMatch match;
  auto* variant=NDb::GetTemplVariant(preset.mapVariant);
  if(!variant || !IsValid(variant->pTemplate) || variant->bNoAttack)
    throw std::runtime_error("Network map is unavailable or prohibits combat");
  match.variantID=preset.mapVariant;match.templateID=variant->pTemplate->GetRecordID();
  match.game = NRPG::CreateGlobalGame();
  match.game->nCurrentTemplateID=match.templateID;
  CObj<NRPG::CGlobalPlayer> first = NRPG::CreateGlobalPlayer(preset.personas);
  if (first->mercs.size() != preset.personas.size())
    throw std::runtime_error("network preset contains an unavailable persona");
  SupplementEquipment(first,preset.equipment);
  match.game->players.push_back(first);
  CMemoryStream copy;
  { CStructureSaver save(copy, CStructureSaver::WRITE); save.Add(1,&first); }
  copy.Seek(0);
  CObj<NRPG::CGlobalPlayer> second;
  { CStructureSaver load(copy, CStructureSaver::READ); load.Add(1,&second); }
  match.game->players.push_back(second);
  match.world = new NWorld::CWorld(match.game);
  match.world->bNetworkArena = true;
  match.world->bNetworkEncounterDeployment=preset.useEncounterDeployment;
  CObj<NWorld::CPostWorldCreateInfo> post;
  match.world->CreateRandom(preset.mapVariant, vector<string>(), true,
    list<CPtr<NScenario::CScenarioClue>>(), 0, &post, SRandomSeed(preset.seed));
  if (!post || !post->scripts.empty()) throw std::runtime_error("network map construction failed");
  match.world->PrepareNetworkDeployment(static_cast<int>(preset.personas.size()));
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
