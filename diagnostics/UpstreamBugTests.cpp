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
#include "../Main/wUnitCommands.h"
#include "../Main/aiGrid.h"
#include "../Main/aiLocker.h"
#include "../Main/wUnitServer.h"
#include "../Main/RPGItem.h"
#include "../Main/wUnitAttackExec.h"
#include "../Main/GDecalGeometry.h"
#include "../Main/GCombiner.h"
#include "../Main/GSceneUtils.h"
#include "../Main/rpgGlobal.h"
#include "../Main/RPGUnit.h"
#include "../Main/RPGUnitMission.h"
#include "../Main/rpgCheatConstants.h"
#include "../Main/aiCommander.h"
#include "../Main/aiRouteLogic.h"
#include "../Main/aiUnit.h"
#include "../DBFormat/DataFormat.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <new>

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
    void Step() override {}
    bool ProcessEvent(const NInput::SEvent&) override { return false; }
    int CountSelected() override { return 0; }
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
        else throw std::runtime_error("unknown upstream regression mode");
        std::printf("PASS: %s\n", mode);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
