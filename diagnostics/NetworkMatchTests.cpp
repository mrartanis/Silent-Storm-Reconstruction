#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/NetworkMatch.h"
#include "../Main/NetworkWorld.h"
#include "../Main/aiGrid.h"
#include "../Main/GSceneUtils.h"
#include "../Main/wUnitServer.h"
#include "../Main/wUnitCommands.h"
#include "../Main/RPGUnitMission.h"
#include "../Main/RPGUnit.h"
#include "../Main/RPGStore.h"
#include "../Main/rpgPerk.h"
#include "../DBFormat/DataPerk.h"
#include "../Main/RPGItem.h"
#include "../Main/wUICommands.h"
#include "../Main/wExplTracker.h"
#include "../Main/BuildingGrid.h"
#include "../Main/wBuilding.h"
#include "../Main/MakeBuilding.h"
#include "../Main/wMine.h"
#include "../Main/wObject.h"
#include "../Main/wInterfaceVisitors.h"
#include "../DBFormat/DataGeometry.h"
#include "../Main/wUnitStates.h"
#include "../Main/NetworkIdentity.h"
#include "../Main/wBuilding.h"
#include "../Main/MakeBuilding.h"
#include "../Misc/RandomGen.h"
#include <cstdlib>
#include "../Main/rpgDiplomacy.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataRPG.h"
#include "../DBFormat/DataMap.h"
#include <cstdio>
#include <set>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
static void Require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
class ObjectRenderProbe : public CSyncDst<NWorld::IVisObj>, public NWorld::IRenderVisitor {
  NWorld::IVisObj* target;
  void VisitObject(int, NWorld::IVisObj* object) override {
    if(object==target) { ++visits;models.clear();object->Visit(this); }
  }
public:
  int visits=0;
  vector<int> models;
  ObjectRenderProbe(NWorld::CWorld* world,NWorld::IVisObj* object) : CSyncDst(world->GetActive()),target(object) {}
  void AddMesh(NDb::CModel* model,const SFBTransform&,NGScene::CLightGroup*,int,int) override {
    if(model) {
      models.push_back(model->pGeometry ? model->pGeometry->GetRecordID() : 0);
      for(int i=0;i<NDb::N_MODEL_MATERIALS;++i)models.push_back(model->pMaterials[i] ? model->pMaterials[i]->GetRecordID() : 0);
    }
  }
};
int main(int argc,char** argv) {
  if (argc != 3 && argc != 4 && argc != 5 && argc != 6) return 2;
  try {
    std::setvbuf(stdout,nullptr,_IONBF,0);
    // The grenade/destruction scenario must replay the same real trajectory:
    // a clock-seeded throw can legitimately miss the selected building.
    GlobalGameRandom().SeedForHarness(42);
    std::srand(42);
    CFileStream db; db.OpenRead(argv[1]);
    NDatabase::Serialize(db,CStructureSaver::READ);
    NGScene::AddResourceDir(argv[2]);
    if(argc==4 && std::string(argv[3])=="--object-stages") {
      S2Net::NetworkWorld host(true),client(false);client.Apply(host.Initial(),true);
      auto remote=client.Match().world->GetObjectsForHarness().begin();
      int checked=0;
      for(auto object:host.Match().world->GetObjectsForHarness()) {
        auto* replica=remote++->GetPtr();
        auto* db=object->GetDBObjectForHarness();
        if(!db || db->GetStagesQuantity()<3 || object->GetDestroyStage()!=0)continue;
        ObjectRenderProbe local(host.Match().world,object),peer(client.Match().world,replica);
        local.Sync();peer.Sync();
        Require(local.models==peer.models,"initial object presentation differs");
        for(int stage=1;stage<db->GetStagesQuantity();++stage) {
          object->SetDestroyStage(stage);
          client.Apply(host.Segment(),false);local.Sync();int before=peer.visits;peer.Sync();
          std::printf("STAGE CHECK db=%d stage=%d remote=%d localvisits=%d peervisits=%d previous=%d\n",object->GetDBObjectID(),stage,replica->GetDestroyStage(),local.visits,peer.visits,before);
          Require(replica->GetDestroyStage()==object->GetDestroyStage(),"object stage did not replicate");
          Require(peer.visits>before,"object renderer missed intermediate damage stage");
          Require(local.models==peer.models,"damaged object model differs on client");
          for(int i=0;i<4;++i){client.Apply(host.Segment(),false);local.Sync();peer.Sync();}
          std::printf("OBJECT db=%d stage=%d models=%zu visits=%d\n",object->GetDBObjectID(),stage,peer.models.size(),peer.visits);
        }
        if(++checked==3)break;
      }
      Require(checked>0,"no multistage object tested");
      std::puts("Intermediate object damage presentation passed");return 0;
    }
    if(argc==4 && std::string(argv[3])=="--preparation") {
      Require(!S2Net::IsNetworkMapLargeEnough(16,16) && !S2Net::IsNetworkMapLargeEnough(32,32),"small maps retained");
      Require(!S2Net::IsNetworkMapLargeEnough(16,128) && S2Net::IsNetworkMapLargeEnough(64,64),"map dimensions filter");
      auto maps=S2Net::GetNetworkEncounterMaps();Require(!maps.empty(),"filter removed every encounter");
      for(auto map:maps){auto* t=NDb::GetTemplate(map.templateID);Require(S2Net::IsNetworkMapLargeEnough(t->nWidth,t->nHeight),"small encounter offered");}
      std::printf("PREPARATION selectable large maps=%zu first=%d\n",maps.size(),maps.front().variantID);std::fflush(stdout);
      std::set<int> previous;
      for(int level=1;level<=14;++level) {
        auto items=S2Net::GetNetworkSupplyItems(level);std::set<int> current(items.begin(),items.end());
        Require(!current.empty() && std::includes(current.begin(),current.end(),previous.begin(),previous.end()),"supply is not monotonic");
        for(int id:items) {
          auto* item=NDb::GetRPGItem(id);auto* record=item->pSuccessor.GetPtr();
          Require(!dynamic_cast<NDb::CPanzerklein*>(record),"panzerklein unlocked");
          if(auto* w=dynamic_cast<NDb::CRPGWeapon*>(record))Require(w->nPanzerkleinType==0 && w->pWeaponType->eStoreWeaponType!=NDb::SWT_PK_WEAPON,"PK weapon unlocked");
        }
        previous=current;std::printf("SUPPLY level=%d items=%zu\n",level,items.size());
      }
      auto recruits=S2Net::GetNetworkRecruitIDs();Require(recruits.size()>=6,"missing recruits");
      for(int id:recruits)for(int level:{1,5,10,20}) {
        auto unit=S2Net::CreateNetworkRecruit(id,level);
        if(level==1 && unit->GetRPGPersID()!=id)std::printf("PERSONA record=%d gameplay-id=%d\n",id,unit->GetRPGPersID());
        Require(unit->Skills(NDb::ST_LEVEL)==level && unit->GetPerksTree()->GetPerkPoints()==level-1,"starting level/perk points");
        Require(unit->GetInventory()->GetItems().empty() && !unit->GetInventory()->Get(NDb::SLOT_1) && !unit->GetInventory()->Get(NDb::SLOT_2),"recruit smuggled equipment");
        Require(unit->fXP==unit->GetXPForSkill(NDb::ST_LEVEL,level),"starting XP mismatch");
        int before=unit->Skills(NDb::ST_LEVEL);unit->AddXP(1);Require(unit->Skills(NDb::ST_LEVEL)==before,"first XP gain skipped levels");
      }
      {auto low=S2Net::CreateNetworkRecruit(recruits.front(),1),high=S2Net::CreateNetworkRecruit(recruits.front(),20);Require(high->Skills(NDb::ST_SHOOTING)>low->Skills(NDb::ST_SHOOTING),"level did not improve skills");}
      // Real worlds and replica; same commands as the native panels and network UI.
      for(int count:{1,6}) {
        S2Net::NetworkMatchPreset preset;preset.prepareOnBase=true;preset.squadSize=count;preset.startingLevel=5;preset.supplyLevel=14;
        preset.mapVariant=maps.front().variantID;preset.useEncounterDeployment=true;
        S2Net::NetworkWorld host(true,preset),client(false);client.Apply(host.Initial(),true);
        auto code=[](const S2Net::Bytes& bytes){size_t pos=8;return S2Net::Get32(bytes,pos);};
        auto sync=[&]{auto delta=host.Segment();client.Apply(delta,false);for(auto event:host.DrainEvents()){host.PresentEvent(event);client.PresentEvent(event);}
          if(host.IsPreparing())while(CObj<NWorld::CUICmd> ui=client.Match().world->GetUICommand())host.Handle(1,client.EncodeCommand(new NWorld::CCmdInterfaceEvent(ui->GetID())),false);
          while(CObj<NWorld::CUICmd> ui=host.SlotMatch(0).world->GetUICommand())host.Handle(0,host.EncodeCommand(new NWorld::CCmdInterfaceEvent(ui->GetID())),false);};
        auto settled=[&]{for(int i=0;i<300;++i){sync();if(i>=30 && !host.SlotMatch(0).world->IsExecuting() && !host.SlotMatch(1).world->IsExecuting())return;}throw std::runtime_error("base actions never settled");};
        Require(host.IsPreparing() && client.IsPreparing(),"base phase not replicated");
        auto* replicaPaths=dynamic_cast<NAI::CPathNetwork*>(client.Match().world->GetPathNetwork());
        auto* hostPaths=dynamic_cast<NAI::CPathNetwork*>(host.SlotMatch(1).world->GetPathNetwork());
        Require(replicaPaths && hostPaths && !replicaPaths->GetWaysCalcer() && hostPaths->GetWaysCalcer(),"path search cache leaked to replica or removed on host");
        for(int layer=0;layer<replicaPaths->GetNumLayers();++layer)
          Require(!replicaPaths->GetColourer(layer) && hostPaths->GetColourer(layer),"path colour cache leaked to replica or removed on host");
        Require(client.Match().ambient && client.Match().ambient.GetPtr()!=host.SlotMatch(1).ambient.GetPtr(),"ambient light missing on replica");
        auto serverBuildings=host.SlotMatch(1).world->GetBuildingsForHarness();
        auto clientBuildings=client.Match().world->GetBuildingsForHarness();
        Require(serverBuildings.size()==clientBuildings.size(),"base buildings lost");
        size_t roomLights=0;auto replicaBuilding=clientBuildings.begin();
        for(auto building:serverBuildings) {
          auto clientBuilding=*replicaBuilding++;
          auto* shell=dynamic_cast<NBuilding::CBuildingInfoHold*>(clientBuilding->GetSceneHandle());
          Require(shell && !shell->GetInfo().info.empty(),"replica building shell missing");
          const auto& lights=building->info.lights;const auto& replicaLights=clientBuilding->info.lights;
          Require(lights.size()==replicaLights.size(),"room lights missing on replica");roomLights+=lights.size();
          for(size_t i=0;i<lights.size();++i)Require(lights[i].color==replicaLights[i].color && lights[i].nFloor==replicaLights[i].nFloor && lights[i].bLightmap==replicaLights[i].bLightmap,"room light changed on replica");
        }
        // Some base variants use only the global ambient. Exercise the
        // network-only local light field with a nonempty room-light fixture.
        SMapBuilding::SAmbientLight sample;sample.color=CVec3(.2f,.3f,.4f);sample.nFloor=0;sample.bLightmap=true;
        host.SlotMatch(1).world->GetBuildingsForHarness().front()->info.lights.push_back(sample);
        sync();
        const auto& receivedLights=client.Match().world->GetBuildingsForHarness().front()->info.lights;
        Require(!receivedLights.empty() && receivedLights.back().color==sample.color,"room light delta lost");
        Require(client.Match().baseServices.size()==3,"base staff not replicated");
        for(auto staff:client.Match().baseServices)Require(staff && staff->CanTalk(),"base staff cannot interact");
        Require(host.SlotMatch(0).world.GetPtr()!=host.SlotMatch(1).world.GetPtr(),"bases share a world");
        for(unsigned slot=0;slot<2;++slot) {
          auto& base=host.SlotMatch(slot);Require(base.variantID==5246 && base.game->players[slot]->mercs.size()==size_t(count),"base roster size");
          Require(base.players[1-slot]->GetPlayerUnits().empty(),"opponent spawned in private base");
          Require(!base.world->GetCurrentPlayer(),"base is not real time");
          Require(base.baseServices.size()==3,"base staff missing");
          for(auto staff:base.baseServices)Require(base.world->GetDiplomacyState(base.players[slot],staff->GetPlayer())==NDb::DS_ALLY,"staff is not friendly");
          Require(code(host.Handle(slot,host.PreparationCommand(S2Net::PreparationAction::Heal),false))==NWorld::UCR_OK,"medic unavailable");
          auto* unit=base.players[slot]->GetPlayerUnits().front().GetPtr();
          auto* inventory=unit->GetUnitRPG()->GetInventory();
          vector<CPtr<NRPG::IInventoryItem>> stock;base.game->players[slot]->pStore->GetItemsForDiagnostics(&stock);
          NRPG::IInventoryItem* weapon=nullptr;
          for(auto item:stock)if(dynamic_cast<NRPG::IWeaponItem*>(item.GetPtr())){weapon=item;break;}
          Require(weapon!=nullptr,"free supply has no weapon");int itemID=weapon->GetDBItem()->GetRecordID();
          NWorld::SItem src(nullptr,NWorld::SItem::STORAGE,weapon,base.players[slot]),hand(nullptr,NWorld::SItem::HAND),dst(unit,NWorld::SItem::SLOT,NDb::SLOT_1);
          auto request=host.EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdMoveInventoryItem(src,hand)));
          Require(code(host.Handle(slot,request,false))==NWorld::UCR_OK,"native store pickup rejected");host.Handle(slot,host.EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdContinue)),false);settled();
          NWorld::SItem held;
          Require(base.players[slot]->GetInHandItem(&held) && held.pUnit==unit && held.pItem && held.pItem->GetDBItem()->GetRecordID()==itemID,"store pickup lost cursor owner");
          Require(code(host.Handle(slot,host.PreparationCommand(S2Net::PreparationAction::Ready,0,1),false))!=NWorld::UCR_OK,"ready accepted with held item");
          // The native drag state sends HAND with a null item, resolving it on the server.
          hand.pUnit=unit;
          request=host.EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdMoveInventoryItem(hand,dst)));
          Require(code(host.Handle(slot,request,false))==NWorld::UCR_OK,"native cursor drop rejected");host.Handle(slot,host.EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdContinue)),false);settled();
          Require(!base.players[slot]->GetInHandItem(&held),"cursor drop left an item held");
          Require(inventory->Get(NDb::SLOT_1) && inventory->Get(NDb::SLOT_1)->GetDBItem()->GetRecordID()==itemID,"native store transfer did not execute");
          Require(code(host.Handle(1-slot,request,false))!=NWorld::UCR_OK,"opponent can equip another squad");
          vector<CPtr<NRPG::CPerk>> perks;unit->GetUnitRPG()->GetRPGUnit()->GetPerksTree()->GetAvailablePerks(&perks);Require(!perks.empty(),"no starting perks");
          request=host.EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdTakePerk(perks.front()->GetDBPerk()->GetRecordID())));
          Require(code(host.Handle(slot,request,false))==NWorld::UCR_OK,"perk request rejected");host.Handle(slot,host.EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdContinue)),false);settled();
          Require(unit->GetUnitRPG()->GetRPGUnit()->GetPerksTree()->GetPerkPoints()==3,"perk was not spent exactly once");
          auto before=unit->GetPosition().GetCP();auto point=unit->GetPosition().pos;
          bool moved=false;for(int d=0;d<8 && !moved;++d) {
            auto target=point;int x=int(point.p.GetX())+NAI::nMoveShift[d][0],y=int(point.p.GetY())+NAI::nMoveShift[d][1];if(x<0 || y<0)continue;target.p.SetXY(x,y);target.p.SetFinal(0);
            request=host.EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdPath(target)));
            if(code(host.Handle(slot,request,false))==NWorld::UCR_OK){host.Handle(slot,host.EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdContinue)),false);settled();moved=unit->GetPosition().GetCP()!=before;}
          }
          Require(moved,"could not walk on base");
        }
        // Repeated edits on both private bases must not replace the map or leave
        // stale actors linked to a different global mercenary.
        for(int edit=0;edit<12;++edit) {
          unsigned side=count>1 ? edit%2 : 0;auto& current=host.SlotMatch(side);auto* originalWorld=current.world.GetPtr();
          // Keep client slot 0 equipped for the transition assertions below.
          auto* party=current.game->players[side].GetPtr();int at=side ? 1+(edit/2)%(count-1) : (edit/2)%count;
          int duplicate=party->mercs[(at+1)%count]->GetPers()->GetRecordID();
          Require(code(host.Handle(side,host.PreparationCommand(S2Net::PreparationAction::ChooseRecruit,at,duplicate),false))!=NWorld::UCR_OK,"duplicate recruit accepted");
          int pick=0;for(int id:recruits){bool used=false;for(auto merc:party->mercs)if(merc->GetPers()->GetRecordID()==id)used=true;if(!used){pick=id;break;}}
          Require(code(host.Handle(side,host.PreparationCommand(S2Net::PreparationAction::ChooseRecruit,at,pick),false))==NWorld::UCR_OK,"repeated replacement rejected");sync();
          Require(current.world.GetPtr()==originalWorld,"recruit replacement reloaded base");
          Require(S2Net::IsNetworkSquadValid(party,count),"replacement invalidated roster");
          Require(current.players[side]->GetPlayerUnits().size()==size_t(count),"replacement lost an actor");
          for(int slot=0;slot<count;++slot)Require(current.players[side]->GetPlayerUnits()[slot]->GetUnitRPG()->GetRPGUnit()==party->mercs[slot].GetPtr(),"actor/roster identity mismatch");
          if(side==1)for(int slot=0;slot<count;++slot)Require(client.Match().players[1]->GetPlayerUnits()[slot]->GetUnitRPG()->GetRPGUnit()==client.Match().game->players[1]->mercs[slot].GetPtr(),"replica actor/roster identity mismatch");
        }
        // Readiness is independent, and rejected input never changes a roster.
        auto stale=host.PreparationCommand(S2Net::PreparationAction::Ready,0,1);
        Require(code(host.Handle(0,host.PreparationCommand(S2Net::PreparationAction::ChooseRecruit,0,999999),false))!=NWorld::UCR_OK,"invalid recruit accepted");
        Require(code(host.Handle(0,stale,false))==NWorld::UCR_OK,"first readiness rejected");sync();Require(client.IsReady(0) && !client.IsReady(1),"readiness not replicated");
        int replacement=0;for(int id:recruits){bool used=false;for(auto unit:host.SlotMatch(0).game->players[0]->mercs)if(unit->GetPers()->GetRecordID()==id)used=true;if(!used){replacement=id;break;}}
        Require(code(host.Handle(0,host.PreparationCommand(S2Net::PreparationAction::ChooseRecruit,0,replacement),false))==NWorld::UCR_OK,"roster selection rejected");sync();
        Require(!host.IsReady(0) && host.SlotMatch(0).game->players[0]->mercs.front()->GetPers()->GetRecordID()==replacement,"roster change did not reset ready");
        Require(code(host.Handle(0,stale,false))!=NWorld::UCR_OK,"stale preparation command accepted");
        Require(code(host.Handle(0,host.PreparationCommand(S2Net::PreparationAction::Ready,0,1),false))==NWorld::UCR_OK,"host ready rejected");
        Require(code(host.Handle(1,client.PreparationCommand(S2Net::PreparationAction::Ready,0,1),false))==NWorld::UCR_OK,"client ready rejected");
        sync();Require(host.IsStartingBattle() && client.IsStartingBattle(),"match start not announced to both sides");
        Require(code(host.Handle(1,client.PreparationCommand(S2Net::PreparationAction::ChooseRecruit,0,recruits.front()),false))!=NWorld::UCR_OK,"roster changed during launch");
        sync();Require(!host.IsPreparing() && !client.IsPreparing(),"did not launch battle");
        Require(host.Match().variantID==preset.mapVariant && client.Match().variantID==preset.mapVariant,"battle map not preserved");
        for(unsigned side=0;side<2;++side)Require(host.Match().players[side]->GetPlayerUnits().size()==size_t(count),"battle lost roster");
        Require(host.Match().game->players[1]->mercs.front()->GetPerksTree()->GetPerkPoints()==3,"battle lost purchased perk");
        Require(host.Match().game->players[1]->mercs.front()->GetInventory()->Get(NDb::SLOT_1)!=nullptr,"battle lost equipment");
        Require(host.Match().game->players[0]->networkSupplyLevel==0,"base supply remained active in battle");
        Require(code(host.Handle(0,host.PreparationCommand(S2Net::PreparationAction::Ready,0,1),false))!=NWorld::UCR_OK,"preparation reopened during battle");
        // A completed transition must reopen tactical commands on both sides,
        // rather than leaving the one-frame launch barrier permanently closed.
        auto* battleUnit=host.Match().players[0]->GetPlayerUnits().front().GetPtr();
        auto oldPosition=battleUnit->GetPosition().GetCP();auto battlePoint=battleUnit->GetPosition().pos;
        bool battleMoved=false;
        for(int d=0;d<8 && !battleMoved;++d) {
          auto target=battlePoint;int x=int(battlePoint.p.GetX())+NAI::nMoveShift[d][0],y=int(battlePoint.p.GetY())+NAI::nMoveShift[d][1];if(x<0 || y<0)continue;target.p.SetXY(x,y);target.p.SetFinal(0);
          auto request=host.EncodeCommand(new NWorld::CCmdSetCommand(battleUnit,new NWorld::CCmdPath(target)));
          if(code(host.Handle(0,request,false))==NWorld::UCR_OK){host.Handle(0,host.EncodeCommand(new NWorld::CCmdSetCommand(battleUnit,new NWorld::CCmdContinue)),false);settled();battleMoved=battleUnit->GetPosition().GetCP()!=oldPosition;}
        }
        Require(battleMoved,"tactical movement blocked after preparation");
        Require(code(host.Handle(0,host.EncodeCommand(new NWorld::CCmdEndOfTurn),false))==NWorld::UCR_OK,"end turn blocked after preparation");settled();
        Require(host.Match().world->GetCurrentPlayer()==host.Match().players[1].GetPtr() && client.Match().world->GetCurrentPlayer()==client.Match().players[1].GetPtr(),"post-base turn did not replicate");
        auto* clientUnit=client.Match().players[1]->GetPlayerUnits().front().GetPtr();
        Require(code(host.Handle(1,client.EncodeCommand(new NWorld::CCmdSetCommand(clientUnit,new NWorld::CCmdWishPose(NAI::CROUCH))),false))==NWorld::UCR_OK,"client commands blocked after preparation");
        for(int i=0;i<5;++i)sync();
        for(unsigned side=0;side<2;++side)for(int at=0;at<count;++at) {
          Require(host.Match().players[side]->GetPlayerUnits()[at]->GetUnitRPG()->GetRPGUnit()==host.Match().game->players[side]->mercs[at].GetPtr(),"battle actor/global identity mismatch");
          Require(client.Match().players[side]->GetPlayerUnits()[at]->GetUnitRPG()->GetRPGUnit()==client.Match().game->players[side]->mercs[at].GetPtr(),"battle replica/global identity mismatch");
        }
        // A replaced recruit must retain the same identity in HUD/global state
        // when damaged, rather than leaving a healthy orphan on the base.
        auto* changed=host.Match().players[0]->GetPlayerUnits().front().GetPtr();
        changed->GetUnitRPG()->MakeDirectDamage(100000);
        sync();
        auto* remoteChanged=client.Match().players[0]->GetPlayerUnits().front().GetPtr();
        Require(changed->GetUnitRPG()->GetTotalVP()==remoteChanged->GetUnitRPG()->GetTotalVP(),"replaced fighter damage differs on replica");
        Require(remoteChanged->GetUnitRPG()->GetRPGUnit()==client.Match().game->players[0]->mercs[0].GetPtr(),"HUD retained healthy orphan recruit");
        std::printf("PREPARATION PASSED count=%d base walk/equip/perks/ownership/ready/roster/transition/replica/damage\n",count);std::fflush(stdout);
      }
      std::puts("Network preparation tests passed");return 0;
    }
    if(argc==4 && std::string(argv[3])=="--stop-resume") {
      S2Net::NetworkWorld host(true),client(false);client.Apply(host.Initial(),true);
      auto sync=[&]{client.Apply(host.Segment(),false);};
      auto code=[](const S2Net::Bytes& bytes){size_t pos=8;return S2Net::Get32(bytes,pos);};
      for(int i=0;i<20;++i)sync();
      host.Match().world->GivePlayerTurn(host.Match().players[1]);sync();
      Require(code(host.Handle(0,host.StopActionCommand(),false))!=NWorld::UCR_OK,"opponent stopped active player's action");
      Require(code(host.Handle(1,client.StopActionCommand(),true))!=NWorld::UCR_OK,"stop preview mutated commander");
      auto* actor=host.Match().players[1]->GetPlayerUnits().front().GetPtr();
      auto* replica=client.Match().players[1]->GetPlayerUnits().front().GetPtr();
      auto before=actor->GetPosition();NAI::SPosition destination;bool found=false;
      auto* net=dynamic_cast<NAI::CPathNetwork*>(host.Match().world->GetPathNetwork());
      auto& grid=net->GetLayer(before.pos.p.GetLayer())->tiles;
      for(int distance:{12,16,24})for(auto delta:{CTPoint<int>(distance,0),CTPoint<int>(0,distance),CTPoint<int>(-distance,0),CTPoint<int>(0,-distance)}) {
        int x=int(before.pos.p.GetX())+delta.x,y=int(before.pos.p.GetY())+delta.y;
        if(x<0 || y<0 || x>=grid.GetXSize() || y>=grid.GetYSize())continue;
        auto target=before.pos;target.p.SetXY(x,y);target.p.SetFinal(0);
        if(!net->IsNativePassable(target.p) || net->IsLocked(target.p))continue;
        CObj<NWorld::CCmd> path=new NWorld::CCmdPath(target);int start=0,full=0;
        auto result=actor->CanDo(path,&start,&full);
        if((result==NWorld::UCR_OK || result==NWorld::UCR_NOT_ENOUGH_AP) && full>8 && start<actor->GetAP()){destination=target;found=true;break;}
      }
      Require(found,"no long route for stop/resume fixture");
      auto target=replica->GetPosition().pos;target.p=destination.p;
      Require(code(host.Handle(1,client.EncodeCommand(new NWorld::CCmdSetCommand(replica,new NWorld::CCmdPath(target))),false))==NWorld::UCR_OK,"stop fixture move rejected");
      host.Handle(1,client.EncodeCommand(new NWorld::CCmdSetCommand(replica,new NWorld::CCmdContinue)),false);
      for(int i=0;i<20 && !actor->IsPerformingAction();++i)sync();
      Require(actor->IsPerformingAction(),"stop fixture never started");
      Require(code(host.Handle(1,client.StopActionCommand(),false))==NWorld::UCR_OK,"native stop rejected");
      for(int i=0;i<120;++i)sync();
      Require(!actor->IsPerformingAction() && actor->HasCommand() && actor->GetCurrentPath(),"Stop discarded the order needed by Continue");
      auto stopped=actor->GetPosition();int ap=actor->GetAP();
      for(int i=0;i<20;++i)sync();
      Require(actor->GetPosition().pos.p.GetBits()==stopped.pos.p.GetBits(),"stopped route resumed without a click");
      auto resume=client.EncodeCommand(new NWorld::CCmdSetCommand(replica,new NWorld::CCmdContinue));
      Require(code(host.Handle(1,resume,true))==NWorld::UCR_OK,"Continue unavailable after Stop");
      Require(code(host.Handle(1,resume,false))==NWorld::UCR_OK,"Continue rejected after Stop");
      for(int i=0;i<160;++i){sync();if(actor->GetPosition().pos.p.GetBits()!=stopped.pos.p.GetBits() && !actor->IsPerformingAction())break;}
      Require(actor->GetPosition().pos.p.GetBits()!=stopped.pos.p.GetBits() && actor->GetAP()<ap,"Continue did not resume stopped route");
      Require(replica->GetPosition().pos.p.GetBits()==actor->GetPosition().pos.p.GetBits() && replica->GetAP()==actor->GetAP(),"continued route replica differs");
      std::puts("Network Stop preserves route and Continue resumes it");return 0;
    }
    if(argc==4 && std::string(argv[3])=="--partial-move") {
      S2Net::NetworkWorld host(true),client(false);client.Apply(host.Initial(),true);
      for(int tick=0;tick<20;++tick)client.Apply(host.Segment(),false);
      host.Match().world->GivePlayerTurn(host.Match().players[1]);
      auto* actor=host.Match().players[1]->GetPlayerUnits().front().GetPtr();
      auto before=actor->GetPosition();NAI::SPosition destination;int start=0,full=0;bool found=false;
      for(int distance:{8,12,16,24})for(auto delta:{CTPoint<int>(distance,0),CTPoint<int>(0,distance),CTPoint<int>(-distance,0),CTPoint<int>(0,-distance)}) {
        auto* net=dynamic_cast<NAI::CPathNetwork*>(host.Match().world->GetPathNetwork());
        auto& grid=net->GetLayer(before.pos.p.GetLayer())->tiles;
        int x=int(before.pos.p.GetX())+delta.x,y=int(before.pos.p.GetY())+delta.y;
        if(x<0 || y<0 || x>=grid.GetXSize() || y>=grid.GetYSize())continue;
        auto target=before.pos;target.p.SetXY(target.p.GetX()+delta.x,target.p.GetY()+delta.y);target.p.SetFinal(0);
        if(!net->IsNativePassable(target.p) || net->IsLocked(target.p))continue;
        CObj<NWorld::CCmd> path=new NWorld::CCmdPath(target,NAI::PF_DEFAULT);
        if(actor->CanDo(path,&start,&full)==NWorld::UCR_OK && full>start+4) {destination=target;found=true;break;}
      }
      Require(found,"no affordable-first-step long route for fixture");
      CObj<NWorld::CCmd> selectedRoute=new NWorld::CCmdPath(destination,NAI::PF_DEFAULT);
      actor->CanDo(selectedRoute,&start,&full);
      int budget=start+2;actor->GetUnitRPG()->SpendAP(actor->GetAP()-budget);
      client.Apply(host.Segment(),false);
      auto* replica=client.Match().players[1]->GetPlayerUnits().front().GetPtr();
      Require(actor->CanDo(selectedRoute,&start,&full)==NWorld::UCR_NOT_ENOUGH_AP,"fixture route fits in this turn");
      auto remoteDestination=replica->GetPosition().pos;remoteDestination.p=destination.p;
      CObj<NWorld::CCmd> route=new NWorld::CCmdPath(remoteDestination,NAI::PF_DEFAULT);
      auto result=host.Handle(1,client.EncodeCommand(new NWorld::CCmdSetCommand(replica,route)),false);size_t pos=8;
      Require(S2Net::Get32(result,pos)==NWorld::UCR_OK,"long move rejected instead of queued");
      host.Handle(1,client.EncodeCommand(new NWorld::CCmdSetCommand(replica,new NWorld::CCmdContinue)),false);
      for(int tick=0;tick<180;++tick) {
        client.Apply(host.Segment(),false);
        if(actor->GetPosition().pos.p.GetBits()!=before.pos.p.GetBits() && !host.Match().world->IsExecuting())break;
      }
      Require(actor->GetPosition().pos.p.GetBits()!=before.pos.p.GetBits(),"partial move made no progress");
      Require(actor->GetAP()>=0 && actor->GetAP()<budget,"partial move did not respect AP budget");
      Require(actor->GetPosition().pos.p.GetBits()!=destination.p.GetBits(),"unaffordable destination reached");
      Require(replica->GetPosition().pos.p.GetBits()==actor->GetPosition().pos.p.GetBits() && replica->GetAP()==actor->GetAP(),"partial move replica differs");
      Require(actor->GetCurrentPath()!=nullptr,"partial route was discarded");
      actor->GetUnitRPG()->StartNewTurn(actor->GetPosition().GetCP());
      CObj<NWorld::CCmdContinue> resume=new NWorld::CCmdContinue;
      actor->CanDo(resume,&start,&full);
      budget=start+2;actor->GetUnitRPG()->SpendAP(actor->GetAP()-budget);
      client.Apply(host.Segment(),false);
      Require(actor->CanDo(resume,&start,&full)==NWorld::UCR_NOT_ENOUGH_AP,"continued route already fits in budget");
      before=actor->GetPosition();
      auto request=client.EncodeCommand(new NWorld::CCmdSetCommand(replica,resume));
      auto preview=host.Handle(1,request,true);pos=8;
      Require(S2Net::Get32(preview,pos)==NWorld::UCR_OK,"continue button disabled for affordable partial path");
      auto resumed=host.Handle(1,request,false);pos=8;Require(S2Net::Get32(resumed,pos)==NWorld::UCR_OK,"continue rejected");
      for(int tick=0;tick<180;++tick) {
        client.Apply(host.Segment(),false);
        if(actor->GetPosition().pos.p.GetBits()!=before.pos.p.GetBits() && !host.Match().world->IsExecuting())break;
      }
      Require(actor->GetPosition().pos.p.GetBits()!=before.pos.p.GetBits(),"continue made no progress");
      Require(actor->GetAP()>=0 && actor->GetAP()<budget,"continued movement exceeded AP budget");
      std::printf("Partial movement respects AP: budget=%d remaining=%d total-route=%d\n",budget,actor->GetAP(),full);return 0;
    }
    if(argc>=4 && std::string(argv[3])=="--maps") {
      auto maps=S2Net::GetNetworkEncounterMaps();Require(!maps.empty(),"no encounter maps");
      std::set<int> variants;
      size_t tested=0;
      for(const auto& map:maps) {
        if(argc==5 && map.variantID!=std::atoi(argv[4]))continue;
        Require(variants.insert(map.variantID).second,"duplicate encounter variant");
        std::printf("ENCOUNTER template=%d variant=%d\n",map.templateID,map.variantID);std::fflush(stdout);
        S2Net::NetworkMatchPreset preset;preset.mapVariant=map.variantID;preset.useEncounterDeployment=true;
        S2Net::NetworkWorld host(true,preset),client(false);auto initial=host.Initial();
        auto& match=host.Match();
        Require(match.templateID==map.templateID && match.variantID==map.variantID,"incorrect match map metadata");
        std::set<unsigned> places;vector<CPtr<NWorld::CUnit>> all;match.world->GetAllUnits(&all);
        Require(all.size()==12,"encounter retained campaign characters");
        for(int side=0;side<2;++side) {
          const auto& anchors=match.world->networkSpawnPositions[side];Require(!anchors.empty(),"missing authored spawn area");
          const auto& deployed=match.players[side]->GetPlayerUnits();Require(deployed.size()==6,"encounter squad size");
          for(auto unit:deployed) {
            Require(places.insert(unit->GetPosition().pos.p.GetBits()&0x07ffffffu).second,"encounter spawn overlap");
            double minimum=1e30;auto position=unit->GetPosition().GetCP();
            for(auto target:anchors){auto d=position-target;minimum=(std::min)(minimum,double(d.x*d.x+d.y*d.y+d.z*d.z));}
            if(minimum>=36)std::fprintf(stderr,"spawn too far variant=%d side=%d squared=%.3f position=%.3f,%.3f,%.3f anchors=%zu\n",map.variantID,side,minimum,position.x,position.y,position.z,anchors.size());
            Require(minimum<36,"fighter is far from authored spawn positions");
          }
        }
        client.Apply(initial,true);
        Require(client.Match().variantID==map.variantID && client.Match().templateID==map.templateID,"replica lost selected map");
        for(int i=0;i<2;++i)client.Apply(host.Segment(),false);
        Require(client.Match().variantID==map.variantID,"map changed after replica update");
        ++tested;
        std::printf("ENCOUNTER PASSED variant=%d spawns=%zu/%zu\n",map.variantID,
          match.world->networkSpawnPositions[0].size(),match.world->networkSpawnPositions[1].size());std::fflush(stdout);
      }
      Require(tested>0,"selected encounter variant not found");
      std::printf("%zu/%zu random encounter variants: authored deployment and replica passed\n",tested,maps.size());return 0;
    }
    if(argc==4 && std::string(argv[3])=="--client-ui") {
      S2Net::NetworkMatchPreset preset;preset.mapVariant=2400;preset.useEncounterDeployment=true;
      auto host=std::make_shared<S2Net::NetworkWorld>(true,preset),client=std::make_shared<S2Net::NetworkWorld>(false);
      auto server=std::make_shared<S2Net::NetworkSession>(S2Net::Compatibility{"ui-regression","data"},host->Callbacks());
      auto peer=std::make_shared<S2Net::NetworkSession>(S2Net::Compatibility{"ui-regression","data"},client->Callbacks());
      server->Host(0);peer->Connect("127.0.0.1",server->Port());
      int uiStage=0;
      auto until=[&](auto condition) {
        std::printf("UI stage %d\n",++uiStage);std::fflush(stdout);
        auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(!condition() && std::chrono::steady_clock::now()<end) {
          server->Poll();peer->Poll();
          if(host->Match().world)while(CObj<NWorld::CUICmd> ui=host->Match().world->GetUICommand())
            server->Submit(host->EncodeCommand(new NWorld::CCmdInterfaceEvent(ui->GetID())));
          if(client->Match().world)while(CObj<NWorld::CUICmd> ui=client->Match().world->GetUICommand()){}
          Require(server->State()!=S2Net::SessionState::Failed && peer->State()!=S2Net::SessionState::Failed,"UI request ended connection");
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        Require(condition(),"UI request timed out");
      };
      until([&]{return server->State()==S2Net::SessionState::Playing && peer->State()==S2Net::SessionState::Playing;});
      host->BindSession(server);client->BindSession(peer);
      server->Submit(host->EncodeCommand(new NWorld::CCmdEndOfTurn));
      until([&]{return client->Match().world->GetCurrentPlayer()==client->Match().players[1].GetPtr();});
      auto* unit=client->Match().players[1]->GetPlayerUnits().front().GetPtr();
      auto* actor=host->Match().players[1]->GetPlayerUnits().front().GetPtr();
      client->SetPreviewSelection({unit});
      auto point=unit->GetPosition().pos;point.p.SetXY(point.p.GetX()+1,point.p.GetY());point.p.SetFinal(0);
      CObj<NWorld::CCmd> shoot=new NWorld::CCmdShootObject(nullptr,0),reload=new NWorld::CCmdReload,
        pose=new NWorld::CCmdWishPose(NAI::CROUCH),path=new NWorld::CCmdPath(point);
      int start=-1,full=-1;
      Require(client->Preview(unit,shoot,&start,&full,true)==NWorld::UCR_PENDING,"missing preview was treated as failure");
      client->Preview(unit,reload,&start,&full,true);client->Preview(unit,pose,&start,&full,true);
      client->Preview(unit,path,&start,&full);client->HitChance(1,unit,nullptr,unit->GetPosition().GetCP(),0,false);
      until([&]{return client->Preview(unit,shoot,&start,&full,true)!=NWorld::UCR_PENDING &&
        client->Preview(unit,reload,&start,&full,true)!=NWorld::UCR_PENDING &&
        client->Preview(unit,pose,&start,&full,true)!=NWorld::UCR_PENDING &&
        client->Preview(unit,path,&start,&full)!=NWorld::UCR_PENDING;});
      Require(client->Preview(unit,shoot,&start,&full,true)==NWorld::UCR_NO_TARGET,"targetless shooting capability rejected");
      Require(client->Preview(unit,path,&start,&full)==NWorld::UCR_OK,"panel queries displaced path preview");
      std::vector<CObj<NWorld::CCmd>> capabilities;
      for(auto poseValue:{NAI::RUN,NAI::WALK,NAI::CROUCH,NAI::CRAWL})capabilities.push_back(new NWorld::CCmdWishPose(poseValue));
      for(auto aimValue:{NWorld::CSAP_1AP,NWorld::CSAP_10AP,NWorld::CSAP_MAX,NWorld::CSAP_ALL})capabilities.push_back(new NWorld::CCmdCollectSnipeAP(aimValue));
      for(auto capability:capabilities)client->Preview(unit,capability,&start,&full,true);
      until([&]{bool resolved=true;for(auto capability:capabilities)if(client->Preview(unit,capability,&start,&full,true)==NWorld::UCR_PENDING)resolved=false;return resolved;});
      // Cursor and slot windows issue different requests of the same command
      // type each frame. Both exact keys must resolve despite sharing a lane.
      CObj<NWorld::CCmd> pick=new NWorld::CCmdMoveInventoryItem(NWorld::SItem(unit,NWorld::SItem::SLOT,NDb::SLOT_1),NWorld::SItem(unit,NWorld::SItem::HAND));
      CObj<NWorld::CCmd> transfer=new NWorld::CCmdMoveInventoryItem(NWorld::SItem(unit,NWorld::SItem::SLOT,NDb::SLOT_1),NWorld::SItem(unit,NWorld::SItem::SLOT,NDb::SLOT_2));
      until([&]{auto a=client->Preview(unit,pick,&start,&full);auto b=client->Preview(unit,transfer,&start,&full);return a!=NWorld::UCR_PENDING && b!=NWorld::UCR_PENDING;});
      Require(client->Preview(unit,pick,&start,&full)==NWorld::UCR_OK,"native inventory pickup preview failed");
      Require(unit->GetCurrentPath()!=nullptr,"server path not available to client overlay");
      auto* selected=unit;auto* world=client->Match().world.GetPtr();
      CDGPtr<CFuncBase<STime>> clock(world->GetTime());clock.Refresh();auto oldTime=world->GetTime()->GetValue();
      until([&]{return world->GetTime()->GetValue()>oldTime;});Require(clock.Refresh(),"replica time did not update DG version");
      // Switch selection with a target check in flight, then return to the old
      // unit. Its previous generation must not satisfy the new selection.
      client->Preview(unit,new NWorld::CCmdLook(point),&start,&full);
      auto* second=client->Match().players[1]->GetPlayerUnits()[1].GetPtr();
      client->SetPreviewSelection({second});client->SetPreviewSelection({unit});
      Require(client->Preview(unit,pose,&start,&full,true)==NWorld::UCR_PENDING,"selection reused an old preview generation");
      until([&]{return client->Preview(unit,pose,&start,&full,true)!=NWorld::UCR_PENDING;});
      auto before=actor->GetPosition().GetCP();
      peer->Submit(client->EncodeCommand(new NWorld::CCmdSetCommand(unit,path)));
      peer->Submit(client->EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdContinue)));
      until([&]{return actor->GetPosition().GetCP()!=before;});
      Require(client->Match().world.GetPtr()==world && client->Match().players[1]->GetPlayerUnits().front().GetPtr()==selected,"UI references were replaced");
      until([&]{return !actor->HasCommand() && !host->Match().world->IsExecuting();});
      Require(unit->GetAP()==actor->GetAP(),"movement AP did not replicate");
      Require(client->Preview(unit,pose,&start,&full,true)==NWorld::UCR_PENDING,"AP change reused stale availability");
      until([&]{return client->Preview(unit,pose,&start,&full,true)!=NWorld::UCR_PENDING;});
      peer->Submit(client->EncodeCommand(new NWorld::CCmdEndOfTurn));
      until([&]{return world->GetCurrentPlayer()==client->Match().players[0].GetPtr();});
      until([&]{return client->Preview(unit,pose,&start,&full,true)==NWorld::UCR_UNAVAILABLE;});
      server->Finish("Player left");until([&]{return peer->State()==S2Net::SessionState::Finished;});
      std::puts("Client UI regressions: concurrent capabilities/path/aim, targetless capability, pending, selection generation, AP/turn invalidation, DG clock, stable references passed");return 0;
    }
    if(argc>3) {
      bool authoritative=std::string(argv[3])=="--host";
      if(!authoritative && std::string(argv[3])!="--client")return 2;
      std::atomic<bool> cancel(false);
      auto identity=S2Net::IdentifyGameData({argv[2]},{argv[1]},cancel);
      auto endpoint=std::make_shared<S2Net::NetworkWorld>(authoritative);
      auto owner=std::make_shared<S2Net::NetworkSession>(identity,endpoint->Callbacks());
      if(authoritative)owner->Host(static_cast<std::uint16_t>(std::stoi(argv[4])));
      else owner->Connect(argc==6?argv[5]:"127.0.0.1",static_cast<std::uint16_t>(std::stoi(argv[4])));
      bool bound=false;auto end=std::chrono::steady_clock::now()+std::chrono::seconds(120);
      auto report=std::chrono::steady_clock::now();
      while(std::chrono::steady_clock::now()<end) {
        owner->Poll();
        if(owner->State()==S2Net::SessionState::Playing) {
          if(!bound){endpoint->BindSession(owner);bound=true;std::puts("NETWORK READY");std::fflush(stdout);}
          auto* world=endpoint->Match().world.GetPtr();
          while(CObj<NWorld::CUICmd> ui=world->GetUICommand())if(authoritative)owner->Submit(endpoint->EncodeCommand(new NWorld::CCmdInterfaceEvent(ui->GetID())));
          while(CObj<NWorld::CHitLocator> hit=world->GetHitEvent()){}
          while(CObj<NWorld::CEarthQuakeEvent> quake=world->GetEarthQuakeEvent()){}
          std::ifstream controls("_network_cmd.txt");string text;
          if(controls && std::getline(controls,text)) {
            controls.close();std::remove("_network_cmd.txt");std::istringstream request(text);string verb;int index=0,x=0,y=0;request>>verb>>index>>x>>y;
            if(verb=="quit"){owner->Finish("Player left");continue;}
            if(verb=="turn")owner->Submit(endpoint->EncodeCommand(new NWorld::CCmdEndOfTurn));
            else if(index>=0 && index<int(endpoint->Match().players[owner->LocalSlot()]->GetPlayerUnits().size())) {
              auto* unit=endpoint->Match().players[owner->LocalSlot()]->GetPlayerUnits()[index].GetPtr();CObj<NWorld::CCmd> command;
              if(verb=="move"){auto destination=unit->GetPosition().pos;destination.p.SetXY(destination.p.GetX()+x,destination.p.GetY()+y);command=new NWorld::CCmdPath(destination);}
              if(verb=="shoot")command=new NWorld::CCmdShootTile(unit->GetPosition().GetCP()+CVec3(x,y,1));
              if(verb=="reload")command=new NWorld::CCmdReload;
              if(command){owner->Submit(endpoint->EncodeCommand(new NWorld::CCmdSetCommand(unit,command)));owner->Submit(endpoint->EncodeCommand(new NWorld::CCmdSetCommand(unit,new NWorld::CCmdContinue)));}
            }
            std::printf("NETWORK COMMAND %s\n",text.c_str());std::fflush(stdout);
          }
          if(std::chrono::steady_clock::now()>=report) {
            report=std::chrono::steady_clock::now()+std::chrono::seconds(2);
            auto* actor=endpoint->Match().players[1]->GetPlayerUnits().front().GetPtr();auto p=actor->GetPosition().GetCP();
            std::printf("NETWORK time=%u turn=%d client-ap=%d ammo=%d pos=%.3f,%.3f,%.3f\n",world->GetTime()->GetValue(),world->GetCurrentPlayer()?world->GetCurrentPlayer()->GetScenarioPlayerID():-1,actor->GetAP(),actor->GetUnitRPG()->GetWeaponItem()->GetAmmoQuantity(),p.x,p.y,p.z);std::fflush(stdout);
          }
        }
        if(owner->State()==S2Net::SessionState::Finished || owner->State()==S2Net::SessionState::Failed) {
          std::printf("NETWORK END %s\n",owner->Status().c_str());return owner->State()==S2Net::SessionState::Finished && bound?0:1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      owner->Finish("Acceptance complete");return bound?0:1;
    }
    auto match = S2Net::CreateNetworkMatch(S2Net::NetworkMatchPreset());
    Require(match.world->GetCurrentPlayer() == match.players[0].GetPtr(),"host must move first");
    Require(!match.world->IsRealTime(),"network arena must be turn based");
    vector<CPtr<NWorld::CUnit>> all;
    match.world->GetAllUnits(&all);
    Require(all.size() == 12,"arena must contain only the two prepared squads");
    std::set<unsigned> positions;
    std::set<NRPG::CUnit*> instances;
    for (int slot=0;slot<2;++slot) {
      NWorld::CPlayer::CUnitSet units; match.players[slot]->GetUnits(&units);
      Require(units.size() == 6,"squad size");
      Require(match.players[slot]->GetScenarioPlayerID() == slot,"distinct scenario side");
      for (auto unit: units) {
        Require(positions.insert(unit->GetPosition().pos.p.GetBits() & 0x07ffffffu).second,"deployment overlap");
        Require(instances.insert(static_cast<NWorld::CUnitServer*>(unit.GetPtr())->GetUnitRPG()->GetRPGUnit()).second,"shared persona instance");
      }
      Require(match.world->GetDiplomacy()->GetDiplomacyState(slot,1-slot)==NDb::DS_ENEMY,"hostility");
    }
    Require(match.Winner()==-2,"match ended prematurely");
    auto time=match.world->GetTime()->GetValue();
    match.world->UpdateWorld(time+5000,match.players[0]);
    Require(match.world->GetTime()->GetValue()==time,"render advanced authoritative time");
    for(int i=0;i<20;++i) match.world->AdvanceNetworkSegment();
    Require(match.world->GetTime()->GetValue()==time+1000,"fixed clock segments");
    match.world->GivePlayerTurn(match.players[1]);
    Require(match.world->GetCurrentPlayer()==match.players[1].GetPtr(),"client turn");
    S2Net::NetworkWorld host(true),client(false);
    auto initial=host.Initial();client.Apply(initial,true);
    auto* mirror=client.Match().world.GetPtr();
    auto* selected=client.Match().players[0]->GetPlayerUnits().front().GetPtr();
    Require(mirror->bNetworkReplica && mirror!=host.Match().world.GetPtr(),"client world is not an independent replica");
    auto clientTime=mirror->GetTime()->GetValue();
    CDGPtr<CFuncBase<STime>> replicaClock(mirror->GetTime());replicaClock.Refresh();
    for(int i=0;i<5;++i) {
      auto packet=host.Segment();
      auto* source=host.Match().players[0]->GetPlayerUnits().front()->GetNetworkSkeletonAnimator();
      CDGPtr<NAnimation::CSkeletonAnimator> sourcePose(source);sourcePose.Refresh();auto expected=source->GetValue();
      client.Apply(packet,false);
      auto* target=client.Match().players[0]->GetPlayerUnits().front()->GetNetworkSkeletonAnimator();
      CDGPtr<NAnimation::CSkeletonAnimator> targetPose(target);targetPose.Refresh();
      const auto& actual=target->GetValue();
      Require(actual.size()==expected.size() && !actual.empty() && std::memcmp(actual.data(),expected.data(),actual.size()*sizeof(NAnimation::SBonePose))==0,"client skeleton differs from confirmed server pose");
      MarkNewDGFrame();targetPose.Refresh();
      Require(std::memcmp(target->GetValue().data(),expected.data(),expected.size()*sizeof(NAnimation::SBonePose))==0,"client advanced authoritative animation locally");
    }
    Require(replicaClock.Refresh(),"in-place clock read did not notify animation dependencies");
    Require(client.Match().world.GetPtr()==mirror,"world identity changed");
    Require(client.Match().players[0]->GetPlayerUnits().front().GetPtr()==selected,"selected unit identity changed");
    Require(mirror->GetTime()->GetValue()==host.Match().world->GetTime()->GetValue(),"replica clock differs");
    mirror->UpdateWorld(clientTime+100000,client.Match().players[0]);
    Require(mirror->GetTime()->GetValue()==host.Match().world->GetTime()->GetValue(),"client simulated its world");
    auto command=client.EncodeCommand(new NWorld::CCmdEndOfTurn);
    auto result=host.Handle(1,command,false);size_t offset=8;
    Require(S2Net::Get32(result,offset)==NWorld::UCR_UNAVAILABLE,"wrong-side turn accepted");
    result=host.Handle(0,command,false);offset=8;
    Require(S2Net::Get32(result,offset)==NWorld::UCR_OK,"current-side turn rejected");
    auto pump=[&](int ticks) {
      for(int i=0;i<ticks;++i) {
        client.Apply(host.Segment(),false);
        for(const auto& event:host.DrainEvents()){host.PresentEvent(event);client.PresentEvent(event);}
        for(auto* endpoint:{&host,&client}) {
          auto* world=endpoint->Match().world.GetPtr();
          while(CObj<NWorld::CUICmd> ui=world->GetUICommand()) {
            if(endpoint==&host)host.Handle(0,host.EncodeCommand(new NWorld::CCmdInterfaceEvent(ui->GetID())),false);
          }
          while(CObj<NWorld::CHitLocator> hit=world->GetHitEvent()) {}
          while(CObj<NWorld::CEarthQuakeEvent> quake=world->GetEarthQuakeEvent()) {}
        }
      }
    };
    pump(5);
    Require(host.Match().world->GetCurrentPlayer()==host.Match().players[1].GetPtr(),"turn did not reach client");
    auto* actor=host.Match().players[1]->GetPlayerUnits().front().GetPtr();
    auto* replica=client.Match().players[1]->GetPlayerUnits().front().GetPtr();
    bool interrupted=false;
    unsigned side=1;
    auto action=[&](NWorld::CCmd* cmd,bool continueAction=true) {
      std::printf("action %s ap=%d time=%u\n",typeid(*cmd).name(),actor->GetAP(),host.Match().world->GetTime()->GetValue());std::fflush(stdout);
      auto reply=host.Handle(side,client.EncodeCommand(new NWorld::CCmdSetCommand(replica,cmd)),false);
      size_t pos=8;auto code=S2Net::Get32(reply,pos);
      std::printf("command result=%u\n",code);std::fflush(stdout);
      Require(code==NWorld::UCR_OK || code==NWorld::UCR_OK_RELOAD,"remote tactical command rejected");
      if(continueAction)host.Handle(side,client.EncodeCommand(new NWorld::CCmdSetCommand(replica,new NWorld::CCmdContinue)),false);
      pump(1);
      if(!continueAction)return;
      for(int i=0;i<200 && (actor->HasCommand() || host.Match().world->IsExecuting());++i) {
        if(!host.Match().world->IsExecuting() && host.Match().world->GetCurrentPlayer()!=actor->GetPlayer()) {
          interrupted=true;host.Handle(1-side,host.EncodeCommand(new NWorld::CCmdEndOfTurn),false);
        }
        pump(1);
      }
      std::string running;actor->GetCurrentCommandName(&running);
      std::printf("action end ap=%d command=%s executing=%d\n",actor->GetAP(),running.c_str(),host.Match().world->IsExecuting());std::fflush(stdout);
      Require(!actor->HasCommand(),"remote action did not finish");
      Require(actor->GetAP()==replica->GetAP(),"replicated AP differs");
    };
    int ap=actor->GetAP();
    auto before=actor->GetPosition().GetCP();bool moved=false;
    for(auto delta:{CTPoint<int>(1,0),CTPoint<int>(0,1),CTPoint<int>(-1,0),CTPoint<int>(0,-1)}) {
      auto destination=replica->GetPosition().pos;
      destination.p.SetXY(destination.p.GetX()+delta.x,destination.p.GetY()+delta.y);destination.p.SetFinal(0);
      auto preview=host.Handle(1,client.EncodeCommand(new NWorld::CCmdSetCommand(replica,new NWorld::CCmdPath(destination))),true);
      size_t pos=8;if(S2Net::Get32(preview,pos)!=NWorld::UCR_OK)continue;
      Require(preview.size()>24,"server preview contains no path graph");
      action(new NWorld::CCmdPath(destination));moved=fabs(actor->GetPosition().GetCP()-before)>.1f;if(moved)break;
    }
    Require(moved && actor->GetAP()<ap,"remote movement did not spend AP");
    Require(fabs(replica->GetPosition().GetCP()-actor->GetPosition().GetCP())<.001f,"replicated position differs");
    // Native pose changes are a wish followed by a path to the same place.
    action(new NWorld::CCmdWishPose(NAI::CROUCH),false);
    auto crouch=replica->GetSetPosePosition();crouch.SetPose(NAI::CROUCH);
    ap=actor->GetAP();action(new NWorld::CCmdPath(crouch.pos,NAI::PF_USE_POSEDIR));
    Require(actor->GetPosition().GetPose()==NAI::CROUCH && actor->GetAP()<ap,"remote pose did not execute");
    // Refresh AP through the real end-turn queue on both participants.
    host.Handle(1,client.EncodeCommand(new NWorld::CCmdEndOfTurn),false);pump(5);
    host.Handle(0,host.EncodeCommand(new NWorld::CCmdEndOfTurn),false);pump(5);
    auto* weapon=actor->GetUnitRPG()->GetWeaponItem();int ammo=weapon->GetAmmoQuantity();ap=actor->GetAP();
    action(new NWorld::CCmdShootTile(replica->GetPosition().GetCP()+CVec3(3,0,1)));
    Require(weapon->GetAmmoQuantity()<ammo && actor->GetAP()<ap,"remote shot did not spend ammunition/AP");
    Require(replica->GetUnitRPG()->GetWeaponItem()->GetAmmoQuantity()==weapon->GetAmmoQuantity(),"replicated ammunition differs");
    action(new NWorld::CCmdReload);
    Require(weapon->GetAmmoQuantity()==ammo,"remote reload did not restore magazine");
    auto resetAP=[&]{host.Match().world->GivePlayerTurn(host.Match().players[side]);pump(2);};
    auto equip=[&](NRPG::IInventoryItem* item,const CTPoint<int>& position) {
      NWorld::SItem source(replica,NWorld::SItem::BACKPACK,position,item);
      NWorld::SItem target(replica,NWorld::SItem::SLOT,int(NDb::SLOT_2));
      resetAP();action(new NWorld::CCmdExchangeInventoryItems(source,target,true,int(NDb::SLOT_2)));
      Require(replica->GetUnitRPG()->GetInventory()->GetActiveSlot()==NDb::SLOT_2,"inventory equip/activate not replicated");
    };
    // Medical equipment comes from the preset, with injury as a fixture.
    actor=host.Match().players[1]->GetPlayerUnits()[2].GetPtr();
    replica=client.Match().players[1]->GetPlayerUnits()[2].GetPtr();
    bool treated=false;
    for(const auto& item:replica->GetUnitRPG()->GetInventory()->GetItems())if(dynamic_cast<NRPG::IFirstAidItem*>(item.pItem.GetPtr()) && dynamic_cast<NRPG::IFirstAidItem*>(item.pItem.GetPtr())->GetDBFirstAid()->effect==NDb::FAE_NORMAL) {
      auto position=item.sPos;CObj<NRPG::IInventoryItem> aid=item.pItem.GetPtr();equip(aid,position);
      actor->GetUnitRPG()->MakeDirectDamage(10);pump(2);int vp=actor->GetUnitRPG()->GetTotalVP();resetAP();
      std::printf("medical fixture vp=%d healed=%d skill=%d item=%d\n",vp,actor->GetUnitRPG()->GetHealedVP(),int(actor->GetUnitRPG()->GetRPGUnit()->Skills(NDb::ST_MEDICINE)),aid->GetDBItem()->GetRecordID());
      auto* kit=dynamic_cast<NRPG::IFirstAidItem*>(aid.GetPtr())->GetDBFirstAid();
      NRPG::SFirstAid treatment;int required=0;
      bool possible=actor->GetUnitRPG()->GetRPGUnit()->CreateFirstAid(&treatment,40,100,dynamic_cast<NRPG::IFirstAidItem*>(aid.GetPtr()),actor->GetUnitRPG()->GetRPGUnit(),&required);
      std::printf("medical kit effect=%d modifier=%d maxvp=%d basevp=%d possible=%d heal=%f required=%d\n",int(kit->effect),kit->nSkillModifier,actor->GetUnitRPG()->GetRPGUnit()->Skills(NDb::ST_VP).GetMaxValue(),int(actor->GetUnitRPG()->GetRPGUnit()->Skills(NDb::ST_VP)),possible,possible?treatment.fdVP:0.f,required);std::fflush(stdout);
      action(new NWorld::CCmdHeal(replica));pump(30);
      std::printf("medical result vp=%d healed=%d\n",actor->GetUnitRPG()->GetTotalVP(),actor->GetUnitRPG()->GetHealedVP());
      Require(actor->GetUnitRPG()->GetTotalVP()>vp,"remote healing did not restore health");
      Require(replica->GetUnitRPG()->GetTotalVP()==actor->GetUnitRPG()->GetTotalVP(),"replicated healing differs");treated=true;break;
    }
    Require(treated,"preset lacks medicine");
    std::puts("medicine verified");std::fflush(stdout);
    actor=host.Match().players[1]->GetPlayerUnits()[5].GetPtr();
    replica=client.Match().players[1]->GetPlayerUnits()[5].GetPtr();
    bool armed=false;
    for(const auto& item:replica->GetUnitRPG()->GetInventory()->GetItems())if(dynamic_cast<NRPG::IMineItem*>(item.pItem.GetPtr())) {
      auto position=item.sPos;CObj<NRPG::IInventoryItem> mine=item.pItem.GetPtr();equip(mine,position);resetAP();
      auto destination=replica->GetPosition().pos;destination.p.SetXY(destination.p.GetX()+1,destination.p.GetY());
      action(new NWorld::CCmdSetMineOnTile(destination));pump(10);armed=true;break;
    }
    Require(armed,"preset lacks mines");
    list<CPtr<CObjectBase>> traps;
    client.Match().players[1]->GetTrappedObjectsList(&traps);
    list<CPtr<NWorld::IMine>> serverMines,clientMines;
    host.Match().world->GetMinesNear(actor->GetPosition().GetCP(),&serverMines,100);
    client.Match().world->GetMinesNear(replica->GetPosition().GetCP(),&clientMines,100);
    std::printf("mine lists host=%zu client=%zu traps=%zu\n",serverMines.size(),clientMines.size(),traps.size());std::fflush(stdout);
    CPtr<NWorld::CMine> remoteMine;
    for(auto object:traps)if(auto* mine=dynamic_cast<NWorld::CMine*>(object.GetPtr()))remoteMine=mine;
    Require(IsValid(remoteMine),"placed mine missing from replica/side visibility");
    bool cleared=false;
    for(const auto& item:replica->GetUnitRPG()->GetInventory()->GetItems())if(auto* tool=dynamic_cast<NRPG::IToolItem*>(item.pItem.GetPtr())) {
      if(!tool->GetDBItemInfo()->bCanUseForMineCleaning)continue;
      auto position=item.sPos;CObj<NRPG::IInventoryItem> equipment=item.pItem.GetPtr();equip(equipment,position);resetAP();
      actor->GetUnitRPG()->GetRPGUnit()->Skills(NDb::ST_ENGINEERING).SetConst(140);pump(2);
      int quantity=tool->GetIncQuantity();ap=actor->GetAP();
      action(new NWorld::CCmdUntrapObject(remoteMine));pump(20);
      Require(!IsValid(remoteMine),"disarmed mine remains in client graph");
      Require(actor->GetAP()<ap,"disarming did not spend AP");
      Require(!IsValid(equipment) || tool->GetIncQuantity()<quantity,"clearing tool charge not spent");
      cleared=true;break;
    }
    Require(cleared,"preset lacks a usable clearing tool");
    std::puts("mine creation, visibility, disarming and deletion verified");std::fflush(stdout);
    side=0;actor=host.Match().players[0]->GetPlayerUnits()[5].GetPtr();replica=client.Match().players[0]->GetPlayerUnits()[5].GetPtr();resetAP();
    NWorld::CWindowDoor* door=nullptr;NWorld::CWindowDoor* remoteDoor=nullptr;
    auto remoteObject=client.Match().world->GetObjectsForHarness().begin();
    for(auto object:host.Match().world->GetObjectsForHarness()) {
      auto* candidate=dynamic_cast<NWorld::CWindowDoor*>(object.GetPtr());
      if(IsValid(candidate) && !candidate->IsBroken()) {
        vector<NAI::SPathPlace> places;candidate->GetApproaches(&places,host.Match().world->GetPathNetwork());
        for(auto place:places)if(host.Match().world->GetPathNetwork()->IsNativePassable(place) && !host.Match().world->GetPathNetwork()->IsLocked(place)) {
          auto position=actor->GetPosition();position.pos=NAI::SPosition(place,host.Match().world->GetPathNetwork());
          host.Match().world->GetPathNetwork()->Unlock(actor);actor->SetPosition(position);actor->animator.PlaceUnit(position);
          door=candidate;remoteDoor=dynamic_cast<NWorld::CWindowDoor*>(remoteObject->GetPtr());break;
        }
      }
      if(door)break;++remoteObject;
    }
    Require(door && IsValid(remoteDoor),"arena has no usable replicated door");pump(10);resetAP();
    if(door->IsLockedDoor()) {
      auto denied=host.Handle(side,client.EncodeCommand(new NWorld::CCmdSetCommand(replica,new NWorld::CCmdOpenClose(remoteDoor,!door->IsOpen()))),false);
      size_t pos=8;Require(S2Net::Get32(denied,pos)==NWorld::UCR_DOOR_LOCKED,"locked door did not return a command error");
      // The authored arena has locked doors; unlock this test fixture so the
      // opening and trap lifecycle can also be exercised without a campaign key.
      door->LockDoor(false,0,0);pump(2);
    }
    bool wasOpen=door->IsOpen();action(new NWorld::CCmdOpenClose(remoteDoor,!wasOpen));pump(10);
    Require(door->IsOpen()!=wasOpen && remoteDoor->IsOpen()==door->IsOpen(),"door state not replicated");
    bool trapped=false;
    for(const auto& item:replica->GetUnitRPG()->GetInventory()->GetItems())if(dynamic_cast<NRPG::IGrenadeItemInfo*>(item.pItem.GetPtr())) {
      auto position=item.sPos;CObj<NRPG::IInventoryItem> grenade=item.pItem.GetPtr();equip(grenade,position);resetAP();
      action(new NWorld::CCmdGrenadeMode(NRPG::GM_SETTRAP));pump(2);
      action(new NWorld::CCmdSetGrenadeOnObject(remoteDoor));pump(10);
      Require(door->IsMineSet() && remoteDoor->IsMineSet(),"door trap not replicated");trapped=true;break;
    }
    Require(trapped,"failed to arm a door trap");
    cleared=false;
    for(const auto& item:replica->GetUnitRPG()->GetInventory()->GetItems())if(auto* tool=dynamic_cast<NRPG::IToolItem*>(item.pItem.GetPtr())) {
      if(!tool->GetDBItemInfo()->bCanUseForMineCleaning)continue;
      auto position=item.sPos;CObj<NRPG::IInventoryItem> equipment=item.pItem.GetPtr();equip(equipment,position);resetAP();
      actor->GetUnitRPG()->GetRPGUnit()->Skills(NDb::ST_ENGINEERING).SetConst(140);pump(2);
      action(new NWorld::CCmdUntrapObject(remoteDoor));pump(20);
      Require(!door->IsMineSet() && !remoteDoor->IsMineSet(),"door trap remains armed after disarming");cleared=true;break;
    }
    Require(cleared,"failed to clear a door trap");
    std::puts("door opening, trap arming and disarming verified");std::fflush(stdout);
    side=0;
    // Scoped aiming uses the same server queue and retains its native AP pool.
    bool aimed=false;
    for(size_t index=0;index<host.Match().players[0]->GetPlayerUnits().size();++index) {
      actor=host.Match().players[0]->GetPlayerUnits()[index];replica=client.Match().players[0]->GetPlayerUnits()[index];
      if(!actor->CanFight() || !actor->CanSnipe())continue;
      // The native rules allow aiming at a friendly unit; this fixture keeps
      // the AP/state test independent of cover in the authored arena.
      resetAP();action(new NWorld::CCmdShootMode(NDb::SM_Snipe));resetAP();
      auto* remoteEnemy=client.Match().players[0]->GetPlayerUnits().front().GetPtr();
      action(new NWorld::CCmdShootObject(remoteEnemy,0));
      Require(actor->IsSniping() && replica->IsSniping(),"scoped aiming state not replicated");
      ap=actor->GetAP();action(new NWorld::CCmdCollectSnipeAP(NWorld::CSAP_1AP));
      Require(actor->GetAP()==ap-1 && actor->GetSnipingState()->GetCollectedSnipeAP()==replica->GetSnipingState()->GetCollectedSnipeAP(),"aim AP differs");
      host.Handle(0,client.EncodeCommand(new NWorld::CCmdCancel(replica)),false);pump(10);aimed=true;break;
    }
    Require(aimed,"preset has no scoped weapon for aiming");
    // Grenade flight and blast are host simulation; the replica receives the resulting voxel grid.
    side=0;
    std::printf("host units=%u mirror=%u\n",unsigned(host.Match().players[0]->GetPlayerUnits().size()),unsigned(client.Match().players[0]->GetPlayerUnits().size()));std::fflush(stdout);
    Require(!host.Match().players[0]->GetPlayerUnits().empty(),"host squad disappeared");
    actor=host.Match().players[side]->GetPlayerUnits().front().GetPtr();
    replica=client.Match().players[side]->GetPlayerUnits().front().GetPtr();
    NWorld::CBuilding* building=nullptr;unsigned long long live=0,hp=0,hash=0;
    size_t buildingIndex=0;
    for(const auto& entry:host.Match().world->GetBuildingsForHarness()) {
      if(IsValid(entry) && IsValid(entry->GetInfo().pGrid) && IsValid(entry->GetInfo().pPos)) {
        entry->GetInfo().pGrid->GetVoxelStatsForHarness(&live,&hp,&hash);if(live){building=entry;break;}
      }
      ++buildingIndex;
    }
    Require(building!=nullptr,"arena has no destructible building");
    std::puts("building selected");std::fflush(stdout);
    CVec3 epicentre;building->GetInfo().pPos->pos.forward.RotateHVector(&epicentre,building->GetInfo().pGrid->GetLocalCenterForHarness());
    std::printf("grenade actor valid=%d mirror valid=%d rpg=%p inventory=%p\n",IsValid(actor),IsValid(replica),replica->GetUnitRPG(),replica->GetUnitRPG()?replica->GetUnitRPG()->GetInventory():nullptr);std::fflush(stdout);
    Require(replica->GetUnitRPG() && replica->GetUnitRPG()->GetInventory(),"replica lost RPG inventory");
    bool threw=false;
    for(const auto& item:replica->GetUnitRPG()->GetInventory()->GetItems())if(dynamic_cast<NRPG::IGrenadeItemInfo*>(item.pItem.GetPtr())) {
      auto position=item.sPos;CObj<NRPG::IInventoryItem> grenade=item.pItem.GetPtr();equip(grenade,position);resetAP();
      std::printf("grenade item=%d target=%.3f,%.3f,%.3f origin=%.3f,%.3f,%.3f\n",grenade->GetDBItem()->GetRecordID(),epicentre.x,epicentre.y,epicentre.z,actor->GetPosition().GetCP().x,actor->GetPosition().GetCP().y,actor->GetPosition().GetCP().z);
      action(new NWorld::CCmdShootTile(epicentre));pump(120);threw=true;break;
    }
    Require(threw,"preset lacks grenades");
    unsigned long long afterLive=0,afterHP=0,afterHash=0;
    building->GetInfo().pGrid->GetVoxelStatsForHarness(&afterLive,&afterHP,&afterHash);
    Require(afterHP<hp || afterLive<live,"grenade changed no building voxels");
    auto mirrorEntry=client.Match().world->GetBuildingsForHarness().begin();std::advance(mirrorEntry,buildingIndex);
    auto* mirrorBuilding=mirrorEntry->GetPtr();
    unsigned long long mirrorLive,mirrorHP,mirrorHash;mirrorBuilding->GetInfo().pGrid->GetVoxelStatsForHarness(&mirrorLive,&mirrorHP,&mirrorHash);
    Require(mirrorHash==afterHash && mirrorLive==afterLive && mirrorHP==afterHP,"replicated destruction differs");
    // Explicitly stage the native interrupt edge, then return through the real
    // end-turn command. Perception/RNG-triggered interrupts share this controller.
    resetAP();list<NWorld::CUnitServer*> interruptUnits;interruptUnits.push_back(host.Match().players[1]->GetPlayerUnits().front());
    host.Match().world->AddInterrupt(interruptUnits);pump(5);
    Require(host.Match().world->IsInterrupt() && host.Match().world->GetCurrentPlayer()==host.Match().players[1].GetPtr(),"native interrupt did not transfer control");
    Require(mirror->GetCurrentPlayer()==client.Match().players[1].GetPtr() && mirror->IsInterrupt(),"interrupt not replicated");
    host.Handle(1,client.EncodeCommand(new NWorld::CCmdEndOfTurn),false);pump(10);
    Require(host.Match().world->GetCurrentPlayer()==host.Match().players[0].GetPtr() && !host.Match().world->IsInterrupt(),"interrupt failed to return control");
    for(auto unit:host.Match().players[1]->GetPlayerUnits()){unit->GetUnitRPG()->Kill();unit->KillUnit(CVec3(0,0,1));}pump(100);
    for(int ticks=0;ticks<400 && host.Match().world->IsExecuting();++ticks)pump(1);
    std::printf("victory host=%d client=%d executing=%d/%d alive=%d/%d\n",host.Match().Winner(),client.Match().Winner(),host.Match().world->IsExecuting(),mirror->IsExecuting(),host.Match().players[0]->HasAlivePeople(),host.Match().players[1]->HasAlivePeople());std::fflush(stdout);
    Require(host.Match().Winner()==0 && client.Match().Winner()==0,"victory not reflected in replica");
    for(auto unit:host.Match().players[0]->GetPlayerUnits()){unit->GetUnitRPG()->Kill();unit->KillUnit(CVec3(0,0,1));}pump(100);
    for(int ticks=0;ticks<400 && host.Match().world->IsExecuting();++ticks)pump(1);
    Require(host.Match().Winner()==-1 && client.Match().Winner()==-1,"simultaneous squad loss is not a draw");
    std::puts("Network arena and replica: deployment, identities, clock, events, turns, movement, pose, burst/reload, inventory, healing, mines, doors/traps, grenade/destruction, aiming, interrupt/return, victory/draw passed");
    return 0;
  } catch(const SFileIOError& e) { std::fprintf(stderr,"%s\n",e.szError.c_str()); }
    catch(const std::exception& e) { std::fprintf(stderr,"%s\n",e.what()); }
  return 1;
}
