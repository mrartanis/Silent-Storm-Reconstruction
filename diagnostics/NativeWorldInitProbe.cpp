#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataMap.h"
#include "../DBFormat/DataRPG.h"
#include "../DBFormat/DataScript.h"
#include "../Main/GSceneUtils.h"
#include "../Main/wUnitAttack.h"
#include "../Main/A5Script.h"
#include "../Main/BuildingGrid.h"
#include "../Main/aiCommander.h"
#include "../Main/aiRoute.h"
#include "../Main/aiUnit.h"
#include "../Main/eventUnit.h"
#include "../Main/iSaveManager.h"
#include "../Main/rpgGlobal.h"
#include "../Main/RPGItem.h"
#include "../Main/RPGUnit.h"
#include "../Main/RPGUnitInfo.h"
#include "../Main/RPGUnitMission.h"
#include "../Main/wMain.h"
#include "../Main/wBuilding.h"
#include "../Main/wUICommands.h"
#include "../Main/wUnitCommands.h"
#include "../Main/wUnitServer.h"
#include "../Misc/RandomGen.h"
#include "../Misc/BasicShare.h"
#include "../MiscDll/Commands.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <set>

static_assert(offsetof(NWorld::CWorld::SWorldDeploySpot, nID) == 4 &&
              offsetof(NWorld::CWorld::SWorldDeploySpot, nPlayer) == 8,
              "world deployment spot field offsets");

namespace {
class AttackEventCounter : public CObjectBase {
  OBJECT_BASIC_METHODS(AttackEventCounter);
 public:
  void OnAttack(const NWorld::CEventOnAttackAtUnit&) { ++count; }
  void OnBullet(const NWorld::CEventOnBullet&) { ++bulletCount; }
  int count = 0;
  int bulletCount = 0;
};
}

int main(int argc, char** argv) {
  const bool mission = argc == 5 && std::strcmp(argv[3], "--mission") == 0;
  const bool missionUIAck = argc == 5 && std::strcmp(argv[3], "--mission-ui-ack") == 0;
  const bool missionRootParty = argc == 5 && std::strcmp(argv[3], "--mission-root-party") == 0;
  const bool missionRootPartySaveTurn = argc == 6 &&
    std::strcmp(argv[3], "--mission-root-party-save-turn") == 0;
  const bool missionRootPartySave = argc == 6 &&
    (std::strcmp(argv[3], "--mission-root-party-save") == 0 ||
     missionRootPartySaveTurn);
  const bool missionBasePartyUIAck = argc == 5 && std::strcmp(argv[3], "--mission-base-party-ui-ack") == 0;
  const bool missionPartyUIAck = argc == 5 && std::strcmp(argv[3], "--mission-party-ui-ack") == 0;
  const bool missionPartyShot = argc == 5 && std::strcmp(argv[3], "--mission-party-shot") == 0;
  const bool missionPartyShotSave = argc == 6 && std::strcmp(argv[3], "--mission-party-shot-save") == 0;
  const bool missionPartyShotSlot = argc == 5 && std::strcmp(argv[3], "--mission-party-shot-slot") == 0;
  const bool missionPartyExplosionSave = argc == 6 && std::strcmp(argv[3], "--mission-party-explosion-save") == 0;
  const bool missionPartyGrenadeSave = argc == 6 && std::strcmp(argv[3], "--mission-party-grenade-save") == 0;
  const bool missionPartyGrenadeFlightSave = argc == 6 && std::strcmp(argv[3], "--mission-party-grenade-flight-save") == 0;
  const bool missionPartyGrenadeInventorySave = argc == 6 && std::strcmp(argv[3], "--mission-party-grenade-inventory-save") == 0;
  const bool missionPartyEngGrenadeInventorySave = argc == 6 && std::strcmp(argv[3], "--mission-party-eng-grenade-inventory-save") == 0;
  const bool missionParty = missionPartyUIAck || missionPartyShot ||
    missionPartyShotSave || missionPartyShotSlot || missionPartyExplosionSave ||
    missionPartyGrenadeSave || missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave ||
    missionPartyEngGrenadeInventorySave;
  const bool missionWithUIAck = missionUIAck || missionParty || missionBasePartyUIAck ||
    missionRootParty || missionRootPartySave;
  if (argc != 3 && !mission && !missionWithUIAck) return 2;
  using Deploy = S2FileIO::StructureFieldCodec<NWorld::CWorld::SWorldDeploySpot>;
  NWorld::CWorld::SWorldDeploySpot spot(
    NAI::SPathPlace::FromBits(0x89abcdefu), -3, 0x12345678), decodedSpot;
  std::uint8_t spotWire[12] = {};
  const std::uint8_t expectedSpot[12] = {
    0xef, 0xcd, 0xab, 0x89, 0xfd, 0xff, 0xff, 0xff,
    0x78, 0x56, 0x34, 0x12};
  if (!Deploy::kPortable || Deploy::kWireSize != 12 ||
      !Deploy::Encode(spot, spotWire, sizeof(spotWire)) ||
      std::memcmp(spotWire, expectedSpot, sizeof(spotWire)) != 0 ||
      !Deploy::Decode(spotWire, sizeof(spotWire), &decodedSpot) ||
      decodedSpot.p.GetBits() != spot.p.GetBits() ||
      decodedSpot.nID != spot.nID || decodedSpot.nPlayer != spot.nPlayer ||
      Deploy::Decode(spotWire, 11, &decodedSpot) ||
      Deploy::Encode(spot, spotWire, 11)) return 67;
  CFileStream database;
  database.OpenRead(argv[1]);
  NDatabase::Serialize(database, CStructureSaver::READ);
  NGScene::AddResourceDir(argv[2]);
  auto* scripts = NDatabase::GetTable<NDb::CDBAutoLoadScript>();
  if (!scripts) return 3;
  int scriptCount = 0;
  CDBIterator<NDb::CDBAutoLoadScript> it(*scripts);
  while (it.MoveNext()) {
    auto* script = it.Get();
    if (!script) return 4;
    CFileStream file;
    std::string path;
    if (!S2FileIO::ResolveGameResourcePath(script->szFileName, &path)) return 8;
    file.OpenRead(path.c_str());
    if (file.GetSize() <= 0) return 8;
    std::printf("autoload %s\n", script->szFileName.c_str());
    ++scriptCount;
  }
  if (!scriptCount) return 5;
  CObj<NRPG::CGlobalGame> game = (mission || missionWithUIAck)
    ? NRPG::CreateGlobalGame() : new NRPG::CGlobalGame;
  if (missionParty || missionBasePartyUIAck || missionRootParty || missionRootPartySave)
    game->players.push_back(NRPG::CreateGlobalPlayer());
  CObj<NWorld::CWorld> world = new NWorld::CWorld(game);
  if (!world || world->GetGlobalGame() != game.GetPtr()) return 6;
  world->ExecuteOwnScript();
  if (!world->GetOwnScript()) return 7;
  {
    Script::AutoBlock stack(*world->GetOwnScript());
    if (world->GetOwnScript()->GetGlobal("DIR_DOWNRIGHT").GetNumber() != 7.0 ||
        world->GetOwnScript()->GetGlobal("maxTriggerIndex").GetNumber() != 0.0 ||
        world->GetOwnScript()->GetGlobal("N_WAIT_TIME_TO_SLEEP").GetNumber() != 2.0)
      return 7;
  }
  std::printf("world initialized with %d autoload scripts\n", scriptCount);
  if (mission || missionWithUIAck) {
    const int variant = std::atoi(argv[4]);
    CObj<NWorld::CPostWorldCreateInfo> post;
    world->CreateRandom(variant, std::vector<std::string>(), true,
      std::list<CPtr<NScenario::CScenarioClue>>(), 0, &post,
      SRandomSeed(123));
    if (!world->GetAIMap() || !world->GetPathNetwork() || !post)
      return 9;
    std::printf("world mission variant %d built\n", variant);
    std::printf("mission scripts queued: %zu\n", post->scripts.size());
    if (variant == 810 || variant == 5376) {
      if (post->scripts.size() != 1) return 10;
    }
    if (variant == 5376) {
      auto* record = NDb::GetTemplVariant(variant);
      if (!record || !record->bNoAttack || world->IsAttackAllowed()) return 58;
      std::printf("base variant 5376: attack prohibited by game.db\n");
    }
    if (variant == 810) {
      world->SetTimeOfDay(NWorld::TOD_NIGHT);
    }
    if (missionParty || missionBasePartyUIAck || missionRootParty || missionRootPartySave) {
      // CPlayerTracker does this between CreateRandom and RunPostInit in the
      // real mission. Its sequence commander also owns human-unit AI wrappers.
      CObj<NAI::CSequenceCommander> commander = new NAI::CSequenceCommander(world);
      CDynamicCast<NWorld::CPlayer> player(world->AddPlayer(
        L"Headless party", game->players.front(), commander));
      if (!player || !game->GetHero()) return 17;
      commander->SetPlayer(player);
      if (!world->GetUnitServerByPersID(game->GetHero()->GetPers()->GetRecordID()))
        return 18;
      std::printf("headless hero deployed\n");
    }
    world->RunPostInit(post);
    int heroHPBefore = -1;
    int shooterAmmoBefore = -1;
    int shooterAPBefore = -1;
    CObj<AttackEventCounter> attackEvents = new AttackEventCounter;
    NGlobal::CEventRegister<AttackEventCounter, NWorld::CEventOnAttackAtUnit>
      attackRegistration(attackEvents.GetPtr(), &AttackEventCounter::OnAttack);
    NGlobal::CEventRegister<AttackEventCounter, NWorld::CEventOnBullet>
      bulletRegistration(attackEvents.GetPtr(), &AttackEventCounter::OnBullet);
    if (missionParty) {
      auto* hero = world->GetUnitServerByPersID(game->GetHero()->GetPers()->GetRecordID());
      auto* shooter = world->GetUnitServer("pers1");
      if (!hero || !shooter) return 20;
      NRPG::SUnitInfo info{};
      hero->GetInfo(&info);
      heroHPBefore = info.nHP;
      auto* weapon = shooter->GetUnitRPG()->GetWeaponItem();
      if (weapon) shooterAmmoBefore = weapon->GetAmmoQuantity();
      if (shooterAmmoBefore < 0) return 22;
      shooterAPBefore = shooter->GetAP();
      std::printf("hero HP before scripted aim: %d\n", heroHPBefore);
      std::printf("shooter ammo before scripted aim: %d\n", shooterAmmoBefore);
      std::printf("shooter AP before scripted aim: %d\n", shooterAPBefore);
    }
    if (variant == 810 && world->GetTimeOfDay() != NWorld::TOD_DAY)
      return 11; // The authored SetTimeOfDay(DAY) command actually ran.
    if (!NScript::luaLastError.szError.empty()) return 12;
    const auto before = world->GetTime()->GetValue();
    int acknowledged = 0;
    std::set<std::string> observedCommands;
    if (missionWithUIAck) {
      int lastShooterAmmo = shooterAmmoBefore;
      // Diagnostic only: the real mission UI consumes these commands and posts
      // CCmdInterfaceEvent on completion. We acknowledge their IDs but do not
      // simulate camera motion, rendering, or input; the world still executes
      // its original scripted unit actions.
      for (int tick = 0; tick < 220; ++tick) {
        world->UpdateWorld(tick * 50, nullptr);
        while (auto* raw = world->GetUICommand()) {
          CObj<NWorld::CUICmd> command(raw);
          const int id = command->GetID();
          if (world->GetOwnScript()->IsUIActionIDPresent(id)) {
            world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(id));
            ++acknowledged;
          }
        }
        if (missionRootPartySaveTurn && tick == 50) {
          auto* boss = world->GetUnitServer("BOSS");
          if (!boss || !boss->CanFight() ||
              !boss->GetPosition().pos.p.IsFinal()) return 78;
          const CVec3 before = boss->GetPosition().GetCP();
          boss->OnTBSEvent(NWorld::TBS_GRID_INFO_UPDATED);
          if (!boss->CanFight() ||
              fabs(boss->GetPosition().GetCP() - before) > 0.001f)
            return 79;
          std::printf("scripted flyer survived grid update at fw2\n");
        }
        if (variant == 810) {
          auto* shooter = world->GetUnitServer("pers1");
          std::string name;
          if (shooter && shooter->GetCurrentCommandName(&name))
            observedCommands.insert(name);
          if (missionParty && shooter) {
            auto* weapon = shooter->GetUnitRPG()->GetWeaponItem();
            const int ammo = weapon ? weapon->GetAmmoQuantity() : -1;
            if (ammo != lastShooterAmmo) {
              std::printf("shooter ammo changed at tick %d: %d -> %d, executor=%s\n",
                tick, lastShooterAmmo, ammo, name.empty() ? "none" : name.c_str());
              lastShooterAmmo = ammo;
            }
          }
        }
        if (!NScript::luaLastError.szError.empty()) return 14;
      }
      std::printf("headless UI IDs acknowledged: %d\n", acknowledged);
      for (const auto& name : observedCommands)
        std::printf("observed shooter executor: %s\n", name.c_str());
      if (missionParty) {
        auto* shooter = world->GetUnitServer("pers1");
        auto* hero = world->GetUnitServerByPersID(game->GetHero()->GetPers()->GetRecordID());
        if (!shooter || !hero) return 21;
        NRPG::SUnitInfo info{};
        hero->GetInfo(&info);
        auto* weapon = shooter->GetUnitRPG()->GetWeaponItem();
        const int shooterAmmoAfter = weapon ? weapon->GetAmmoQuantity() : -1;
        const int shooterAPAfter = shooter->GetAP();
        std::string finalName;
        const bool finalExecutor = shooter && shooter->GetCurrentCommandName(&finalName);
        std::printf("shooter final command=%d executor=%s\n",
          shooter && shooter->HasCommand() ? 1 : 0,
          finalExecutor ? finalName.c_str() : "none");
        std::printf("hero HP after scripted aim: %d\n", info.nHP);
        std::printf("shooter ammo after scripted aim: %d\n", shooterAmmoAfter);
        std::printf("shooter AP after scripted aim: %d\n", shooterAPAfter);
        std::printf("attack events during scripted aim: %d\n", attackEvents->count);
        bool prepared = false;
        for (const auto& name : observedCommands)
          prepared |= name.find("CExecQueue") != std::string::npos;
        if (!prepared) return 19;
        // Steam's aim-only path consumes one round during SelectRay and emits
        // the attack notification on completion, but does not spend AP or damage
        // the target in this selected scripted scenario.
        if (finalExecutor || shooter->HasCommand() ||
            info.nHP != heroHPBefore || shooterAmmoAfter != shooterAmmoBefore - 1 ||
            shooterAPAfter != shooterAPBefore || attackEvents->count != 1)
          return 23;
      }
      if ((variant == 810 && acknowledged == 0) ||
          (variant == 5376 && acknowledged != 3)) return 15;
      if (missionBasePartyUIAck) {
        std::vector<CPtr<NWorld::CPlayer>> players;
        world->GetPlayersList(&players);
        NWorld::CUnitServer* baseUnit = nullptr;
        for (const auto& player : players) {
          std::vector<CPtr<NWorld::CUnitServer>> units;
          player->GetUnits(&units);
          for (const auto& unit : units)
            if (unit && unit->CanFight()) { baseUnit = unit.GetPtr(); break; }
          if (baseUnit) break;
        }
        if (!baseUnit) return 59;
        CObj<NWorld::CCmdShootTile> attack(new NWorld::CCmdShootTile(
          baseUnit->GetPosition().GetCP()));
        if (baseUnit->CanDo(attack.GetPtr()) != NWorld::UCR_GENERAL_FAILURE)
          return 60;
        std::printf("base attack command rejected before trajectory evaluation\n");
      }
      if (missionRootPartySave) {
        const STime savedTime = world->GetTime()->GetValue();
        const auto* hero = game->GetHero();
        if (!hero || !hero->GetPers()) return 61;
        const int heroID = hero->GetPers()->GetRecordID();
        auto* originalHero = world->GetUnitServerByPersID(heroID);
        if (!originalHero) return 61;
        NRPG::SUnitInfo originalInfo{};
        originalHero->GetInfo(&originalInfo);
        const auto originalSpots = world->GetDeploySpotsForHarness();
        {
          CFileStream saved;
          saved.OpenWrite(argv[5]);
          CStructureSaver saver(saved, CStructureSaver::WRITE);
          saver.Add(2, &world);
          SerializeShared(&saver);
        }
        CObj<NWorld::CWorld> restored;
        {
          CFileStream saved;
          saved.OpenRead(argv[5]);
          CSharedHolder shared;
          CStructureSaver saver(saved, CStructureSaver::READ);
          saver.Add(2, &restored);
          SerializeShared(&saver);
        }
        if (!restored || !restored->GetGlobalGame() ||
            !restored->GetGlobalGame()->GetHero() || !restored->GetOwnScript() ||
            !restored->GetTime() || restored->GetTime()->GetValue() != savedTime ||
            restored->GetTimeOfDay() != world->GetTimeOfDay())
          return 62;
        const auto& restoredSpots = restored->GetDeploySpotsForHarness();
        if (restoredSpots.size() != originalSpots.size()) return 68;
        for (std::size_t i = 0; i < originalSpots.size(); ++i) {
          const auto& beforeSpot = originalSpots[i];
          const auto& afterSpot = restoredSpots[i];
          if (afterSpot.p.GetBits() != beforeSpot.p.GetBits() ||
              afterSpot.nID != beforeSpot.nID ||
              afterSpot.nPlayer != beforeSpot.nPlayer) return 68;
        }
        std::printf("root party deployment spots restored: %zu\n", originalSpots.size());
        auto* restoredHero = restored->GetUnitServerByPersID(heroID);
        if (!restoredHero) return 63;
        NRPG::SUnitInfo restoredInfo{};
        restoredHero->GetInfo(&restoredInfo);
        if (restoredInfo.nHP != originalInfo.nHP) return 64;
        world = restored;
        game = world->GetGlobalGame();
        world->RestoreRuntimeCaches(game.GetPtr());
        for (int tick = 220; tick < 230; ++tick) {
          world->UpdateWorld(tick * 50, nullptr);
          while (auto* raw = world->GetUICommand()) {
            CObj<NWorld::CUICmd> command(raw);
            if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
              world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
          }
          if (!NScript::luaLastError.szError.empty()) return 65;
        }
        if (world->GetTime()->GetValue() <= savedTime ||
            !world->GetUnitServerByPersID(heroID)) return 66;
        std::printf("root party save restored and advanced: %u -> %u\n",
          savedTime, world->GetTime()->GetValue());
        std::printf("root party post-load control: realtime=%d sequence=%d current=%d hero_turn=%d turn_id=%d\n",
          world->IsRealTime() ? 1 : 0, world->IsSequence() ? 1 : 0,
          world->GetCurrentPlayer() ? 1 : 0,
          world->GetCurrentPlayer() == restoredHero->GetPlayer() ? 1 : 0,
          world->GetTurnID());
        if (missionRootPartySaveTurn) {
          if (variant != 5247) return 69;
          auto* heroPlayer = restoredHero->GetPlayer();
          int readyTick = 230;
          int waitAcknowledged = 0;
          while (readyTick < 2030 &&
                 (world->IsSequence() || world->GetCurrentPlayer() != heroPlayer)) {
            world->UpdateWorld(readyTick * 50, nullptr);
            while (auto* raw = world->GetUICommand()) {
              CObj<NWorld::CUICmd> command(raw);
              if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID())) {
                world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
                ++waitAcknowledged;
              }
            }
            if (!NScript::luaLastError.szError.empty()) return 77;
            if (readyTick % 200 == 0)
              std::printf("root party waiting for hero turn: tick=%d sequence=%d current=%d turn_id=%d ui_ack=%d\n",
                readyTick, world->IsSequence() ? 1 : 0,
                world->GetCurrentPlayer() ? 1 : 0, world->GetTurnID(),
                waitAcknowledged);
            ++readyTick;
          }
          if (world->IsSequence() || world->GetCurrentPlayer() != heroPlayer) {
            std::printf("root party hero turn unavailable: tick=%d sequence=%d current=%d turn_id=%d ui_ack=%d\n",
              readyTick, world->IsSequence() ? 1 : 0,
              world->GetCurrentPlayer() ? 1 : 0, world->GetTurnID(),
              waitAcknowledged);
            for (const char* waypointName : {"fw2", "B33"}) {
              auto* waypoint = world->GetWaypoint(waypointName);
              if (waypoint)
                std::printf("root party waypoint %s: %.2f %.2f %.2f\n",
                  waypointName, waypoint->ptPos.x, waypoint->ptPos.y,
                  waypoint->ptPos.z);
            }
            std::vector<CPtr<NWorld::CPlayer>> players;
            world->GetPlayersList(&players);
            for (const auto& player : players) {
              std::vector<CPtr<NWorld::CUnitServer>> units;
              player->GetUnits(&units);
              for (const auto& unit : units) {
                std::string name, command;
                world->GetUnitName(unit.GetPtr(), &name);
                unit->GetCurrentCommandName(&command);
                const CVec3 position = unit->GetPosition().GetCP();
                std::printf("root party unit %s: %.2f %.2f %.2f command=%s\n",
                  name.c_str(), position.x, position.y, position.z,
                  command.empty() ? "none" : command.c_str());
              }
            }
            return 69;
          }
          std::printf("root party hero turn ready at tick %d\n", readyTick);
          const int turnBefore = world->GetTurnID();
          heroPlayer->GetCommander()->Do(new NWorld::CCmdEndOfTurn());
          bool sawOtherPlayer = false;
          int transferTick = -1;
          for (int tick = readyTick; tick < readyTick + 400; ++tick) {
            world->UpdateWorld(tick * 50, nullptr);
            while (auto* raw = world->GetUICommand()) {
              CObj<NWorld::CUICmd> command(raw);
              if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
                world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
            }
            auto* current = world->GetCurrentPlayer();
            if (current && current != heroPlayer) sawOtherPlayer = true;
            if (!NScript::luaLastError.szError.empty()) return 70;
            if (sawOtherPlayer && world->GetTurnID() > turnBefore) {
              transferTick = tick;
              break;
            }
          }
          std::printf("root party post-load end turn: %d -> %d, other_player=%d\n",
            turnBefore, world->GetTurnID(), sawOtherPlayer ? 1 : 0);
          if (world->GetTurnID() <= turnBefore || !sawOtherPlayer || transferTick < 0)
            return 71;
          for (int tick = transferTick + 1; tick <= transferTick + 80; ++tick) {
            world->UpdateWorld(tick * 50, nullptr);
            while (auto* raw = world->GetUICommand()) {
              CObj<NWorld::CUICmd> command(raw);
              if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
                world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
            }
            if (!NScript::luaLastError.szError.empty()) return 72;
          }
          const STime secondSavedTime = world->GetTime()->GetValue();
          const int secondSavedTurn = world->GetTurnID();
          if (!world->GetUnitServerByPersID(heroID)) return 73;
          {
            CFileStream saved;
            saved.OpenWrite(argv[5]);
            CStructureSaver saver(saved, CStructureSaver::WRITE);
            saver.Add(2, &world);
            SerializeShared(&saver);
          }
          CObj<NWorld::CWorld> secondRestored;
          {
            CFileStream saved;
            saved.OpenRead(argv[5]);
            CSharedHolder shared;
            CStructureSaver saver(saved, CStructureSaver::READ);
            saver.Add(2, &secondRestored);
            SerializeShared(&saver);
          }
          if (!secondRestored || !secondRestored->GetGlobalGame() ||
              !secondRestored->GetOwnScript() || !secondRestored->GetTime() ||
              secondRestored->GetTime()->GetValue() != secondSavedTime ||
              secondRestored->GetTurnID() != secondSavedTurn ||
              !secondRestored->GetUnitServerByPersID(heroID)) return 74;
          world = secondRestored;
          game = world->GetGlobalGame();
          world->RestoreRuntimeCaches(game.GetPtr());
          for (int tick = transferTick + 81; tick <= transferTick + 90; ++tick) {
            world->UpdateWorld(tick * 50, nullptr);
            while (auto* raw = world->GetUICommand()) {
              CObj<NWorld::CUICmd> command(raw);
              if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
                world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
            }
            if (!NScript::luaLastError.szError.empty()) return 75;
          }
          if (world->GetTime()->GetValue() <= secondSavedTime ||
              !world->GetUnitServerByPersID(heroID)) return 76;
          std::printf("root party second save after end turn restored and advanced: %u -> %u, turn_id=%d\n",
            secondSavedTime, world->GetTime()->GetValue(), world->GetTurnID());
        }
      }
      if (variant == 810) {
        Script::AutoBlock stack(*world->GetOwnScript());
        if (!world->GetOwnScript()->GetGlobal("OnClickUsable").IsFunction())
          return 16; // Defined only after the authored opening sequence.
      }
      if (missionPartyExplosionSave || missionPartyGrenadeSave || missionPartyGrenadeFlightSave ||
          missionPartyGrenadeInventorySave || missionPartyEngGrenadeInventorySave) {
        if (variant != 810) return 45;
        int buildingIndex = -1;
        unsigned long long liveBefore = 0, hpBefore = 0, hashBefore = 0;
        NWorld::CBuilding* target = nullptr;
        int candidateIndex = 0;
        for (const auto& building : world->GetBuildingsForHarness()) {
          if (building && building->GetInfo().pGrid && building->GetInfo().pPos) {
            unsigned long long live = 0, hp = 0, hash = 0;
            building->GetInfo().pGrid->GetVoxelStatsForHarness(&live, &hp, &hash);
            if (live != 0) {
              buildingIndex = candidateIndex;
              liveBefore = live;
              hpBefore = hp;
              hashBefore = hash;
              target = building.GetPtr();
              break;
            }
          }
          ++candidateIndex;
        }
        if (!target) return 46;
        const auto& info = target->GetInfo();
        CVec3 epicentre;
        info.pPos->pos.forward.RotateHVector(&epicentre,
          info.pGrid->GetLocalCenterForHarness());
        if (missionPartyGrenadeSave || missionPartyGrenadeFlightSave ||
            missionPartyGrenadeInventorySave || missionPartyEngGrenadeInventorySave) {
          auto* grenade = missionPartyEngGrenadeInventorySave ? nullptr : NDb::GetRPGGrenade(21);
          auto* engTable = missionPartyEngGrenadeInventorySave
            ? NDatabase::GetTable<NDb::CRPGEngGrenade>() : nullptr;
          auto* engGrenade = engTable ? engTable->GetRecord(2) : nullptr;
          auto* thrower = world->GetUnitServer("pers1");
          if ((!grenade && !engGrenade) || !thrower ||
              !world->GetExplosionMasterForHarness()) return 52;
          const CVec3 throwerPosition = thrower->GetPosition().GetCP();
          if (engGrenade)
            std::printf("engineer grenade 2: skill req %d, waves %d+delta/%d, damage %.1f+%.2f*delta, radius %.2f, item %d\n",
              engGrenade->nSkillReq, engGrenade->nStartNWave,
              engGrenade->nDeltaWave, engGrenade->fStartWaveDamage,
              engGrenade->fDamageModifier, engGrenade->fWaveRadius,
              engGrenade->pItem ? engGrenade->pItem->GetRecordID() : 0);
          else
            std::printf("grenade 21: waves %d, damage %.1f..%.1f, radius %.2f, structure coeff %.2f, max delay %d\n",
              grenade->nWaveNumber, grenade->fWaveDmgMin, grenade->fWaveDmgMax,
              grenade->fWaveRadius, grenade->fStructureDamageCoeff, grenade->nMaxDelay);
          std::printf("thrower %.2f %.2f %.2f, building %.2f %.2f %.2f\n",
            throwerPosition.x, throwerPosition.y, throwerPosition.z,
            epicentre.x, epicentre.y, epicentre.z);
          const CVec3 throwTarget = engGrenade
            ? throwerPosition + (epicentre - throwerPosition) * 0.3f : epicentre;
          // Hold process entropy fixed for comparison. The blast registry is
          // pointer-hashed, so its iteration/draw assignment can still vary
          // across processes and architectures; retail hashes pointers too.
#if defined(_WIN32)
          random.SeedForHarness(81021);
#else
          s2_game_random.SeedForHarness(81021);
#endif
          const auto miscBefore = world->GetMiscObjects()->size();
          int grenadeAPBefore = -1, grenadeAPCost = -1;
          if (missionPartyGrenadeInventorySave || missionPartyEngGrenadeInventorySave) {
            auto* inventory = thrower->GetUnitRPG()->GetInventory();
            if (missionPartyEngGrenadeInventorySave) {
              if (engGrenade->nSkillReq != 40 || engGrenade->nStartNWave != 2 ||
                  engGrenade->nDeltaWave != 45 || !engGrenade->pItem ||
                  engGrenade->pItem->GetRecordID() != 433) return 56;
              // The authored shooter has engineering 15, below every engineer
              // grenade's gate. Give this isolated mission test an expert thrower.
              thrower->GetUnitRPG()->GetRPGUnit()->Skills(NDb::ST_ENGINEERING).SetConst(100);
            }
            CObj<NRPG::IInventoryItem> displaced(inventory->TakeOff(NDb::SLOT_2));
            CObj<NRPG::IInventoryItem> grenadeItem(engGrenade
              ? NRPG::CreateItem(engGrenade) : NRPG::CreateGrenadeItem(grenade));
            if (!grenadeItem || !inventory->Equip(NDb::SLOT_2, grenadeItem) ||
                !inventory->Activate(NDb::SLOT_2)) return 54;
            grenadeAPBefore = thrower->GetAP();
            grenadeAPCost = thrower->GetActionAP(NRPG::AC_THROW_GRENADE);
            std::printf("grenade inventory pre-command: attack allowed %d, action type %d, command %d\n",
              world->IsAttackAllowed() ? 1 : 0, int(NWorld::GetActionType(thrower)),
              thrower->HasCommand() ? 1 : 0);
            if (missionPartyEngGrenadeInventorySave) {
              int startAP = -1, fullAP = -1;
              CObj<NWorld::CCmdShootTile> check(new NWorld::CCmdShootTile(throwTarget));
              if (thrower->CanDo(check.GetPtr(), &startAP, &fullAP) != NWorld::UCR_OK ||
                  fullAP != grenadeAPCost) return 57;
            }
            thrower->Do(new NWorld::CCmdSetCommand(thrower,
              new NWorld::CCmdShootTile(throwTarget)));
            thrower->Do(new NWorld::CCmdSetCommand(thrower,
              new NWorld::CCmdContinue()));
            std::string commandName;
            std::printf("grenade inventory throw: AP before %d, active slot %d, command %d, executor %s\n",
              grenadeAPBefore, inventory->GetActiveSlot(), thrower->HasCommand() ? 1 : 0,
              thrower->GetCurrentCommandName(&commandName) ? commandName.c_str() : "none");
          } else if (missionPartyGrenadeFlightSave)
            NWorld::UnitThrowGrenade(thrower, grenade, epicentre);
          else
            world->AddGrenadeExplosion(epicentre, grenade, thrower);
          if (missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave ||
              missionPartyEngGrenadeInventorySave)
            std::printf("grenade flight objects: %zu -> %zu\n", miscBefore,
              world->GetMiscObjects()->size());
          for (int tick = 220; tick < 420; ++tick) {
            world->UpdateWorld(tick * 50, nullptr);
            while (auto* raw = world->GetUICommand()) {
              CObj<NWorld::CUICmd> command(raw);
              if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
                world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
            }
            if (!NScript::luaLastError.szError.empty()) return 53;
          }
          if (missionPartyEngGrenadeInventorySave &&
              thrower->GetUnitRPG()->GetInventory()->Get(NDb::SLOT_2)) {
            // The first rotation triggers TBS_CANCEL_ACTION in this mission.
            // Re-issue the still-unspent action as a player can after an
            // interrupt notification.
            std::printf("engineer grenade retry after turn-based interrupt\n");
            thrower->Do(new NWorld::CCmdSetCommand(thrower,
              new NWorld::CCmdShootTile(throwTarget)));
            thrower->Do(new NWorld::CCmdSetCommand(thrower,
              new NWorld::CCmdContinue()));
            for (int tick = 420; tick < 620; ++tick) {
              world->UpdateWorld(tick * 50, nullptr);
              while (auto* raw = world->GetUICommand()) {
                CObj<NWorld::CUICmd> command(raw);
                if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
                  world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
              }
              if (!NScript::luaLastError.szError.empty()) return 53;
            }
          }
          if (missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave ||
              missionPartyEngGrenadeInventorySave)
            std::printf("grenade flight objects after action: %zu, AP %d, slot2 %d\n",
              world->GetMiscObjects()->size(), thrower->GetAP(),
              thrower->GetUnitRPG()->GetInventory()->Get(NDb::SLOT_2) ? 1 : 0);
          if ((missionPartyGrenadeInventorySave || missionPartyEngGrenadeInventorySave) &&
              (thrower->GetAP() != grenadeAPBefore - grenadeAPCost ||
               thrower->GetUnitRPG()->GetInventory()->Get(NDb::SLOT_2))) return 55;
        } else {
          world->Explode(epicentre, 1024);
        }
        unsigned long long liveAfter = 0, hpAfter = 0, hashAfter = 0;
        info.pGrid->GetVoxelStatsForHarness(&liveAfter, &hpAfter, &hashAfter);
        std::printf("building %d %s explosion: live %llu -> %llu, HP %llu -> %llu, hash %016llx -> %016llx\n",
          buildingIndex, missionPartyEngGrenadeInventorySave ? "engineer grenade inventory" :
            (missionPartyGrenadeInventorySave ? "grenade inventory" :
            (missionPartyGrenadeFlightSave ? "grenade flight" :
            (missionPartyGrenadeSave ? "grenade" : "world"))),
          liveBefore, liveAfter, hpBefore, hpAfter, hashBefore, hashAfter);
        if (hpAfter >= hpBefore || liveAfter > liveBefore || hashAfter == hashBefore ||
            ((missionPartyGrenadeSave || missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave ||
              missionPartyEngGrenadeInventorySave) &&
             liveAfter == liveBefore)) return 47;
        {
          CFileStream saved;
          saved.OpenWrite(argv[5]);
          CStructureSaver saver(saved, CStructureSaver::WRITE);
          saver.Add(2, &world);
          SerializeShared(&saver);
        }
        CObj<NWorld::CWorld> restored;
        {
          CFileStream saved;
          saved.OpenRead(argv[5]);
          CSharedHolder shared;
          CStructureSaver saver(saved, CStructureSaver::READ);
          saver.Add(2, &restored);
          SerializeShared(&saver);
        }
        if (!restored || !restored->GetGlobalGame()) return 48;
        restored->RestoreRuntimeCaches(restored->GetGlobalGame());
        int restoredIndex = 0;
        for (const auto& building : restored->GetBuildingsForHarness()) {
          if (restoredIndex++ != buildingIndex) continue;
          if (!building || !building->GetInfo().pGrid) return 49;
          unsigned long long restoredLive = 0, restoredHP = 0, restoredHash = 0;
          building->GetInfo().pGrid->GetVoxelStatsForHarness(&restoredLive, &restoredHP, &restoredHash);
          std::printf("restored building %d: live %llu, HP %llu, hash %016llx\n",
            buildingIndex, restoredLive, restoredHP, restoredHash);
          if (restoredLive != liveAfter || restoredHP != hpAfter || restoredHash != hashAfter) return 50;
          break;
        }
        if (restoredIndex <= buildingIndex) return 51;
      }
      if (missionPartyShot || missionPartyShotSave || missionPartyShotSlot) {
        auto* shooter = world->GetUnitServer("pers1");
        auto* hero = world->GetUnitServerByPersID(game->GetHero()->GetPers()->GetRecordID());
        if (!shooter || !hero) return 24;
        auto* weapon = shooter->GetUnitRPG()->GetWeaponItem();
        if (!weapon) return 25;
        const int ammoBeforeShot = weapon->GetAmmoQuantity();
        const int apBeforeShot = shooter->GetAP();
        NRPG::SUnitInfo heroBeforeShot{};
        hero->GetInfo(&heroBeforeShot);
        const int attackEventsBeforeShot = attackEvents->count;
        const int bulletEventsBeforeShot = attackEvents->bulletCount;
        // Use the game's own Lua-exposed to-hit override to make this
        // headless combat path independent of the process-level RNG seed.
        shooter->SetScriptToHit(100);
        shooter->Do(new NWorld::CCmdSetCommand(shooter,
          new NWorld::CCmdShootObject(hero, 0, NAI::HL_ANY)));
        shooter->Do(new NWorld::CCmdSetCommand(shooter, new NWorld::CCmdContinue()));
        bool observedShotExecutor = false;
        for (int tick = 220; tick < 440; ++tick) {
          world->UpdateWorld(tick * 50, nullptr);
          while (auto* raw = world->GetUICommand()) {
            CObj<NWorld::CUICmd> command(raw);
            if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
              world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
          }
          std::string name;
          if (shooter->GetCurrentCommandName(&name))
            observedShotExecutor = true;
          if (!NScript::luaLastError.szError.empty()) return 26;
        }
        NRPG::SUnitInfo heroAfterShot{};
        hero->GetInfo(&heroAfterShot);
        std::printf("full shot executor observed: %d\n", observedShotExecutor ? 1 : 0);
        std::printf("full shot ammo: %d -> %d\n", ammoBeforeShot, weapon->GetAmmoQuantity());
        std::printf("full shot AP: %d -> %d\n", apBeforeShot, shooter->GetAP());
        std::printf("full shot hero HP: %d -> %d\n", heroBeforeShot.nHP, heroAfterShot.nHP);
        std::printf("full shot attack events: %d -> %d\n", attackEventsBeforeShot, attackEvents->count);
        std::printf("full shot bullet events: %d -> %d\n", bulletEventsBeforeShot, attackEvents->bulletCount);
        if (!observedShotExecutor || shooter->HasCommand() ||
            weapon->GetAmmoQuantity() != ammoBeforeShot - 1 ||
            shooter->GetAP() != apBeforeShot - 10 ||
            heroAfterShot.nHP >= heroBeforeShot.nHP ||
            attackEvents->count != attackEventsBeforeShot + 1 ||
            attackEvents->bulletCount != bulletEventsBeforeShot + 1)
          return 27;
        if (missionPartyShotSave || missionPartyShotSlot) {
          if (missionPartyShotSlot) {
            auto* preSaveCommander = dynamic_cast<NAI::CAICommander*>(
              shooter->GetPlayer()->GetCommander());
            auto* preSaveAI = preSaveCommander ?
              preSaveCommander->GetAIUnit(shooter) : nullptr;
            if (!preSaveAI || preSaveAI->GetAIState() != preSaveCommander->GetAIState())
              return 42;
          }
          NMainLoop::CSaveManager* saveManager = nullptr;
          std::wstring slotPath;
          if (missionPartyShotSlot) {
            const char* userRoot = std::getenv("S2_USER_DATA_DIR");
            if (!userRoot || !*userRoot) return 37;
            NGlobal::RegisterVar("game_profile", nullptr, nullptr,
              NGlobal::CValue(std::wstring(L"Stage2Probe")), true);
            saveManager = NMainLoop::GetSaveManager();
            saveManager->CreateProfile("Stage2Probe");
            saveManager->SetActiveProfile("Stage2Probe");
            saveManager->PrepareSlot(NMainLoop::S_SLOT_ACTIVE);
            slotPath = saveManager->GetSlotFilePathW(
              NMainLoop::S_SLOT_ACTIVE, "world-mission-810-shot.sav");
            if (slotPath.empty()) return 38;
          }
          {
            CFileStream saved;
            if (missionPartyShotSlot) saved.OpenWrite(slotPath.c_str());
            else saved.OpenWrite(argv[5]);
            CStructureSaver saver(saved, CStructureSaver::WRITE);
            saver.Add(2, &world);
            SerializeShared(&saver);
          }
          if (missionPartyShotSlot) {
            saveManager->SaveSlot("ShotRoundtrip");
            saveManager->ClearSlot(NMainLoop::S_SLOT_ACTIVE);
            saveManager->LoadSlot("ShotRoundtrip");
          }
          CObj<NWorld::CWorld> restored;
          {
            CFileStream saved;
            if (missionPartyShotSlot) saved.OpenRead(slotPath.c_str());
            else saved.OpenRead(argv[5]);
            std::printf("world save bytes after shot: %d\n", saved.GetSize());
            CSharedHolder shared;
            CStructureSaver saver(saved, CStructureSaver::READ);
            saver.Add(2, &restored);
            SerializeShared(&saver);
          }
          std::printf("world restored after shot: %d\n", restored ? 1 : 0);
          if (!restored || !restored->GetGlobalGame() ||
              !restored->GetGlobalGame()->GetHero() ||
              !restored->GetTime() || !restored->GetOwnScript() ||
              restored->GetTime()->GetValue() != world->GetTime()->GetValue() ||
              restored->GetTimeOfDay() != world->GetTimeOfDay())
            return 28;
          {
            Script::AutoBlock stack(*restored->GetOwnScript());
            if (!restored->GetOwnScript()->GetGlobal("OnClickUsable").IsFunction())
              return 31;
          }
          auto* restoredHero = restored->GetUnitServerByPersID(
            restored->GetGlobalGame()->GetHero()->GetPers()->GetRecordID());
          auto* restoredShooter = restored->GetUnitServer("pers1");
          if (!restoredHero || !restoredShooter || !restoredShooter->GetUnitRPG())
            return 29;
          auto* restoredWeapon = restoredShooter->GetUnitRPG()->GetWeaponItem();
          NRPG::SUnitInfo restoredHeroInfo{};
          restoredHero->GetInfo(&restoredHeroInfo);
          std::printf("restored hero HP: %d; shooter ammo: %d; AP: %d\n",
            restoredHeroInfo.nHP, restoredWeapon ? restoredWeapon->GetAmmoQuantity() : -1,
            restoredShooter->GetAP());
          if (!restoredWeapon || restoredHeroInfo.nHP != heroAfterShot.nHP ||
              restoredWeapon->GetAmmoQuantity() != weapon->GetAmmoQuantity() ||
              restoredShooter->GetAP() != shooter->GetAP())
            return 30;
          const STime restoredTime = restored->GetTime()->GetValue();
          world = restored;
          game = world->GetGlobalGame();
          world->RestoreRuntimeCaches(game.GetPtr());
          auto* restoredCommander = dynamic_cast<NAI::CAICommander*>(
            restoredShooter->GetPlayer()->GetCommander());
          auto* restoredAI = restoredCommander ?
            restoredCommander->GetAIUnit(restoredShooter) : nullptr;
          if (!restoredAI || restoredAI->GetAIState() != restoredCommander->GetAIState())
            return 41;
          for (int tick = 440; tick < 450; ++tick) {
            world->UpdateWorld(tick * 50, nullptr);
            while (auto* raw = world->GetUICommand()) {
              CObj<NWorld::CUICmd> command(raw);
              if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
                world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
            }
            if (!NScript::luaLastError.szError.empty()) return 32;
          }
          if (world->GetTime()->GetValue() <= restoredTime ||
              !world->GetUnitServer("pers1") ||
              !world->GetUnitServerByPersID(game->GetHero()->GetPers()->GetRecordID()))
            return 33;
          std::printf("restored world advanced: %u -> %u\n",
            restoredTime, world->GetTime()->GetValue());
          auto* postLoadShooter = world->GetUnitServer("pers1");
          auto* postLoadHero = world->GetUnitServerByPersID(
            game->GetHero()->GetPers()->GetRecordID());
          auto* postLoadWeapon = postLoadShooter->GetUnitRPG()->GetWeaponItem();
          if (!postLoadWeapon) return 34;
          const int postLoadAmmo = postLoadWeapon->GetAmmoQuantity();
          const int postLoadAP = postLoadShooter->GetAP();
          NRPG::SUnitInfo postLoadHeroBefore{};
          postLoadHero->GetInfo(&postLoadHeroBefore);
          const int postLoadAttacks = attackEvents->count;
          const int postLoadBullets = attackEvents->bulletCount;
          postLoadShooter->SetScriptToHit(100);
          postLoadShooter->Do(new NWorld::CCmdSetCommand(postLoadShooter,
            new NWorld::CCmdShootObject(postLoadHero, 0, NAI::HL_ANY)));
          postLoadShooter->Do(new NWorld::CCmdSetCommand(postLoadShooter,
            new NWorld::CCmdContinue()));
          bool observedPostLoadExecutor = false;
          for (int tick = 450; tick < 670; ++tick) {
            world->UpdateWorld(tick * 50, nullptr);
            while (auto* raw = world->GetUICommand()) {
              CObj<NWorld::CUICmd> command(raw);
              if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
                world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
            }
            std::string name;
            if (postLoadShooter->GetCurrentCommandName(&name))
              observedPostLoadExecutor = true;
            if (!NScript::luaLastError.szError.empty()) return 35;
          }
          NRPG::SUnitInfo postLoadHeroAfter{};
          postLoadHero->GetInfo(&postLoadHeroAfter);
          std::printf("post-load shot ammo: %d -> %d; AP: %d -> %d; HP: %d -> %d; attacks: %d -> %d; bullets: %d -> %d\n",
            postLoadAmmo, postLoadWeapon->GetAmmoQuantity(),
            postLoadAP, postLoadShooter->GetAP(),
            postLoadHeroBefore.nHP, postLoadHeroAfter.nHP,
            postLoadAttacks, attackEvents->count,
            postLoadBullets, attackEvents->bulletCount);
          // ScriptToHit=100 fixes the hit roll, not cover penetration. A
          // completed shot can spend ammo/AP and emit both events while an
          // intervening object keeps the hero's HP unchanged.
          if (!observedPostLoadExecutor || postLoadShooter->HasCommand() ||
              postLoadWeapon->GetAmmoQuantity() != postLoadAmmo - 1 ||
              postLoadShooter->GetAP() != postLoadAP - 10 ||
              postLoadHeroAfter.nHP > postLoadHeroBefore.nHP ||
              attackEvents->count != postLoadAttacks + 1 ||
              attackEvents->bulletCount != postLoadBullets + 1)
            return 36;
          if (missionPartyShotSlot) {
            auto* current = world->GetCurrentPlayer();
            if (!current || current == postLoadShooter->GetPlayer()) return 39;
            auto* enemyCommander = dynamic_cast<NAI::CAICommander*>(
              postLoadShooter->GetPlayer()->GetCommander());
            if (!enemyCommander || !enemyCommander->HasVisibleEnemies() ||
                postLoadShooter->IsCheatEnabled(NRPG::CHEAT_NOAI)) return 43;
            postLoadShooter->SetScriptToHit(-1); // Let the AI use normal hit rules.
            const int aiAmmoBefore = postLoadWeapon->GetAmmoQuantity();
            const int aiBulletsBefore = attackEvents->bulletCount;
            current->GetCommander()->Do(new NWorld::CCmdEndOfTurn());
            bool observedEnemyTurn = false;
            bool observedAutonomousBullet = false;
            for (int tick = 670; tick < 1170; ++tick) {
              world->UpdateWorld(tick * 50, nullptr);
              while (auto* raw = world->GetUICommand()) {
                CObj<NWorld::CUICmd> command(raw);
                if (world->GetOwnScript()->IsUIActionIDPresent(command->GetID()))
                  world->ExecuteCommand(new NWorld::CCmdInterfaceEvent(command->GetID()));
              }
              if (world->GetCurrentPlayer() == postLoadShooter->GetPlayer())
                observedEnemyTurn = true;
              if (!NScript::luaLastError.szError.empty()) return 40;
              if (observedEnemyTurn && attackEvents->bulletCount > aiBulletsBefore &&
                  postLoadWeapon->GetAmmoQuantity() < aiAmmoBefore) {
                observedAutonomousBullet = true;
                std::printf("autonomous AI shot at tick %d: ammo %d -> %d, bullets %d -> %d\n",
                  tick, aiAmmoBefore, postLoadWeapon->GetAmmoQuantity(),
                  aiBulletsBefore, attackEvents->bulletCount);
                break;
              }
            }
            if (!observedEnemyTurn || !observedAutonomousBullet) return 44;
          }
        }
      }
    } else {
      world->UpdateWorld(before + 220 * 50, nullptr);
    }
    if (world->GetTime()->GetValue() <= before ||
        !NScript::luaLastError.szError.empty())
      return 13;
    std::printf("world mission variant %d post-init completed\n", variant);
  }
  return 0;
}
