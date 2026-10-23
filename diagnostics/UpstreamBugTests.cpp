#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/wMain.h"
#include "../Main/wUnitGroup.h"
#include "../Main/iCommonUI.h"
#include "../Main/GView.h"
#include "../Main/iMission.h"
#include "../Main/iGameStates.h"
#include "../Main/iUnitPanel.h"
#include "../Main/iDesktopWindow.h"
#include "../Main/wUnitCommands.h"
#include "../Main/aiGrid.h"
#include "../Main/aiLocker.h"
#include "../Main/wUnitServer.h"
#include "../Main/RPGItem.h"
#include "../Main/RPGItemSet.h"
#include "../Main/wUnitAttackExec.h"
#include "../Main/GDecalGeometry.h"
#include "../Main/GCombiner.h"
#include "../Main/GSceneUtils.h"
#include "../Main/GGeometryPositionHash.h"
using GeometryTestHash = NGScene::SGeometryPosHash;
#include "../Main/rpgGlobal.h"
#include "../Main/RPGUnit.h"
#include "../Main/RPGUnitMission.h"
#include "../Main/rpgCheatConstants.h"
#include "../Main/aiCommander.h"
#include "../Main/aiRouteLogic.h"
#include "../Main/aiUnit.h"
#include "../Main/aiPlayer.h"
#include "../Main/aiReaction.h"
#include "../Main/Sync.h"
#include "../Main/A5Script.h"
#include "../Main/scriptPtr.h"
#include "../Main/aiMisc.h"
#include "../Main/wOSBase.h"
#include "../Main/aiInterval.h"
#include "../Main/aiDoorCollider.h"
#include "../Main/scFlowChartItems.h"
#include "../Main/scFlowChart.h"
#include "../Main/scScenarioTracker.h"
#include "../Main/RPGGame.h"
#include "../Main/RPGStore.h"
#include "../Main/UnitTracker.h"
#include "../DBFormat/DataFormat.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <new>

extern CPtr<NScript::CScript> pScript;

static void Require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

static void TemporaryGroups()
{
    CObj<NWorld::CWorld> world = new NWorld::CWorld;
    CObj<NWorld::CUnitGroup> temporary = world->CreateUnitGroup(42);
    Require(temporary->GetID() == 42, "temporary group lost its requested ID");
    Require(!world->GetUnitGroup(42), "temporary group leaked into the world registry");
    for (int i = 0; i < 100; ++i) {
        CObj<NWorld::CUnitGroup> query = world->CreateUnitGroup(-1);
        Require(!world->GetUnitGroup(query->GetID()), "query claimed a reinforcement ID");
    }
}

static void ScrollInitialization()
{
    for (unsigned char pattern : {0xff, 0x11}) {
        void* memory = ::operator new(sizeof(NUI::CScrollWindowBase));
        std::memset(memory, pattern, sizeof(NUI::CScrollWindowBase));
        CObj<NUI::CScrollWindowBase> scroll =
            new(memory) NUI::CScrollWindowBase(NUI::SWindowInfo());
        Require(scroll->GetValue().x == 0 && scroll->GetValue().y == 0,
            "new paper window inherits stale scroll offsets");
    }
}

class AttackMission: public NGame::CMissionBase
{
    OBJECT_BASIC_METHODS(AttackMission);
public:
    int probes = 0;
    int selected = 0;
    void Step() override {}
    bool ProcessEvent(const NInput::SEvent&) override { return false; }
    int CountSelected() override { return selected; }
    bool GetTracePosition(CVec3* position) const override { *position = VNULL3; return true; }
    void CanDoCommand(NWorld::CCmd*, bool, NGame::SActionInfo* info) override {
        ++probes;
        *info = NGame::SActionInfo();
    }
};

class AttackState: public NGame::CStateAttack
{
public:
    void Attach(NGame::IMission* mission) { CStateBase::Initialize(mission); }
    void RefreshCursor() { UpdateCursor(); }
};

static void AttackLocationRefresh()
{
    CObj<AttackMission> mission = new AttackMission;
    CObj<AttackState> state = new AttackState;
    state->Attach(mission);
    // Restore a state aimed at the head. With no held targeting key, the next
    // cursor update changes it to HL_ANY, just as releasing the targeting key.
    CMemoryStream stream;
    NAI::EHitLocation location = NAI::HL_HEAD;
    { CStructureSaver saver(stream, CStructureSaver::WRITE); saver.Add(5, &location); }
    stream.Seek(0);
    { CStructureSaver saver(stream, CStructureSaver::READ);
      static_cast<CObjectBase*>(state.GetPtr())->operator&(saver); }
    state->RefreshCursor();
    Require(mission->probes == 1, "changed aim location retained stale attack availability");
    state->RefreshCursor();
    Require(mission->probes == 1, "unchanged aim location needlessly refreshed availability");
}

static void FlyingRestoration()
{
    CObj<NAI::CPathNetwork> network = new NAI::CPathNetwork;
    NAI::SPathPlace flight(10, 10, 200);
    flight.SetFinal(1); // Layer encodes altitude, not a ground-layer index.
    Require(network->IsPassable(flight), "flying pocket restore treated altitude as a ground layer");
    Require(network->GetPassability(flight) == NAI::AIP_YES, "flying passability treated altitude as a ground layer");
    flight.SetIntegral(0);
    Require(network->IsPassable(flight), "flying altitude was treated as a ladder index");
}

template<class T>
static auto SnipingCommand(int, NAI::EHitLocation location)
    -> decltype(new T(nullptr, nullptr, location))
{
    return new T(nullptr, nullptr, location);
}

template<class T>
static auto SnipingCommand(long, NAI::EHitLocation)
    -> decltype(new T(nullptr, nullptr)) { return new T(nullptr, nullptr); }

static void SnipingAim()
{
    for (auto expected : {NAI::HL_HEAD, NAI::HL_LLEG, NAI::HL_ANY}) {
        CObj<NWorld::CExecSnipeAim> command = SnipingCommand<NWorld::CExecSnipeAim>(0, expected);
        CMemoryStream stream;
        { CStructureSaver saver(stream, CStructureSaver::WRITE);
          static_cast<CObjectBase*>(command.GetPtr())->operator&(saver); }
        stream.Seek(0);
        NAI::EHitLocation actual = NAI::HL_BODY;
        { CStructureSaver saver(stream, CStructureSaver::READ); saver.Add(3, &actual); }
        Require(actual == expected, "sniping preparation replaced the requested aim point with the body");
    }
}

class ExplosionDecal: public NGScene::CExplosionDecalGeometry
{
public:
    ExplosionDecal(): CExplosionDecalGeometry(nullptr, {}, VNULL3, 10.f, 0.f) {}
    void Rebuild(NGScene::CObjectInfo::SData* result, const NGScene::CObjectInfo& source) {
        CExplosionDecalGeometry::Recalc(result, source, source.GetPositions());
    }
};

static void DecalRebuild()
{
    NGScene::CObjectInfo::SData data;
    for (auto position : {CVec3(-1,-1,0), CVec3(1,-1,0), CVec3(0,1,0)}) {
        NGScene::SVertex vertex{};
        vertex.pos = position;
        data.verts.push_back(vertex);
    }
    data.geometry.SetTriangles({STriangle(0,1,2)});
    CObj<NGScene::CObjectInfo> source = new NGScene::CObjectInfo;
    source->AssignFast(data);
    CObj<ExplosionDecal> decal = new ExplosionDecal;
    NGScene::CObjectInfo::SData result;
    decal->Rebuild(&result, *source);
    const auto count = result.verts.size();
    Require(count == 3, "invalid explosion decal test fixture");
    for (int reload = 0; reload < 20; ++reload) {
        decal->Rebuild(&result, *source);
        Require(result.verts.size() == count, "decal rebuild appends another copy of saved vertices");
        Require(result.geometry.indices.size() == 3, "decal triangle count changed on rebuild");
    }
}

class MeshNode: public CPtrFuncBase<NGScene::CObjectInfo>
{
    OBJECT_BASIC_METHODS(MeshNode);
protected:
    bool NeedUpdate() override { return false; }
    void Recalc() override {}
public:
    explicit MeshNode(NGScene::CObjectInfo* mesh = nullptr) { pValue = mesh; }
};

class GeometryPart: public NGScene::IPart
{
    OBJECT_BASIC_METHODS(GeometryPart);
public:
    NGScene::ETransformType transform = NGScene::TT_NONE;
    GeometryPart() {}
    explicit GeometryPart(MeshNode* mesh): IPart(mesh, nullptr, false) {}
    NGScene::ETransformType GetTransformType() const override { return transform; }
};

class ProjectionRecorder: public NGScene::CPerPolyDecal
{
    OBJECT_BASIC_METHODS(ProjectionRecorder);
protected:
    void Recalc(NGScene::CObjectInfo::SData*, const NGScene::CObjectInfo&,
                const std::vector<CVec3>& positions) override { captured = positions; }
public:
    std::vector<CVec3> captured;
    ProjectionRecorder() {}
    ProjectionRecorder(GeometryPart* part, const std::vector<CVec3>& saved): CPerPolyDecal(part, saved) {}
    void Rebuild() { CPerPolyDecal::Recalc(); }
};

static void DecalProjection()
{
    NGScene::CObjectInfo::SData data;
    for (auto position : {CVec3(1,2,0), CVec3(3,4,0), CVec3(5,6,0)}) {
        NGScene::SVertex vertex{}; vertex.pos = position; data.verts.push_back(vertex);
    }
    data.geometry.SetTriangles({STriangle(0,1,2)});
    CObj<NGScene::CObjectInfo> mesh = new NGScene::CObjectInfo;
    mesh->AssignFast(data);
    CObj<MeshNode> node = new MeshNode(mesh);
    CObj<GeometryPart> part = new GeometryPart(node);
    auto saved = mesh->GetPositions();
    std::reverse(saved.begin(), saved.end());
    CObj<ProjectionRecorder> terrain = new ProjectionRecorder(part, saved);
    terrain->Rebuild();
    Require(terrain->captured == mesh->GetPositions(), "terrain decal reused a stale saved vertex order");
    part->transform = NGScene::TT_SIMPLE;
    CObj<ProjectionRecorder> transformed = new ProjectionRecorder(part, saved);
    transformed->Rebuild();
    Require(transformed->captured == saved, "transformed decal lost its original projection pose");
}

static CObj<NWorld::CWorld> FixtureWorld(const char* databasePath, const char* resourcePath)
{
    CFileStream database;
    database.OpenRead(databasePath);
    NDatabase::Serialize(database, CStructureSaver::READ);
    NGScene::AddResourceDir(resourcePath);
    CObj<NRPG::CGlobalGame> game = NRPG::CreateGlobalGame(1);
    CObj<NWorld::CWorld> world = new NWorld::CWorld(game);
    CObj<NWorld::CPostWorldCreateInfo> post;
    world->CreateRandom(810, {}, true, {}, 0, &post, SRandomSeed(123));
    Require(post && world->GetPathNetwork(), "mission fixture construction failed");
    // Deliberately exclude mission scripts: this fixture isolates AI scheduling.
    world->RunPostInit(nullptr);
    return world;
}

static void IdleAI(const char* databasePath, const char* resourcePath)
{
    auto world = FixtureWorld(databasePath, resourcePath);
    std::vector<CPtr<NWorld::CUnit>> units;
    world->GetAllUnits(&units);
    NWorld::CUnitServer* actor = nullptr;
    NAI::CAICommander* commander = nullptr;
    for (auto& entry : units) {
        auto* unit = dynamic_cast<NWorld::CUnitServer*>(entry.GetPtr());
        if (!unit || !unit->CanFight()) continue;
        unit->GetUnitRPG()->GetRPGUnit()->SetCheat(NRPG::CHEAT_NOAI, true);
        if (!actor) {
            commander = dynamic_cast<NAI::CAICommander*>(unit->GetPlayer()->GetCommander());
            if (commander) actor = unit;
        }
    }
    Require(actor && commander, "mission fixture contains no AI actor");
    world->GivePlayerTurn(dynamic_cast<NWorld::CPlayer*>(actor->GetPlayer()));
    Require(!world->IsRealTime() && !world->IsSequence() && !world->IsAction(), "AI fixture is not idle in its turn");
    actor->GetUnitRPG()->GetRPGUnit()->SetCheat(NRPG::CHEAT_NOAI, false);
    auto* ai = commander->GetAIUnit(actor);
    Require(ai != nullptr, "AI actor wrapper missing");
    auto original = actor->GetPosition();
    auto inactive = original;
    inactive.pos.p.SetPose(NAI::CM_INACTIVE);
    for (int routine = 0; routine < 3; ++routine) {
        CObj<NAI::CAIRouteLogic> route = new NAI::CAIRouteLogic(ai);
        route->SetCircled(true);
        if (routine == 1) route->AddCommand(new NAI::CTaskCommandChangePose(NAI::WALK));
        if (routine == 2) route->AddCommand(new NAI::CTaskCommandGoto(original.pos));
        route->AddCommand(new NAI::CTaskCommandWait(100));
        route->AddCommand(new NAI::CTaskCommandChangeDirection(0));
        ai->SetLogic(route);
        actor->SetTemporaryPosition(original);
        Require(!commander->IsEndOfTurn(), "normal AI route was incorrectly skipped");
        actor->SetTemporaryPosition(inactive);
        Require(commander->IsEndOfTurn() == (routine != 2),
            routine == 2 ? "a route with movement was incorrectly skipped" : "inactive stationary AI route blocks the end of turn");
    }
    actor->SetTemporaryPosition(original);
}

class RenderCache;
class RenderFixture: public CObjectBase
{
    OBJECT_BASIC_METHODS(RenderFixture);
public:
    void Visit(RenderCache*) {}
};

class RenderCache: public COrdinarySyncDst<RenderFixture, RenderCache>
{
protected:
    void PostVisit(int, RenderFixture* object) override { cached = object; }
public:
    CPtr<RenderFixture> cached;
    explicit RenderCache(CSyncSrc<RenderFixture>* source): COrdinarySyncDst(source) {}
};

static void RenderRemoval()
{
    CObj<CSetSyncSrc<RenderFixture>> source = new CSetSyncSrc<RenderFixture>;
    CObj<RenderFixture> object = new RenderFixture;
    RenderCache cache(source);
    source->Set(std::vector<RenderFixture*>{object});
    cache.Sync();
    Require(cache.cached == object, "invalid render cache test fixture");
    source->Set(std::vector<RenderFixture*>{});
    cache.Sync();
    Require(!cache.cached, "removed render slot retained a live object reference");
}

class ScriptActor: public NWorld::CUnitServer
{
    OBJECT_NOCOPY_METHODS(ScriptActor);
public:
    bool shell = false;
    int orders = 0;
    CPtr<NWorld::CPlayer> owner;
    bool IsEmptyPK() const override { return shell; }
    bool CanFight() const override { return !shell; }
    NWorld::IPlayer* GetPlayer() const override { return owner; }
    void Do(NWorld::CCommand* command) override { CObj<NWorld::CCommand> hold = command; ++orders; }
};

static void RunScript(NScript::CScript* script, const char* code)
{
    Require(script->DoString(code) == 0, "script compilation failed");
    pScript = script;
    script->ExecuteThreads();
    pScript = nullptr;
}

static void ScriptPK(bool leave)
{
    CObj<NWorld::CWorld> world = new NWorld::CWorld;
    CObj<NScript::CScript> script = NScript::CreateScript(world);
    CObj<ScriptActor> actor = new ScriptActor;
    CObj<ScriptActor> shell = new ScriptActor;
    shell->shell = true;
    NScript::luaPushCPtr(*script, actor); script->SetGlobal("upstreamActor");
    NScript::luaPushCPtr(*script, shell); script->SetGlobal("upstreamShell");
    Script::AutoBlock stack(*script);
    if (leave) {
        RunScript(script, "upstreamResult = UnitLeavePK(upstreamActor)");
        Require(script->GetGlobal("upstreamResult").IsNil(), "UnitLeavePK returned a pilot instead of its missing shell");
    } else {
        RunScript(script, "UnitWearPK(upstreamActor, upstreamShell)");
        Require(actor->orders > 0, "UnitWearPK rejected an empty shell because it cannot fight");
        const int before = actor->orders;
        RunScript(script, "UnitWearPK(upstreamActor, upstreamActor)");
        Require(actor->orders == before, "UnitWearPK accepted a living soldier as a shell");
        CObj<NWorld::CPlayer> owner = new NWorld::CPlayer;
        CObj<NAI::CAICommander> commander = new NAI::CAICommander;
        owner->SetCommander(commander);
        actor->owner = owner; shell->owner = owner;
        Require(NAI::GetAICommander(actor) == commander, "live pilot lost its AI commander");
        Require(!NAI::GetAICommander(shell), "empty PK shell acquired an AI commander");
    }
}

static void PKEquipmentTransfer()
{
    CObj<NRPG::IInventory> shell = NRPG::CreateInventory(nullptr);
    CObj<NRPG::IInventory> pilot = NRPG::CreateInventory(nullptr);
    CObj<NRPG::CWeaponItem> weapon = new NRPG::CWeaponItem;
    Require(shell->Equip(NDb::SLOT_1, weapon), "invalid PK weapon test fixture");
    CObj<NDb::CPanzerklein> fixedSuit = new NDb::CPanzerklein;
    fixedSuit->bSingleSlot = true;
    pilot->SetPanzerklein(fixedSuit, shell);
    Require(pilot->Get(NDb::SLOT_1) == weapon, "putting on a fixed-weapon PK lost its weapon");
    Require(!shell->Get(NDb::SLOT_1), "PK weapon exists in both inventories");
    Require(!pilot->CanEquip(NDb::SLOT_1, weapon), "manual fixed-weapon replacement became allowed");
    pilot->SetPanzerklein(nullptr, shell);
    Require(shell->Get(NDb::SLOT_1) == weapon && !pilot->Get(NDb::SLOT_1),
        "removing the PK failed to return its weapon to the shell");
}

template<class T>
static auto CanRemovePKWeapon(T* inventory, int) -> decltype(inventory->CanTakeOff(NDb::SLOT_1))
{ return inventory->CanTakeOff(NDb::SLOT_1); }
template<class T>
static bool CanRemovePKWeapon(T*, long) { return true; }

static void PKFixedWeaponPolicy()
{
    CObj<NRPG::IInventory> inventory = NRPG::CreateInventory(nullptr);
    Require(CanRemovePKWeapon(inventory.GetPtr(), 0), "normal equipment removal was restricted");
    CObj<NDb::CPanzerklein> suit = new NDb::CPanzerklein;
    suit->bSingleSlot = false;
    inventory->SetPanzerklein(suit, nullptr);
    Require(CanRemovePKWeapon(inventory.GetPtr(), 0), "removable PK equipment was restricted");
    suit->bSingleSlot = true;
    Require(!CanRemovePKWeapon(inventory.GetPtr(), 0), "manual removal of a fixed PK weapon is allowed");
}

static void DoorCollision()
{
    CObj<RenderFixture> first = new RenderFixture, second = new RenderFixture;
    NAI::SSourceInfo closedA(first, nullptr, 0, NWorld::TS_STATE_CLOSED);
    NAI::SSourceInfo openA(first, nullptr, 0, NWorld::TS_STATE_OPEN);
    NAI::SSourceInfo openB(second, nullptr, 0, NWorld::TS_STATE_OPEN);
    NAI::SDoorColliderAnalyzer analyzer;
    Require(!analyzer(NAI::SColliderUserInfo(&closedA, 0)), "one door hull blocked by itself");
    Require(!analyzer(NAI::SColliderUserInfo(&openB, 0)), "inactive hulls from different doors were combined");
    Require(analyzer(NAI::SColliderUserInfo(&openA, 0)), "both hulls of the same door did not block");
    analyzer.Clear();
    Require(!analyzer.IsCollided(), "door collision state survived Clear");
}

static void OverlappingDoors()
{
    CObj<NAI::CPathNetwork> network = new NAI::CPathNetwork;
    network->CreateLayer(8, 8, VNULL2, 0.f, 0);
    network->Freeze(true);
    auto* layer = network->GetLayer(0);
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x)
        layer->tiles[y][x].nPassable = NAI::CP_STAND;
    CObj<RenderFixture> first = new RenderFixture, second = new RenderFixture;
    NAI::SPathPlace key(3, 3, 0), position = key;
    position.SetPose(NAI::CM_STAND);
    NAI::CNodesLayer::STile blocked{}, open{};
    open.nPassable = NAI::CP_STAND;
    auto* door = network->GetFlipper(first);
    door->bOpen = false; door->nFixedFlags = NAI::CPathNetwork::F_LOCKED_DOOR;
    door->locksClosed[key] = blocked; door->locksOpen[key] = open;
    door = network->GetFlipper(second);
    door->bOpen = false; door->nFixedFlags = NAI::CPathNetwork::F_LOCKED_DOOR;
    door->locksClosed[key] = open; door->locksOpen[key] = blocked;
    layer->tiles[3][3].nFlipper = 2; // The legacy single-door index hides door A.
    Require(network->IsLocked(position, false), "overlapping door A lost its blocking restriction");
    Require(!network->IsLocked(position, true), "ignoring doors also blocked an unlocked tile");
    network->GetFlipper(first)->bOpen = true;
    Require(!network->IsLocked(position, false), "inactive overlapping door hulls blocked the passage");
    // Replication restores fields into a live grid. A derived door lookup
    // must not retain indices into the old serialized flipper arrays.
    CMemoryStream update;
    { CStructureSaver saver(update, CStructureSaver::WRITE);
      std::vector<NAI::CPathNetwork::SFlipper> empty;
      std::unordered_map<CPtr<CObjectBase>, int, SPtrHash> noDoors;
      saver.Add(9, &empty); saver.Add(10, &noDoors); }
    update.Seek(0);
    { CStructureSaver saver(update, CStructureSaver::READ);
      static_cast<CObjectBase*>(network.GetPtr())->operator&(saver); }
    Require(network->GetFlippersAt(position).empty(), "restored grid retained a stale door lookup");
}

static void WeaponFamiliarity()
{
    CObj<NRPG::CUnit> unit = new NRPG::CUnit;
    CObj<NRPG::CWeaponItem> weapon = new NRPG::CWeaponItem;
    CObj<NRPG::CWeaponItem> other = new NRPG::CWeaponItem;
    unit->UseWeapon(weapon, 1.f, 100, .1f);
    unit->UseWeapon(weapon, 2.f, 100, .1f);
    Require(fabs(unit->GetWeaponAdaptation(weapon) - .2f) < .0001f,
        "familiarity bonus uses training rate instead of the DB multiplier");
    unit->UseWeapon(weapon, .5f, 100, .1f);
    Require(fabs(unit->GetWeaponAdaptation(weapon) - .25f) < .0001f,
        "changing training rate changed familiarity scaling");
    unit->UseWeapon(weapon, 200.f, 100, .1f);
    Require(fabs(unit->GetWeaponAdaptation(weapon) - 10.f) < .0001f,
        "retail familiarity cap changed");
    Require(unit->GetWeaponAdaptation(other) == 0.f, "familiarity leaked to another weapon");
}

static void ClueSlotSelection()
{
    for (auto type : {NDb::CT_ITEM, NDb::CT_PERSON}) {
        CObj<NDb::CDBScenarioZone> dbZone = new NDb::CDBScenarioZone;
        CObj<NScenario::CScenarioZone> zone = new NScenario::CScenarioZone(dbZone, 1);
        dbZone->templatesIDs = {0, 101, 102};
        std::unordered_map<int, NScenario::CScenarioZone::STemplate> slots;
        slots[102].nEmptyItemSlots = 1;
        slots[102].nEmptyPersonSlots = 1;
        CMemoryStream state;
        { CStructureSaver saver(state, CStructureSaver::WRITE); saver.Add(13, &slots); }
        state.Seek(0);
        { CStructureSaver saver(state, CStructureSaver::READ);
          static_cast<CObjectBase*>(zone.GetPtr())->operator&(saver); }
        CObj<NDb::CDBScenarioClue> record = new NDb::CDBScenarioClue;
        record->clueType = type;
        CObj<NScenario::CScenarioClue> clue = new NScenario::CScenarioClue(record, 1);
        zone->PlaceClue(clue);
        Require(clue->IsPlaced() && clue->GetTemplateID() == 102,
            "zero-capacity map selected for a clue instead of the available slot");
        zone->RemoveClue(clue);
        zone->PlaceClue(clue);
        Require(clue->GetTemplateID() == 102, "returned clue slot was not available for reuse");
    }
}

class FullScenarioGraph: public NScenario::CScenarioFlowChartBase
{
public:
    FullScenarioGraph(): CScenarioFlowChartBase(1, true) { Generate(); }
};

static void ConclusionRouting(const char* databasePath, const char* resourcePath)
{
    CFileStream database;
    database.OpenRead(databasePath);
    NDatabase::Serialize(database, CStructureSaver::READ);
    NGScene::AddResourceDir(resourcePath);
    CObj<FullScenarioGraph> graph = new FullScenarioGraph;
    std::vector<CPtr<NScenario::CScenarioClue>> clues;
    graph->GetClues(&clues);
    int checked = 0;
    for (auto& clue : clues) {
        if (clue->GetDBClue()->clueType != NDb::CT_CONCLUSION) continue;
        ++checked;
        Require(clue->IsPlaced(), "abstract conclusion was dropped from the campaign graph");
        Require(clue->GetTemplateID() == 0 && clue->GetParentZones().empty(),
            "abstract conclusion consumed a physical map slot");
    }
    Require(checked > 0, "campaign fixture has no conclusion clues");
}

template<class Network>
static auto CanPlacePK(Network* network, const NAI::SPathPlace& place, int)
    -> decltype(IsBigLockerPlacePassable(network, place))
{ return IsBigLockerPlacePassable(network, place); }
template<class Network>
static bool CanPlacePK(Network* network, const NAI::SPathPlace& place, long)
{ return NAI::IsBigLockerPassable(network, place); }

static void PKPlacement()
{
    CObj<NAI::CPathNetwork> network = new NAI::CPathNetwork;
    network->CreateLayer(8, 8, VNULL2, 0.f, 0);
    network->Freeze(true);
    auto* layer = network->GetLayer(0);
    NAI::SPathPlace position(3, 3, 0);
    position.SetPose(NAI::CRAWL);
    position.SetDirection(0);
    Require(!CanPlacePK(network.GetPtr(), position, 0), "PK placement accepted an unlocked layer without a floor");
    layer->tiles[3][3].nPassable = NAI::CP_STAND;
    Require(!CanPlacePK(network.GetPtr(), position, 0), "PK placement ignored blocked footprint neighbors");
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x)
        layer->tiles[y][x].nPassable = NAI::CP_STAND;
    for (int direction = 0; direction < 8; ++direction) {
        position.SetDirection(direction);
        Require(CanPlacePK(network.GetPtr(), position, 0), "open standing footprint was rejected");
        NAI::SPathPlace edge(position);
        edge.SetXY(0, 0);
        Require(!CanPlacePK(network.GetPtr(), edge, 0), "PK footprint was clipped at the grid border");
    }
    position.SetLayer(200);
    position.SetFinal(1);
    position.SetPose(NAI::CM_STAND);
    Require(position.IsFinal() && CanPlacePK(network.GetPtr(), position, 0), "flying PK restoration was rejected");
}

static void CriticalIDs(const char* databasePath)
{
    CFileStream database;
    database.OpenRead(databasePath);
    NDatabase::Serialize(database, CStructureSaver::READ);
    CObj<NRPG::CUnit> unit = new NRPG::CUnit;
    CObj<NRPG::IUnitMission> mission = NRPG::CreateUnit(unit);
    for (bool enable : {false, true}) {
        CMemoryStream saved;
        { CStructureSaver writer(saved, CStructureSaver::WRITE);
          std::vector<int> empty; writer.Add(20, &empty); }
        saved.Seek(0);
        { CStructureSaver reader(saved, CStructureSaver::READ);
          static_cast<CObjectBase*>(mission.GetPtr())->operator&(reader); }
        if (enable) mission->EnableCriticals(); else mission->DisableCriticals();
        CMemoryStream result;
        { CStructureSaver writer(result, CStructureSaver::WRITE);
          static_cast<CObjectBase*>(mission.GetPtr())->operator&(writer); }
        result.Seek(0);
        std::vector<int> states;
        { CStructureSaver reader(result, CStructureSaver::READ); reader.Add(20, &states); }
        Require(states.size() == NDb::N_CRIT_TYPES, "critical toggle did not repair a restored short state vector");
        for (int state : states)
            Require(state == (enable ? NRPG::CS_ENABLED : NRPG::CS_DISABLED), "critical toggle left a stale status");
    }
    Require(int(NDb::C_PANZERKLEIN_AXIS) == 16 && int(NDb::C_PANZERKLEIN_BROKEN) == 19,
        "PK critical identifiers do not match the retail database and save format");
}

class PathProbeExecutor: public NWorld::CCommandExecute
{
    OBJECT_BASIC_METHODS(PathProbeExecutor);
public:
    static int probes;
    bool running = false;
    int cost = 0;
    int operator&(CStructureSaver& saver) override {
        saver.Add(2, &running); saver.Add(3, &cost); return 0;
    }
    int GetStartAP() const override { return cost; }
    bool IsExecuting() override { return running; }
    NWorld::CPathConflictsRemover* GetPathConflictsRemover() override { ++probes; return nullptr; }
};
int PathProbeExecutor::probes = 0;
REGISTER_SAVELOAD_CLASS(0x7f550101, PathProbeExecutor);

static void IdlePathBlocker(const char* databasePath, const char* resourcePath)
{
    auto world = FixtureWorld(databasePath, resourcePath);
    std::vector<CPtr<NWorld::CUnit>> units;
    world->GetAllUnits(&units);
    NWorld::CUnitServer* actor = nullptr;
    for (auto& candidate : units) {
        auto* unit = dynamic_cast<NWorld::CUnitServer*>(candidate.GetPtr());
        if (unit && unit->CanFight()) { actor = unit; break; }
    }
    Require(actor != nullptr, "path blocker fixture has no actor");
    world->GivePlayerTurn(dynamic_cast<NWorld::CPlayer*>(actor->GetPlayer()));
    Require(!world->IsRealTime() && actor->CanSpendAP(0), "path blocker fixture is not in its turn");
    for (int state = 0; state < 3; ++state) {
        CObj<PathProbeExecutor> executor = new PathProbeExecutor;
        executor->running = state != 0;
        executor->cost = state == 2 ? 100000 : 0;
        CMemoryStream saved;
        { CStructureSaver saver(saved, CStructureSaver::WRITE); saver.Add(7, &executor); }
        saved.Seek(0);
        { CStructureSaver saver(saved, CStructureSaver::READ);
          static_cast<CObjectBase*>(actor)->operator&(saver); }
        PathProbeExecutor::probes = 0;
        actor->GetPathConflictsRemover();
        Require(PathProbeExecutor::probes == (state == 1 ? 1 : 0),
            state == 0 ? "idle unit promises to free its movement tile" :
            state == 2 ? "unaffordable move promises to free its movement tile" : "running affordable mover was ignored");
    }
}

static void HiddenPortraitAck()
{
    CObj<AttackMission> mission = new AttackMission;
    CObj<NUI::CUnitPanel> panel = new NUI::CUnitPanel(NUI::SWindowInfo(), mission);
    for (int selected : {0, 2}) {
        mission->selected = selected;
        CObj<NUI::CAckEvent> event = new NUI::CAckEvent;
        panel->PlayAckEvent(STime(100), event);
        Require(event->IsReady(), "voice acknowledgement waits forever for a hidden portrait");
        const STime end = event->GetEndTime();
        panel->PlayAckEvent(STime(200), event);
        Require(event->GetEndTime() == end, "releasing a hidden portrait extended acknowledgement lifetime");
        event->Cancel();
        panel->PlayAckEvent(STime(300), event);
        Require(event->IsComplete(STime(300)), "hidden portrait revived a cancelled acknowledgement");
    }
}

static void ClueInitialization()
{
    CObj<NDb::CDBScenarioClue> record = new NDb::CDBScenarioClue;
    for (unsigned char pattern : {0xff, 0x11}) {
        void* memory = ::operator new(sizeof(NScenario::CScenarioClue));
        std::memset(memory, pattern, sizeof(NScenario::CScenarioClue));
        CObj<NScenario::CScenarioClue> clue = new(memory) NScenario::CScenarioClue(record, 1);
        Require(clue->GetTemplateID() == 0, "new clue inherits a stale map template ID");
    }
}

template<class Player>
static auto DropInStore(Player* player, const CTPoint<int>& point, NRPG::IInventoryItem* item, int)
    -> decltype(player->PlaceStoreItem(point, item), void())
{ player->PlaceStoreItem(point, item); }
template<class Player>
static void DropInStore(Player* player, const CTPoint<int>&, NRPG::IInventoryItem* item, long)
{ player->PlaceStoreItem(item); }

static void StoreDropPosition()
{
    CObj<NRPG::CGlobalPlayer> global = new NRPG::CGlobalPlayer;
    CObj<NWorld::CPlayer> player = new NWorld::CPlayer(L"Store fixture", nullptr, global, 0);
    CObj<NDb::CRPGItem> record = new NDb::CRPGItem;
    record->sSize = CTPoint<int>(1, 1);
    record->nWeight = 0;
    CObj<NRPG::IInventoryItem> first = NRPG::CreateDummyItem(record);
    DropInStore(player.GetPtr(), CTPoint<int>(5, 5), first, 0);
    auto* items = player->GetStoreItems();
    Require(items->size() == 1 && items->front().sPos == CTPoint<int>(5, 5),
        "store discarded a valid manual drop position");
    CObj<NRPG::IInventoryItem> second = NRPG::CreateDummyItem(record);
    DropInStore(player.GetPtr(), CTPoint<int>(5, 5), second, 0);
    Require(items->size() == 2 && items->back().sPos != CTPoint<int>(5, 5),
        "occupied store cell did not use a free fallback");
    CObj<NRPG::IInventoryItem> third = NRPG::CreateDummyItem(record);
    DropInStore(player.GetPtr(), CTPoint<int>(-1, -1), third, 0);
    Require(items->size() == 3 && items->back().sPos.x >= 0 && items->back().sPos.y >= 0,
        "automatic store placement lost the returned item");
}

class TransferActor: public NWorld::CUnitServer
{
    OBJECT_NOCOPY_METHODS(TransferActor);
public:
    int operator&(CStructureSaver&) override { return 0; }
};
REGISTER_SAVELOAD_CLASS(0x7f550102, TransferActor);

class TransferMission: public AttackMission
{
    OBJECT_BASIC_METHODS(TransferMission);
public:
    static CPtr<NWorld::CUnit> actor;
    static CObj<NWorld::CCmd> command;
    int operator&(CStructureSaver&) override { return 0; }
    void Command(NWorld::CUnit* unit, NWorld::CCmd* order, bool) override {
        actor = unit; command = order;
    }
};
CPtr<NWorld::CUnit> TransferMission::actor;
CObj<NWorld::CCmd> TransferMission::command;
REGISTER_SAVELOAD_CLASS(0x7f550103, TransferMission);

class TransferTracker: public NGame::CUnitTracker
{
    OBJECT_BASIC_METHODS(TransferTracker);
public:
    CObj<TransferActor> unit;
    int operator&(CStructureSaver& saver) override { saver.Add(2, &unit); return 0; }
    NWorld::CUnit* GetUnit() const override { return unit; }
};
REGISTER_SAVELOAD_CLASS(0x7f550104, TransferTracker);

static void InventoryTransferActor(bool backpack)
{
    CObj<TransferMission> mission = new TransferMission;
    CObj<TransferActor> source = new TransferActor, destination = new TransferActor;
    CObj<NUI::CSlot> slot = dynamic_cast<NUI::CSlot*>(pSSClasses->CreateObject(
        backpack ? int(0xB0521142) : int(0xB0521155)));
    Require(slot != nullptr, "inventory slot class is not registered");
    CObj<TransferTracker> tracker = new TransferTracker;
    tracker->unit = destination;
    CMemoryStream state;
    { CStructureSaver saver(state, CStructureSaver::WRITE);
      saver.Add(2, &mission);
      if (backpack) {
          saver.Add(3, &tracker);
      } else {
          NDb::ESlot hand = NDb::SLOT_1;
          saver.Add(3, &hand); saver.Add(4, &destination);
      } }
    state.Seek(0);
    { CStructureSaver saver(state, CStructureSaver::READ);
      static_cast<CObjectBase*>(slot.GetPtr())->operator&(saver); }
    if (backpack) {
        CMemoryStream base;
        CObj<NUI::CWindow> view = new NUI::CWindow(NUI::SWindowInfo());
        view->SetSize(NUI::SPoint(100, 100));
        { CStructureSaver saver(base, CStructureSaver::WRITE);
          int width = 10, height = 10;
          saver.Add(3, &width); saver.Add(4, &height); saver.Add(8, &view); }
        base.Seek(0);
        CStructureSaver saver(base, CStructureSaver::READ);
        slot->NUI::CSlot::operator&(saver);
    }
    slot->SetPosition(NUI::SPoint(0, 0));
    slot->SetSize(NUI::SPoint(100, 100));
    CObj<NDb::CRPGItem> record = new NDb::CRPGItem;
    record->sSize = CTPoint<int>(1, 1);
    CObj<NRPG::IInventoryItem> item = NRPG::CreateDummyItem(record);
    NWorld::SItem drag(source, NWorld::SItem::HAND, item);
    slot->Place(10, 10, drag);
    Require(TransferMission::actor == source, "inventory transfer executes on the recipient instead of the hand owner");
    auto* order = dynamic_cast<NWorld::CCmdMoveInventoryItem*>(TransferMission::command.GetPtr());
    Require(order && order->GetSource().pUnit == source && order->GetTarget().pUnit == slot->GetUnit(),
        "inventory transfer lost its source or destination");
    drag.pUnit = nullptr;
    slot->Place(10, 10, drag);
    Require(TransferMission::actor == slot->GetUnit(), "ownerless hand did not use the destination actor");
    TransferMission::actor = nullptr; TransferMission::command = nullptr;
}

static void GeometryHashBuckets()
{
    int buckets[4096] = {};
    GeometryTestHash hash;
    for (int i = 0; i < 4096; ++i)
        ++buckets[static_cast<unsigned>(hash(CVec3(i * .25f, 0.f, 0.f))) & 4095];
    int occupied = 0, largest = 0;
    for (int count : buckets) { occupied += count != 0; largest = Max(largest, count); }
    Require(occupied > 2000 && largest < 32,
        "grid-aligned geometry collapses into masked hash buckets");
    Require(hash(CVec3(0.f, 0.f, 0.f)) == hash(CVec3(-0.f, 0.f, 0.f)),
        "equal signed-zero positions have different merge hashes");
    std::vector<CVec3> positions = {VNULL3, CVec3(-0.f, 0.f, 0.f), CVec3(1,2,3), CVec3(1,2,3), CVec3(2,1,3)};
    std::vector<WORD> matches;
    NGScene::MergePositions(&matches, &positions);
    Require(positions.size() == 3 && matches == std::vector<WORD>({0,0,1,1,2}),
        "geometry hash changed vertex merge identity or ordering");
}

static void ImmediateClueNotification(const char* databasePath, const char* resourcePath)
{
    CFileStream database;
    database.OpenRead(databasePath);
    NDatabase::Serialize(database, CStructureSaver::READ);
    NGScene::AddResourceDir(resourcePath);
    CObj<NScenario::CScenarioTracker> tracker = NScenario::CreateScenarioTracker(1);
    CPtr<NScenario::CScenarioClue> selected, other;
    auto* table = NDatabase::GetTable<NDb::CDBScenarioClue>();
    CDBIterator<NDb::CDBScenarioClue> it(*table);
    while (it.MoveNext()) {
        auto* record = it.Get();
        auto* clue = tracker->GetClueByName(record->sSmallDescription);
        if (!clue) continue;
        if (!selected && record->bGiveImmediately && clue->GetObjectiveByType(NDb::OT_CAPTURE) && !tracker->IsClueFound(clue))
            selected = clue;
        else if (!other) other = clue;
    }
    Require(selected && other && selected != other, "campaign fixture has no immediate capturable clue");
    other->SetJustFound(true);
    tracker->CheatTakeClue(selected, true);
    Require(tracker->IsClueFound(selected), "immediate clue capture did not advance the scenario");
    Require(!selected->IsJustFound(), "immediate clue is announced again on the chapter map");
    Require(other->IsJustFound(), "clearing an immediate clue discarded another pending notification");
}

int main(int argc, char** argv)
{
    try {
        const char* mode = argc > 1 ? argv[1] : "temporary-groups";
        if (!std::strcmp(mode, "temporary-groups")) TemporaryGroups();
        else if (!std::strcmp(mode, "scroll-initialization")) ScrollInitialization();
        else if (!std::strcmp(mode, "attack-location")) AttackLocationRefresh();
        else if (!std::strcmp(mode, "flying-restoration")) FlyingRestoration();
        else if (!std::strcmp(mode, "sniping-aim")) SnipingAim();
        else if (!std::strcmp(mode, "decal-rebuild")) DecalRebuild();
        else if (!std::strcmp(mode, "decal-projection")) DecalProjection();
        else if (!std::strcmp(mode, "ai-idle") && argc == 4) IdleAI(argv[2], argv[3]);
        else if (!std::strcmp(mode, "render-removal")) RenderRemoval();
        else if (!std::strcmp(mode, "pk-leave")) ScriptPK(true);
        else if (!std::strcmp(mode, "pk-wear")) ScriptPK(false);
        else if (!std::strcmp(mode, "pk-equipment")) PKEquipmentTransfer();
        else if (!std::strcmp(mode, "pk-fixed-weapon")) PKFixedWeaponPolicy();
        else if (!std::strcmp(mode, "door-collision")) DoorCollision();
        else if (!std::strcmp(mode, "overlapping-doors")) OverlappingDoors();
        else if (!std::strcmp(mode, "weapon-familiarity")) WeaponFamiliarity();
        else if (!std::strcmp(mode, "clue-slots")) ClueSlotSelection();
        else if (!std::strcmp(mode, "conclusions") && argc == 4) ConclusionRouting(argv[2], argv[3]);
        else if (!std::strcmp(mode, "pk-placement")) PKPlacement();
        else if (!std::strcmp(mode, "critical-ids") && argc == 3) CriticalIDs(argv[2]);
        else if (!std::strcmp(mode, "idle-blocker") && argc == 4) IdlePathBlocker(argv[2], argv[3]);
        else if (!std::strcmp(mode, "hidden-portrait")) HiddenPortraitAck();
        else if (!std::strcmp(mode, "clue-initialization")) ClueInitialization();
        else if (!std::strcmp(mode, "store-drop")) StoreDropPosition();
        else if (!std::strcmp(mode, "inventory-actor")) InventoryTransferActor(true);
        else if (!std::strcmp(mode, "slot-actor")) InventoryTransferActor(false);
        else if (!std::strcmp(mode, "geometry-hash")) GeometryHashBuckets();
        else if (!std::strcmp(mode, "clue-notification") && argc == 4) ImmediateClueNotification(argv[2], argv[3]);
        else throw std::runtime_error("unknown upstream regression mode");
        std::printf("PASS: %s\n", mode);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
