#pragma once
#include "NetworkMatch.h"
#include "NetworkGraph.h"
#include "NetworkSession.h"
#include <set>
#include "aiPath.h"

namespace S2Net {
enum class PreparationAction : std::uint32_t { ChooseRecruit, Ready, Heal };
enum class PreviewKind : std::uint32_t { Availability, Action, Path };
// A client holds the same CPU graph used by the existing renderer, but never
// advances it. Local camera/UI objects are not members of this graph.
class NetworkWorld {
  CStructureNetworkContext objects;
  NetworkGraph graph;
  NetworkMatch match, otherBase;
  bool preparing=false, ready[2]={false,false}, launchBattle=false;
  unsigned generation=0;
  bool rebuildBase[2]={false,false}, refreshSupply[2]={false,false};
  Bytes HandlePreparation(unsigned,const Bytes&);
  NetworkMatch& ForSlot(unsigned slot) {return host && preparing && slot==1?otherBase:match;}
  bool host;
  bool previewUpdated=false;
  std::uint64_t appliedRevision=0;
  NetworkMatchPreset preset;
  std::set<std::pair<unsigned,int>> pendingUI;
  struct PreviewResult {
    NWorld::EUnitCommandResult result = NWorld::UCR_UNAVAILABLE;
    int startAP = -1, fullAP = -1;
    std::uint64_t worldRevision = 0;
    std::chrono::steady_clock::time_point requested;
    bool pending = false;
    CObj<NAI::CPath> path;
  };
  std::map<Bytes,PreviewResult> previews;
  std::map<std::uint64_t,Bytes> requests;
  // Panel checks have one lane per command type; path/aim have their own lanes.
  using PreviewLane=std::pair<NWorld::CUnitServer*,std::string>;
  std::map<PreviewLane,Bytes> currentPreviews;
  std::vector<NWorld::CUnit*> selection;
  std::uint64_t selectionEpoch=1;
  std::weak_ptr<NetworkSession> session;
  NWorld::EUnitCommandResult commandError=NWorld::UCR_OK;
  Bytes Snapshot(bool initial);
  void RegisterLocalWorld();
  void TrimPreviews();
  std::uint64_t PreviewContext(NWorld::CUnitServer*) const;
  PreviewResult& RequestPreview(const PreviewLane&,const Bytes&);
 public:
  explicit NetworkWorld(bool authoritative,const NetworkMatchPreset& matchPreset=NetworkMatchPreset());
  Bytes Initial();
  Bytes Segment();
  void Apply(const Bytes&,bool initial);
  Bytes EncodeCommand(NWorld::CCommand*);
  Bytes Handle(unsigned slot,const Bytes&,bool preview);
  std::vector<Bytes> DrainEvents();
  void PresentEvent(const Bytes&);
  void BindSession(std::shared_ptr<NetworkSession>);
  NWorld::EUnitCommandResult Preview(NWorld::CUnitServer*,NWorld::CCmd*,int*,int*,bool availability=false);
  void SetPreviewSelection(const std::vector<NWorld::CUnit*>&);
  bool TakePreviewUpdated(){bool changed=previewUpdated;previewUpdated=false;return changed;}
  void ReceiveResult(std::uint64_t,const Bytes&,bool preview);
  int HitChance(int,NWorld::CUnit*,NWorld::CUnit*,CVec3,int,bool);
  NWorld::EUnitCommandResult TakeCommandError(){auto result=commandError;commandError=NWorld::UCR_OK;return result;}
  NetworkMatch& Match() { return match; }
  const NetworkMatch& Match() const {return match;}
  NetworkMatch& SlotMatch(unsigned slot) {return ForSlot(slot);}
  bool IsPreparing() const {return preparing;}
  bool IsReady(unsigned slot) const {return slot<2 && ready[slot];}
  unsigned Generation() const {return generation;}
  const NetworkMatchPreset& Preset() const {return preset;}
  Bytes PreparationCommand(PreparationAction,int index=0,int value=0) const;
  std::uint64_t Revision() const { return host?graph.Revision():appliedRevision; }
  SessionCallbacks Callbacks();
};
}
