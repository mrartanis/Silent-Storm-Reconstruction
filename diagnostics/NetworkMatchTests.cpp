#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/NetworkMatch.h"
#include "../Main/NetworkWorld.h"
#include "../Main/GSceneUtils.h"
#include "../Main/wUnitServer.h"
#include "../Main/wUnitCommands.h"
#include "../Main/RPGUnitMission.h"
#include "../Main/RPGUnit.h"
#include "../Main/RPGItem.h"
#include "../Main/wUICommands.h"
#include "../Main/wExplTracker.h"
#include "../Main/BuildingGrid.h"
#include "../Main/wBuilding.h"
#include "../Main/wMine.h"
#include "../Main/wObject.h"
#include "../Main/wUnitStates.h"
#include "../Main/NetworkIdentity.h"
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
int main(int argc,char** argv) {
  if (argc != 3 && argc != 4 && argc != 5 && argc != 6) return 2;
  try {
    // The grenade/destruction scenario must replay the same real trajectory:
    // a clock-seeded throw can legitimately miss the selected building.
    GlobalGameRandom().SeedForHarness(42);
    std::srand(42);
    CFileStream db; db.OpenRead(argv[1]);
    NDatabase::Serialize(db,CStructureSaver::READ);
    NGScene::AddResourceDir(argv[2]);
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
