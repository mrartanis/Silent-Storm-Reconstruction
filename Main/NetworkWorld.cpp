#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "NetworkWorld.h"
#include "A5Script.h"
#include "aiCommander.h"
#include "aiUnit.h"
#include "aiJob.h"
#include "wUnitServer.h"
#include "wUnitCommands.h"
#include "wObject.h"
#include "aiGrid.h"
#include "wUICommands.h"
#include "RPGGame.h"
#include "RPGUnitMission.h"
#include "RPGItem.h"
#include "../DBFormat/DataRPG.h"
#include <cstring>
#include <cmath>
#include <stdexcept>

namespace S2Net {
namespace {
struct NetworkPose {
  CPtr<NAnimation::CSkeletonAnimator> node;
  NAnimation::SSkeletonPose pose;
  int operator&(CStructureSaver& f){f.Add(1,&node);f.Add(2,&pose);return 0;}
};
constexpr std::uint32_t HitMagic=0x5449484e;
constexpr std::uint32_t PreviewMagic=0x5652504e;
template<class F> auto NetworkCall(F f) -> decltype(f()) {
  try { return f(); }
  catch(const SFileIOError& error) { throw std::runtime_error(error.szError); }
}
Bytes StreamBytes(CMemoryStream& stream) {
  return Bytes(stream.GetBuffer(),stream.GetBuffer()+stream.GetSize());
}
Bytes Result(NWorld::EUnitCommandResult code,int startAP,int fullAP,std::uint64_t revision) {
  Bytes reply;Put64(reply,revision);Put32(reply,code);Put32(reply,startAP);Put32(reply,fullAP);return reply;
}
bool ValidPosition(const NAI::SPosition& position,NWorld::CWorld* world) {
  auto* net=dynamic_cast<NAI::CPathNetwork*>(world->GetPathNetwork());
  if(!net || position.pNet.GetPtr()!=net || position.p.IsFinal() || position.p.GetLayer()>=net->GetNumLayers())return false;
  auto* layer=net->GetLayer(position.p.GetLayer());
  if(position.p.IsIntegral())return position.p.GetX()<layer->tiles.GetXSize() && position.p.GetY()<layer->tiles.GetYSize();
  return position.p.GetX()<layer->ladders.size() && position.p.GetY()<layer->ladders[position.p.GetX()].pointPassable.size();
}
bool ValidAction(NWorld::CCmd* cmd,NWorld::CWorld* world) {
  using namespace NWorld;
  if(dynamic_cast<CCmdCreateInventoryItem*>(cmd) || dynamic_cast<CCmdCreateAndActivateInventoryItem*>(cmd) ||
     dynamic_cast<CCmdUpdateStore*>(cmd) || dynamic_cast<CCmdUsePassage*>(cmd) ||
     dynamic_cast<CCmdTeleport*>(cmd) || dynamic_cast<CCmdFly*>(cmd) || dynamic_cast<CCmdExplode*>(cmd) ||
     dynamic_cast<CCmdTakeCorpseOnDeploy*>(cmd) || dynamic_cast<CCmdPlayAnimation*>(cmd) ||
     dynamic_cast<CCmdTalk*>(cmd) || dynamic_cast<CCmdNotHeroWantsToTalk*>(cmd) || dynamic_cast<CCmdTakePerk*>(cmd))return false;
  if(auto* path=dynamic_cast<CCmdPath*>(cmd))return !path->bLeaveZone && path->eParams>=NAI::PF_DEFAULT && path->eParams<=NAI::PF_USE_POSEDIR && ValidPosition(path->ptDst,world);
  if(auto* look=dynamic_cast<CCmdLook*>(cmd))return ValidPosition(look->ptDst,world);
  if(auto* mine=dynamic_cast<CCmdSetMineOnTile*>(cmd))return ValidPosition(mine->ptDst,world);
  if(auto* trap=dynamic_cast<CCmdUntrapObject*>(cmd))return IsValid(trap->pTarget) && (dynamic_cast<CMine*>(trap->pTarget.GetPtr()) || dynamic_cast<CWindowDoor*>(trap->pTarget.GetPtr()));
  if(auto* door=dynamic_cast<CCmdOpenClose*>(cmd))return IsValid(door->pObject) && dynamic_cast<IGetApproaches*>(door->pObject.GetPtr());
  if(auto* shot=dynamic_cast<CCmdShootTile*>(cmd))return std::isfinite(shot->ptTarget.x) && std::isfinite(shot->ptTarget.y) && std::isfinite(shot->ptTarget.z);
  if(auto* shot=dynamic_cast<CCmdShootObject*>(cmd))return IsValid(shot->pTarget) && shot->eHL>=NAI::HL_ANY && shot->eHL<NAI::N_HL && shot->nExtraAttackAP>=0;
  if(auto* pose=dynamic_cast<CCmdWishPose*>(cmd))return pose->pose>=NAI::CRAWL && pose->pose<=NAI::RUN;
  if(auto* mode=dynamic_cast<CCmdShootMode*>(cmd))return mode->eMode>=NDb::SM_Snap && mode->eMode<NDb::SM_MAXVALUE;
  if(auto* mode=dynamic_cast<CCmdGrenadeMode*>(cmd))return mode->eMode>=NRPG::GM_THROW && mode->eMode<=NRPG::GM_SETTRAP;
  if(auto* aim=dynamic_cast<CCmdCollectSnipeAP*>(cmd))return aim->eAP>=CSAP_1AP && aim->eAP<=CSAP_ALL;
  return true;
}
}
NetworkWorld::NetworkWorld(bool authoritative,const NetworkMatchPreset& matchPreset):host(authoritative),preset(matchPreset) {
  objects.include=[](CObjectBase* p) {
    return !dynamic_cast<NScript::CScript*>(p) &&
           !dynamic_cast<NAI::CAICommander*>(p) &&
           !dynamic_cast<NAI::IAIUnit*>(p) &&
           !dynamic_cast<NAI::IAIJobManager*>(p);
  };
}
Bytes NetworkWorld::Snapshot(bool initial) {
  // Sample lazy animation/dynamics on the common clock, independent of cameras.
  MarkNewDGFrame();
  vector<CPtr<NWorld::CUnit>> units;match.world->GetAllUnits(&units);
  vector<NetworkPose> poses;
  for(auto unit:units)if(auto* server=dynamic_cast<NWorld::CUnitServer*>(unit.GetPtr())) {
    auto* animator=server->GetNetworkSkeletonAnimator();if(!animator)continue;
    CDGPtr<NAnimation::CSkeletonAnimator> evaluated(animator);evaluated.Refresh();
    NetworkPose frame;frame.node=animator;frame.pose=animator->GetValue();poses.push_back(frame);
  }
  CMemoryStream stream;
  { CStructureSaver save(stream,CStructureSaver::WRITE,&objects); save.Add(1,&match.world);
    save.Add(2,&match.variantID);save.Add(3,&match.templateID);save.Add(4,&poses); }
  return graph.Capture(StreamBytes(stream),initial);
}
Bytes NetworkWorld::Initial() {
  if(!host) throw std::runtime_error("Client cannot create an authoritative match");
  match=CreateNetworkMatch(preset);
  return Snapshot(true);
}
Bytes NetworkWorld::Segment() {
  match.world->AdvanceNetworkSegment();
  return Snapshot(false);
}
void NetworkWorld::Apply(const Bytes& delta,bool initial) {
  if(host) throw std::runtime_error("Host cannot apply a client world");
  graph.Apply(delta,initial);
  objects.changed.clear();
  vector<NetworkPose> poses;
  auto structure=graph.Structure(); CMemoryStream stream;
  stream.Write(structure.data(),static_cast<int>(structure.size()));stream.Seek(0);
  { CStructureSaver load(stream,CStructureSaver::READ,&objects);load.Add(1,&match.world);
    load.Add(2,&match.variantID);load.Add(3,&match.templateID);load.Add(4,&poses); }
  if(!match.world) throw std::runtime_error("Initial state has no world");
  match.world->bNetworkReplica=true;
  match.world->bNetworkArena=true;
  match.game=match.world->GetGlobalGame();
  vector<CPtr<NWorld::CPlayer>> players;match.world->GetPlayersList(&players);
  for(auto player:players) {
    int slot=player->GetScenarioPlayerID();
    if(slot<0 || slot>1) throw std::runtime_error("Unexpected player in network arena");
    match.players[slot]=player;
    // Commands and AI perception run only on the authoritative host. The
    // presentation still expects a harmless commander on each player.
    if(initial) player->SetCommander(new NWorld::CCommander);
  }
  if(!match.players[0] || !match.players[1]) throw std::runtime_error("Incomplete network squads");
  // Deserialization bypasses CCTime::Set and animator setters. Refresh only
  // after the complete graph is linked, before any client view evaluates it.
  MarkNewDGFrame();
  for(auto object:objects.changed) if(IsValid(object))
    if(auto* node=dynamic_cast<CVersioningBase*>(object.GetPtr()))node->NetworkFieldsChanged();
  if(poses.size()>1024)throw std::runtime_error("Too many network skeleton poses");
  for(const auto& frame:poses) {
    if(!IsValid(frame.node) || frame.pose.size()>512)throw std::runtime_error("Invalid network animation node");
    frame.node->ApplyNetworkPose(frame.pose);
  }
  objects.changed.clear();
  appliedRevision=graph.Revision();
  previewUpdated=true;
}
Bytes NetworkWorld::EncodeCommand(NWorld::CCommand* raw) {
  CObj<NWorld::CCommand> command=raw;
  CStructureNetworkContext refs;
  refs.externalReferences=true;refs.ids=objects.ids;refs.refs=objects.refs;refs.types=objects.types;
  // Command-local objects never collide with the permanent world registry.
  refs.nextID=0x80000000u;
  CMemoryStream stream;
  {CStructureSaver save(stream,CStructureSaver::WRITE,&refs);save.Add(1,&command);}
  return StreamBytes(stream);
}
Bytes NetworkWorld::Handle(unsigned slot,const Bytes& bytes,bool preview) {
  using namespace NWorld;
  if(!host || slot>1 || !match.world) return Result(UCR_UNAVAILABLE,0,0,Revision());
  PreviewKind purpose=PreviewKind::Action;
  Bytes commandBytes=bytes;
  if(preview && bytes.size()>=4) {
    size_t pos=0;
    const auto magic=Get32(bytes,pos);
    if(magic==PreviewMagic) {
      auto kind=Get32(bytes,pos);Get64(bytes,pos);Get64(bytes,pos);
      if(kind>static_cast<unsigned>(PreviewKind::Path))return Result(UCR_INVALID_COMMAND,0,0,Revision());
      purpose=static_cast<PreviewKind>(kind);
      commandBytes.assign(bytes.begin()+pos,bytes.end());
    }
    if(magic==HitMagic) {
      auto kind=Get32(bytes,pos),actorID=Get32(bytes,pos),targetID=Get32(bytes,pos),location=Get32(bytes,pos);
      auto first=Get32(bytes,pos);CVec3 point;
      float* coordinates[3]={&point.x,&point.y,&point.z};
      for(auto* value:coordinates){auto bits=Get32(bytes,pos);std::memcpy(value,&bits,4);}
      Get64(bytes,pos);Get64(bytes,pos); // client selection generation and actor state
      auto hitLocation=static_cast<std::int32_t>(location);
      if(pos!=bytes.size() || kind>3 || first>1 || !objects.refs.count(actorID) ||
         !std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z) ||
         (kind==0 && (hitLocation<NAI::HL_ANY || hitLocation>=NAI::N_HL)) ||
         ((kind==2 || kind==3) && location>NAI::THL_UPPER))
        return Result(UCR_INVALID_COMMAND,0,0,Revision());
      auto* actor=dynamic_cast<CUnitServer*>(objects.refs.at(actorID).GetPtr());
      auto* target=targetID && objects.refs.count(targetID) ? dynamic_cast<CUnitServer*>(objects.refs.at(targetID).GetPtr()) : nullptr;
      if(!IsValid(actor) || actor->GetPlayer()!=match.players[slot].GetPtr() || (kind==0 && !IsValid(target)))
        return Result(UCR_UNAVAILABLE,0,0,Revision());
      // Equipment may have changed while the preview was in transit. Native
      // calcers assume a matching active item (grenade calcers dereference it).
      auto* active=actor->GetUnitRPG()->GetInventory()->GetActive();
      auto* heldWeapon=dynamic_cast<NRPG::IWeaponItemInfo*>(active);
      if(!actor->CanFight() || (kind==1 && !dynamic_cast<NRPG::IGrenadeItemInfo*>(active)) ||
         (kind==3 && (!heldWeapon || !heldWeapon->GetDBWeapon()->bBazookaLogic)))
        return Result(UCR_UNAVAILABLE,0,0,Revision());
      auto* game=match.world->GetGame();int chance=0;
      if(kind==0)chance=game->GetCompositeToHit(actor,target,static_cast<NAI::EHitLocation>(location),first!=0);
      if(kind==1)chance=game->GetGrenadeCompositeToHit(actor,point,first!=0,nullptr);
      if(kind==2)chance=game->GetTileCompositeToHit(actor,point,static_cast<NAI::ETileHitLocation>(location),first!=0);
      if(kind==3)chance=game->GetBazookaToHit(actor,point,static_cast<NAI::ETileHitLocation>(location),first!=0);
      return Result(UCR_OK,chance,0,Revision());
    }
  }
  CStructureNetworkContext refs;
  refs.externalReferences=true;refs.ids=objects.ids;refs.refs=objects.refs;refs.types=objects.types;
  CMemoryStream stream;stream.Write(commandBytes.data(),static_cast<int>(commandBytes.size()));stream.Seek(0);
  CObj<CCommand> command;
  {CStructureSaver load(stream,CStructureSaver::READ,&refs);load.Add(1,&command);}
  auto* player=match.players[slot].GetPtr();
  if(!command) return Result(UCR_INVALID_COMMAND,0,0,Revision());
  if(auto* completion=dynamic_cast<CCmdInterfaceEvent*>(command.GetPtr())) {
    if(preview || slot!=0 || !pendingUI.erase(completion->nID))
      return Result(UCR_UNAVAILABLE,0,0,Revision());
    match.world->ExecuteCommand(completion);return Result(UCR_OK,0,0,Revision());
  }
  if(dynamic_cast<CCmdEndOfTurn*>(command.GetPtr())) {
    if(preview || match.world->GetCurrentPlayer()!=player || match.world->IsExecuting())
      return Result(UCR_UNAVAILABLE,0,0,Revision());
    player->GetCommander()->Do(command);return Result(UCR_OK,0,0,Revision());
  }
  auto* unitCommand=dynamic_cast<CCmdUnit*>(command.GetPtr());
  if(!unitCommand || !IsValid(unitCommand->pUnit)) return Result(UCR_INVALID_COMMAND,0,0,Revision());
  auto* unit=dynamic_cast<CUnitServer*>(unitCommand->pUnit.GetPtr());
  if(!unit || unit->GetPlayer()!=player)
    return Result(UCR_UNAVAILABLE,0,0,Revision());
  if(auto* ack=dynamic_cast<CCmdPlayAck*>(command.GetPtr())) {
    if(preview || ack->GetAck()<IA_WEAPON_EMPTY || ack->GetAck()>IA_NO_PLACE_IN_INVENTORY)
      return Result(UCR_INVALID_COMMAND,0,0,Revision());
    player->GetCommander()->DoEvent(command);return Result(UCR_OK,0,0,Revision());
  }
  if(!match.world->IsUnitActive(unit))return Result(UCR_UNAVAILABLE,0,0,Revision());
  int start=0,full=0;EUnitCommandResult result=UCR_OK;
  if(auto* action=dynamic_cast<CCmdSetCommand*>(command.GetPtr())) {
    if(!action->GetCmd()) return Result(UCR_INVALID_COMMAND,0,0,Revision());
    // Script-only inventory creation, stores and leaving the campaign are not
    // tactical requests. Ordinary combat commands keep their native rules.
    bool valid=ValidAction(action->GetCmd(),match.world);
    // The native panel deliberately probes shooting without a target. It is
    // capability information, never permission to execute a targetless shot.
    if(preview && purpose==PreviewKind::Availability)
      if(auto* shot=dynamic_cast<CCmdShootObject*>(action->GetCmd()))
        valid=!shot->pTarget && shot->eHL>=NAI::HL_ANY && shot->eHL<NAI::N_HL && shot->nExtraAttackAP>=0;
    if(!valid)return Result(UCR_INVALID_COMMAND,0,0,Revision());
    if(!dynamic_cast<CCmdContinue*>(action->GetCmd())) result=unit->CanDo(action->GetCmd(),&start,&full);
  }else if(!dynamic_cast<CCmdCancel*>(command.GetPtr()))
    result=UCR_INVALID_COMMAND;
  if(!preview && (result==UCR_OK || result==UCR_OK_RELOAD)) player->GetCommander()->Do(command);
  auto reply=Result(result,start,full,Revision());
  if(preview) {
    CObj<NAI::CPath> path;
    if(auto* action=dynamic_cast<CCmdSetCommand*>(command.GetPtr()))
      if(purpose!=PreviewKind::Availability && dynamic_cast<CCmdPath*>(action->GetCmd()))path=unit->CreateNetworkPreviewPath(action->GetCmd());
    CStructureNetworkContext refs;
    refs.externalReferences=true;refs.ids=objects.ids;refs.refs=objects.refs;refs.types=objects.types;refs.nextID=0x80000000u;
    CMemoryStream data;
    {CStructureSaver save(data,CStructureSaver::WRITE,&refs);save.Add(1,&path);}
    auto body=StreamBytes(data);Put32(reply,static_cast<std::uint32_t>(body.size()));reply.insert(reply.end(),body.begin(),body.end());
  }
  return reply;
}
SessionCallbacks NetworkWorld::Callbacks() {
  SessionCallbacks callbacks;
  callbacks.createInitial=[this]{return NetworkCall([&]{return Initial();});};
  callbacks.segment=[this]{return NetworkCall([&]{return Segment();});};
  callbacks.applyState=[this](const Bytes& bytes,bool initial){NetworkCall([&]{Apply(bytes,initial);});};
  callbacks.command=[this](unsigned slot,const Bytes& bytes){return NetworkCall([&]{return Handle(slot,bytes,false);});};
  callbacks.query=[this](unsigned slot,const Bytes& bytes){return NetworkCall([&]{return Handle(slot,bytes,true);});};
  callbacks.drainEvents=[this]{return NetworkCall([&]{return DrainEvents();});};
  callbacks.event=[this](const Bytes& bytes){NetworkCall([&]{PresentEvent(bytes);});};
  callbacks.result=[this](std::uint64_t id,const Bytes& bytes,bool preview){NetworkCall([&]{ReceiveResult(id,bytes,preview);});};
  callbacks.revision=[this]{return Revision();};
  return callbacks;
}
std::vector<Bytes> NetworkWorld::DrainEvents() {
  vector<CObj<CObjectBase>> events;
  match.world->DrainNetworkEvents(&events);
  std::vector<Bytes> packets;
  for(auto& event:events) {
    if(auto* ui=dynamic_cast<NWorld::CUICmd*>(event.GetPtr())) pendingUI.insert(ui->GetID());
    CStructureNetworkContext refs;
    refs.externalReferences=true;refs.ids=objects.ids;refs.refs=objects.refs;refs.types=objects.types;
    refs.nextID=0x80000000u;
    CMemoryStream stream;
    {CStructureSaver save(stream,CStructureSaver::WRITE,&refs);save.Add(1,&event);}
    packets.push_back(StreamBytes(stream));
  }
  return packets;
}
void NetworkWorld::PresentEvent(const Bytes& bytes) {
  CStructureNetworkContext refs;
  refs.externalReferences=true;refs.ids=objects.ids;refs.refs=objects.refs;refs.types=objects.types;
  CMemoryStream stream;stream.Write(bytes.data(),static_cast<int>(bytes.size()));stream.Seek(0);
  CObj<CObjectBase> event;
  {CStructureSaver load(stream,CStructureSaver::READ,&refs);load.Add(1,&event);}
  if(!event) throw std::runtime_error("empty presentation event");
  match.world->PresentNetworkEvent(event);
}
void NetworkWorld::BindSession(std::shared_ptr<NetworkSession> owner) {
  session=owner;
  if(!host && match.world) {
    match.world->networkPreview=[this](NWorld::CUnitServer* unit,NWorld::CCmd* cmd,int* start,int* full){return Preview(unit,cmd,start,full);};
    match.world->networkPreviewPath=[this](NWorld::CUnitServer* unit)->NAI::CPath*{
      auto active=currentPreviews.find({unit,"path"});if(active==currentPreviews.end())return nullptr;
      auto cache=previews.find(active->second);return cache==previews.end()?nullptr:cache->second.path.GetPtr();
    };
    match.world->networkHitChance=[this](int kind,NWorld::CUnit* actor,NWorld::CUnit* target,CVec3 point,int location,bool first){return HitChance(kind,actor,target,point,location,first);};
  }
}
void NetworkWorld::SetPreviewSelection(const std::vector<NWorld::CUnit*>& units) {
  if(selection==units)return;
  selection=units;++selectionEpoch;currentPreviews.clear();
}
std::uint64_t NetworkWorld::PreviewContext(NWorld::CUnitServer* unit) const {
  // The world revision changes every tick, even while idle. Use the actor's
  // rule-relevant state to retire caches without starving replies in flight.
  Bytes state;auto* world=match.world.GetPtr();auto* rpg=unit->GetUnitRPG();
  Put32(state,unit->GetAP());Put32(state,rpg->GetTotalVP());
  Put32(state,unit->GetPosition().pos.p.GetBits());Put32(state,unit->GetPosition().GetPose());
  Put32(state,world->IsUnitActive(unit));
  Put32(state,world->GetCurrentPlayer()?world->GetCurrentPlayer()->GetScenarioPlayerID():0xffffffffu);
  auto* weapon=rpg->GetWeaponItem();Put32(state,weapon?weapon->GetAmmoQuantity():0);
  Put32(state,weapon?weapon->GetShootMode():0);
  auto* inventory=rpg->GetInventory();
  auto active=objects.ids.find(inventory->GetActive());Put32(state,active==objects.ids.end()?0:active->second);
  for(const auto& item:inventory->GetItems()) {
    auto found=objects.ids.find(item.pItem.GetPtr());Put32(state,found==objects.ids.end()?0:found->second);
    auto body=found==objects.ids.end()?objects.bodies.end():objects.bodies.find(found->second);
    if(body!=objects.bodies.end())state.insert(state.end(),body->second.begin(),body->second.end());
  }
  std::uint64_t hash=14695981039346656037ull;
  for(auto byte:state){hash^=byte;hash*=1099511628211ull;}return hash;
}
NetworkWorld::PreviewResult& NetworkWorld::RequestPreview(const PreviewLane& lane,const Bytes& key) {
  TrimPreviews();
  currentPreviews[lane]=key;
  auto& cached=previews[key];auto now=std::chrono::steady_clock::now();
  auto owner=session.lock();
  if(owner && !cached.pending && (cached.worldRevision==0 || now-cached.requested>std::chrono::milliseconds(250))) {
    cached.pending=true;cached.requested=now;
    auto id=owner->Query(key);
    if(id) requests[id]=key;else cached.pending=false;
  }
  return cached;
}
NWorld::EUnitCommandResult NetworkWorld::Preview(NWorld::CUnitServer* unit,NWorld::CCmd* cmd,int* start,int* full,bool availability) {
  if(!session.lock())return NWorld::UCR_UNAVAILABLE;
  const bool path=dynamic_cast<NWorld::CCmdPath*>(cmd)!=nullptr;
  PreviewKind kind=availability?PreviewKind::Availability:path?PreviewKind::Path:PreviewKind::Action;
  Bytes key;Put32(key,PreviewMagic);Put32(key,static_cast<unsigned>(kind));
  Put64(key,selectionEpoch);Put64(key,PreviewContext(unit));
  std::string lane=availability?std::string("available:")+typeid(*cmd).name():path?"path":std::string("action:")+typeid(*cmd).name();
  if(availability) {
    if(auto* pose=dynamic_cast<NWorld::CCmdWishPose*>(cmd))lane+=":"+std::to_string(pose->pose);
    if(auto* aim=dynamic_cast<NWorld::CCmdCollectSnipeAP*>(cmd))lane+=":"+std::to_string(aim->eAP);
    if(auto* mode=dynamic_cast<NWorld::CCmdShootMode*>(cmd))lane+=":"+std::to_string(mode->eMode);
  }
  auto body=EncodeCommand(new NWorld::CCmdSetCommand(unit,cmd));key.insert(key.end(),body.begin(),body.end());
  auto& cached=RequestPreview({unit,lane},key);
  if(start) *start=cached.startAP;if(full)*full=cached.fullAP;
  return cached.worldRevision ? cached.result : NWorld::UCR_PENDING;
}
void NetworkWorld::ReceiveResult(std::uint64_t id,const Bytes& bytes,bool preview) {
  if(!preview) {
    size_t pos=8;auto result=Get32(bytes,pos);
    if(result>NWorld::UCR_CANT_SEE_TARGET)throw std::runtime_error("Invalid command result");
    if(result!=NWorld::UCR_OK && result!=NWorld::UCR_OK_RELOAD)commandError=static_cast<NWorld::EUnitCommandResult>(result);
    return;
  }
  auto request=requests.find(id);if(request==requests.end()) return;
  auto key=request->second;requests.erase(request);
  auto found=previews.find(key);if(found==previews.end())return;
  auto& cached=found->second;cached.pending=false;
  bool current=false;
  for(const auto& entry:currentPreviews) if(entry.second==key) current=true;
  if(!current) return;
  size_t pos=0;auto revision=Get64(bytes,pos);auto result=Get32(bytes,pos);
  auto start=Get32(bytes,pos),full=Get32(bytes,pos);
  if(result>NWorld::UCR_CANT_SEE_TARGET)
    throw std::runtime_error("Invalid preview response");
  if(revision<cached.worldRevision)return;
  if(pos<bytes.size()) {
    auto length=Get32(bytes,pos);if(length!=bytes.size()-pos)throw std::runtime_error("Invalid preview path");
    CMemoryStream data;data.Write(bytes.data()+pos,static_cast<int>(length));data.Seek(0);
    CStructureNetworkContext refs;refs.externalReferences=true;refs.ids=objects.ids;refs.refs=objects.refs;refs.types=objects.types;
    {CStructureSaver load(data,CStructureSaver::READ,&refs);load.Add(1,&cached.path);}
  }
  cached.worldRevision=revision;cached.result=static_cast<NWorld::EUnitCommandResult>(result);
  cached.startAP=static_cast<int>(static_cast<std::int32_t>(start));
  cached.fullAP=static_cast<int>(static_cast<std::int32_t>(full));
  previewUpdated=true;
}
int NetworkWorld::HitChance(int kind,NWorld::CUnit* actor,NWorld::CUnit* target,CVec3 point,int location,bool first) {
  TrimPreviews();
  auto owner=session.lock();if(!owner || !objects.ids.count(actor))return 0;
  Bytes key;Put32(key,HitMagic);Put32(key,kind);Put32(key,objects.ids.at(actor));
  Put32(key,target && objects.ids.count(target)?objects.ids.at(target):0);Put32(key,location);Put32(key,first?1:0);
  for(float coordinate:{point.x,point.y,point.z}){std::uint32_t bits;std::memcpy(&bits,&coordinate,4);Put32(key,bits);}
  auto* unit=dynamic_cast<NWorld::CUnitServer*>(actor);
  Put64(key,selectionEpoch);Put64(key,PreviewContext(unit));
  auto& cached=RequestPreview({unit,"hit:"+std::to_string(kind)+":"+std::to_string(location)},key);
  return cached.worldRevision?cached.startAP:0;
}
void NetworkWorld::TrimPreviews() {
  if(previews.size()<512)return;
  for(auto it=previews.begin();it!=previews.end() && previews.size()>256;) {
    bool active=false;
    for(const auto& current:currentPreviews)if(current.second==it->first)active=true;
    if(active)++it;else it=previews.erase(it);
  }
}
}
