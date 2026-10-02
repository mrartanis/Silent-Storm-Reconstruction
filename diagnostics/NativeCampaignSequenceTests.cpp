#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataMap.h"
#include "../DBFormat/DataRPG.h"
#include "../Main/A5Script.h"
#include "../Main/aiCommander.h"
#include "../Main/aiPosition.h"
#include "../Main/GSceneUtils.h"
#include "../Main/rpgGlobal.h"
#include "../Main/rpgCheatConstants.h"
#include "../Main/RPGUnit.h"
#include "../Main/RPGUnitMission.h"
#include "../Main/RPGItem.h"
#include "../Main/wMain.h"
#include "../Main/wUnitServer.h"
#include "../Main/wUICommands.h"
#include "../Main/wUnitCommands.h"
#include "../Misc/BasicShare.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
void Lua(NWorld::CWorld* world, const char* code) {
  Script::AutoBlock stack(*world->GetOwnScript());
  Require(world->GetOwnScript()->DoString(code) == 0, code);
}
NWorld::CUnitServer* Hero(NWorld::CWorld* world) {
  auto* hero = world->GetGlobalGame()->GetHero();
  Require(hero && hero->GetPers(), "hero missing");
  auto* unit = world->GetUnitServerByPersID(hero->GetPers()->GetRecordID());
  Require(unit != nullptr, "deployed hero missing");
  return unit;
}
void State(NWorld::CWorld* world, const char* phase, bool check) {
  std::vector<CPtr<NWorld::CUnit>> units;
  world->GetAllUnits(&units);
  std::printf("phase=%s sequence=%d realtime=%d units=%zu\n", phase,
    world->IsSequence(), world->IsRealTime(), units.size());
  for (const auto& value : units) {
    auto* unit = dynamic_cast<NWorld::CUnitServer*>(value.GetPtr());
    if (!unit) continue;
    auto* rpg = unit->GetUnitRPG()->GetRPGUnit();
    const bool flag = rpg->IsCheatEnabled(NRPG::CHEAT_SCRIPTSEQUENCE);
    std::printf("unit=%d hero=%d script_sequence=%d free_ap=%d ap=%d\n",
      rpg->GetPers() ? rpg->GetPers()->GetRecordID() : -1,
      rpg->IsHero(), flag, rpg->IsCheatEnabled(NRPG::CHEAT_AP), unit->GetAP());
    if (check) Require(flag == world->IsSequence(), "sequence flag disagrees with world");
  }
}
void Restore(CObj<NWorld::CWorld>& world, const char* path) {
  {
    CFileStream saved;
    saved.OpenWrite(path);
    CStructureSaver saver(saved, CStructureSaver::WRITE);
    saver.Add(2, &world);
    SerializeShared(&saver);
  }
  CObj<NWorld::CWorld> restored;
  {
    CFileStream saved;
    saved.OpenRead(path);
    CSharedHolder shared;
    CStructureSaver saver(saved, CStructureSaver::READ);
    saver.Add(2, &restored);
    SerializeShared(&saver);
  }
  Require(restored.GetPtr() != nullptr, "world did not restore");
  world = restored;
  world->RestoreRuntimeCaches(world->GetGlobalGame());
  State(world, "restored", true);
}
// Acknowledgements deliberately span several world ticks. This exercises the
// authored Lua waits, but the graphical suite must also exercise actual UI completion.
void Pump(CObj<NWorld::CWorld>& world, int& tick, int count, const char* saveInScene = nullptr) {
  std::map<int, int> pending;
  bool saved = false;
  for (int end = tick + count; tick < end; ++tick) {
    world->UpdateWorld(tick * 50, nullptr);
    if (saveInScene && !saved && world->IsSequence() && pending.empty()) {
      State(world, "authored-scene-before-save", true);
      Restore(world, saveInScene);
      saved = true;
    }
    while (auto* raw = world->GetUICommand()) {
      CObj<NWorld::CUICmd> command(raw);
      if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
        pending.emplace(command->GetID(), tick + 4);
    }
    for (auto it = pending.begin(); it != pending.end();) {
      if (it->second > tick) { ++it; continue; }
      world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(it->first));
      it = pending.erase(it);
    }
    Require(NScript::luaLastError.szError.empty(), NScript::luaLastError.szError.c_str());
  }
  Require(pending.empty(), "UI acknowledgement still pending at checkpoint");
  Require(!saveInScene || saved, "no authored scene observed before save timeout");
}
CObj<NWorld::CWorld> Enter(NRPG::CGlobalGame* game, int variant) {
  CObj<NWorld::CWorld> world = new NWorld::CWorld(game);
  world->ExecuteOwnScript();
  CObj<NWorld::CPostWorldCreateInfo> post;
  world->CreateRandom(variant, std::vector<std::string>(), true,
    std::list<CPtr<NScenario::CScenarioClue>>(), 0, &post, SRandomSeed(123));
  Require(post && world->GetPathNetwork(), "mission construction failed");
  CObj<NAI::CSequenceCommander> commander = new NAI::CSequenceCommander(world);
  CDynamicCast<NWorld::CPlayer> player(world->AddPlayer(L"Campaign test", game->players.front(), commander));
  Require(player.GetPtr() != nullptr, "party deployment failed");
  commander->SetPlayer(player);
  world->RunPostInit(post);
  return world;
}
void Pose(NWorld::CWorld* world, int& tick) {
  auto* hero = Hero(world);
  const auto before = hero->GetPosition().GetPose();
  const auto target = before == NAI::CROUCH ? NAI::WALK : NAI::CROUCH;
  auto* player = dynamic_cast<NWorld::CPlayer*>(hero->GetPlayer());
  Require(player != nullptr, "hero player missing");
  world->GivePlayerTurn(player);
  const int ap = hero->GetAP();
  CObj<NWorld::CCmdWishPose> wish = new NWorld::CCmdWishPose(target);
  std::printf("pose setup time=%u tick=%d final=%d active=%d current=%d position=%.3f,%.3f,%.3f can=%d\n",
    world->GetTime()->GetValue(), tick, hero->GetPosition().pos.p.IsFinal(), world->IsUnitActive(hero),
    world->GetCurrentPlayer() == hero->GetPlayer(), hero->GetPosition().GetCP().x,
    hero->GetPosition().GetCP().y, hero->GetPosition().GetCP().z,
    hero->CanDo(wish));
  Require(ap >= 4, "hero has insufficient AP for pose control");
  world->ExecuteCommand(new NWorld::CCmdSetCommand(hero, wish));
  world->UpdateWorld(tick++ * 50, nullptr);
  auto position = hero->GetSetPosePosition();
  position.SetPose(target);
  world->ExecuteCommand(new NWorld::CCmdSetCommand(hero,
    new NWorld::CCmdPath(position.pos, NAI::PF_USE_POSEDIR)));
  world->ExecuteCommand(new NWorld::CCmdSetCommand(hero, new NWorld::CCmdContinue()));
  for (int end = tick + 200; tick < end &&
       (hero->GetPosition().GetPose() != target || hero->HasCommand()); ++tick)
    world->UpdateWorld(tick * 50, nullptr);
  std::string executor;
  hero->GetCurrentCommandName(&executor);
  std::printf("pose before=%d after=%d target=%d ap_before=%d ap_after=%d wish=%d executor=%s time=%u\n",
    before, hero->GetPosition().GetPose(), target, ap, hero->GetAP(), hero->GetWishPose(), executor.c_str(), world->GetTime()->GetValue());
  Require(hero->GetPosition().GetPose() == target, "pose command did not execute");
  Require(hero->GetAP() < ap, "completed pose spent no AP");
  Require(!hero->HasCommand(), "pose did not finish within 200 ticks");
}
void Move(NWorld::CWorld* world, int& tick) {
  auto* hero = Hero(world);
  world->GivePlayerTurn(dynamic_cast<NWorld::CPlayer*>(hero->GetPlayer()));
  const CVec3 before = hero->GetPosition().GetCP();
  const int ap = hero->GetAP();
  bool moved = false;
  // Find a usable adjacent tile through real path commands, not teleportation.
  const int offsets[][2] = {{1,0},{0,1},{-1,0},{0,-1}};
  for (const auto& offset : offsets) {
    auto destination = hero->GetPosition().pos;
    destination.p.SetXY(destination.p.GetX() + offset[0], destination.p.GetY() + offset[1]);
    destination.p.SetFinal(0);
    world->ExecuteCommand(new NWorld::CCmdSetCommand(hero, new NWorld::CCmdPath(destination)));
    world->ExecuteCommand(new NWorld::CCmdSetCommand(hero, new NWorld::CCmdContinue()));
    for (int end = tick + 200; tick < end; ++tick) {
      world->UpdateWorld(tick * 50, nullptr);
      if (!hero->HasCommand() && tick + 1 < end) break;
    }
    Require(!hero->HasCommand(), "movement did not finish within 200 ticks");
    moved = fabs(hero->GetPosition().GetCP() - before) > .1f;
    if (moved) break;
  }
  const auto after = hero->GetPosition().GetCP();
  std::printf("move before=%.3f,%.3f,%.3f after=%.3f,%.3f,%.3f ap_before=%d ap_after=%d\n",
    before.x,before.y,before.z,after.x,after.y,after.z,ap,hero->GetAP());
  Require(moved, "movement commands did not move hero");
  Require(hero->GetAP() < ap, "completed movement spent no AP");
}
void Shoot(NWorld::CWorld* world, int& tick) {
  auto* hero = Hero(world);
  world->GivePlayerTurn(dynamic_cast<NWorld::CPlayer*>(hero->GetPlayer()));
  auto* weapon = hero->GetUnitRPG()->GetWeaponItem();
  Require(weapon && weapon->HasAmmo(), "hero has no loaded weapon");
  const int ammo = weapon->GetAmmoQuantity(), ap = hero->GetAP();
  const CVec3 destination = hero->GetPosition().GetCP() + CVec3(3,0,1);
  world->ExecuteCommand(new NWorld::CCmdSetCommand(hero, new NWorld::CCmdShootTile(destination)));
  world->ExecuteCommand(new NWorld::CCmdSetCommand(hero, new NWorld::CCmdContinue()));
  for (int end = tick + 200; tick < end; ++tick) {
    world->UpdateWorld(tick * 50, nullptr);
    if (!hero->HasCommand() && weapon->GetAmmoQuantity() < ammo) break;
  }
  std::printf("shoot ammo_before=%d ammo_after=%d ap_before=%d ap_after=%d\n",
    ammo,weapon->GetAmmoQuantity(),ap,hero->GetAP());
  Require(weapon->GetAmmoQuantity() < ammo, "shoot command fired no round");
  Require(!hero->HasCommand(), "shooting did not finish within 200 ticks");
  Require(hero->GetAP() < ap, "completed shot spent no AP");
}
}

int main(int argc, char** argv) {
  if (argc != 5) {
    std::fprintf(stderr, "usage: NativeCampaignSequenceTests <game.db> <res> <normal|nested-save|scene-save> <scratch.sav>\n");
    return 2;
  }
  try {
    CFileStream database;
    database.OpenRead(argv[1]);
    NDatabase::Serialize(database, CStructureSaver::READ);
    NGScene::AddResourceDir(argv[2]);
    CObj<NRPG::CGlobalGame> game = NRPG::CreateGlobalGame(1);
    game->players.push_back(NRPG::CreateGlobalPlayer(std::vector<int>{54}));
    CObj<NWorld::CWorld> world = Enter(game, 5246);
    int tick = 0;
    Pump(world, tick, 400);
    State(world, "headquarters", true);
    Require(!world->IsSequence(), "headquarters sequence did not finish");
    // Carry the same global player/merc objects through the actual world-deploy path.
    game = world->GetGlobalGame();
    // EFirst (6102) is the campaign intro BEFORE HQ. ECct (3791) is the
    // first available Allied operation AFTER HQ, the reported Kai trigger.
    const bool sceneSave = !std::strcmp(argv[3], "scene-save");
    const bool intro = sceneSave || !std::strcmp(argv[3], "intro-normal");
    world = Enter(game, intro ? 6102 : 3791);
    tick = 0;
    Require(sceneSave || intro || !std::strcmp(argv[3], "normal") || !std::strcmp(argv[3], "nested-save"), "unknown mode");
    Pump(world, tick, 800, sceneSave ? argv[4] : nullptr);
    State(world, "first-mission-after-scene", true);
    Require(!world->IsSequence(), "first mission scene did not finish");
    Pose(world, tick);
    Move(world, tick);
    Shoot(world, tick);
    if (!std::strcmp(argv[3], "nested-save")) {
      Lua(world, "c_BeginSequence(true); c_BeginSequence(true)");
      Pump(world, tick, 10);
      Require(world->IsSequence(), "nested Begin did not run");
      State(world, "nested-begin", true);
      Restore(world, argv[4]);
      Lua(world, "EndSequence(false,true)");
      Pump(world, tick, 10);
      Require(world->IsSequence(), "inner End ended the outer scene");
      State(world, "nested-inner-end", true);
      Lua(world, "EndSequence(false,true)");
      Pump(world, tick, 10);
      State(world, "nested-outer-end", true);
      Require(!world->IsSequence(), "outer scene did not end");
      Pose(world, tick);
      Move(world, tick);
      Shoot(world, tick);
    }
    Restore(world, argv[4]);
    State(world, "gameplay-after-save", true);
    Pose(world, tick);
    Move(world, tick);
    Shoot(world, tick);
    std::printf("campaign sequence regression passed mode=%s\n", argv[3]);
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
  }
}
