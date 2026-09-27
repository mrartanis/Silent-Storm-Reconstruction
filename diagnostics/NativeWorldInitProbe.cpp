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
#include "../DBFormat/DataRPG.h"
#include "../DBFormat/DataScript.h"
#include "../Main/GSceneUtils.h"
#include "../Main/wUnitAttack.h"
#include "../Main/A5Script.h"
#include "../Main/BuildingGrid.h"
#include "../Main/aiCommander.h"
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
#include <set>

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
  const bool missionPartyUIAck = argc == 5 && std::strcmp(argv[3], "--mission-party-ui-ack") == 0;
  const bool missionPartyShot = argc == 5 && std::strcmp(argv[3], "--mission-party-shot") == 0;
  const bool missionPartyShotSave = argc == 6 && std::strcmp(argv[3], "--mission-party-shot-save") == 0;
  const bool missionPartyShotSlot = argc == 5 && std::strcmp(argv[3], "--mission-party-shot-slot") == 0;
  const bool missionPartyExplosionSave = argc == 6 && std::strcmp(argv[3], "--mission-party-explosion-save") == 0;
  const bool missionPartyGrenadeSave = argc == 6 && std::strcmp(argv[3], "--mission-party-grenade-save") == 0;
  const bool missionPartyGrenadeFlightSave = argc == 6 && std::strcmp(argv[3], "--mission-party-grenade-flight-save") == 0;
  const bool missionPartyGrenadeInventorySave = argc == 6 && std::strcmp(argv[3], "--mission-party-grenade-inventory-save") == 0;
  const bool missionParty = missionPartyUIAck || missionPartyShot ||
    missionPartyShotSave || missionPartyShotSlot || missionPartyExplosionSave ||
    missionPartyGrenadeSave || missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave;
  const bool missionWithUIAck = missionUIAck || missionParty;
  if (argc != 3 && !mission && !missionWithUIAck) return 2;
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
  if (missionParty)
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
    if (variant == 810) {
      if (post->scripts.size() != 1) return 10;
      world->SetTimeOfDay(NWorld::TOD_NIGHT);
    }
    if (missionParty) {
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
      if (variant == 810 && acknowledged == 0) return 15;
      if (variant == 810) {
        Script::AutoBlock stack(*world->GetOwnScript());
        if (!world->GetOwnScript()->GetGlobal("OnClickUsable").IsFunction())
          return 16; // Defined only after the authored opening sequence.
      }
      if (missionPartyExplosionSave || missionPartyGrenadeSave || missionPartyGrenadeFlightSave ||
          missionPartyGrenadeInventorySave) {
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
        if (missionPartyGrenadeSave || missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave) {
          auto* grenade = NDb::GetRPGGrenade(21); // Retail's scripted blast record.
          auto* thrower = world->GetUnitServer("pers1");
          if (!grenade || !thrower || !world->GetExplosionMasterForHarness()) return 52;
          const CVec3 throwerPosition = thrower->GetPosition().GetCP();
          std::printf("grenade 21: waves %d, damage %.1f..%.1f, radius %.2f, structure coeff %.2f, max delay %d\n",
            grenade->nWaveNumber, grenade->fWaveDmgMin, grenade->fWaveDmgMax,
            grenade->fWaveRadius, grenade->fStructureDamageCoeff, grenade->nMaxDelay);
          std::printf("thrower %.2f %.2f %.2f, building %.2f %.2f %.2f\n",
            throwerPosition.x, throwerPosition.y, throwerPosition.z,
            epicentre.x, epicentre.y, epicentre.z);
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
          if (missionPartyGrenadeInventorySave) {
            auto* inventory = thrower->GetUnitRPG()->GetInventory();
            CObj<NRPG::IInventoryItem> displaced(inventory->TakeOff(NDb::SLOT_2));
            CObj<NRPG::IInventoryItem> grenadeItem(NRPG::CreateGrenadeItem(grenade));
            if (!grenadeItem || !inventory->Equip(NDb::SLOT_2, grenadeItem) ||
                !inventory->Activate(NDb::SLOT_2)) return 54;
            grenadeAPBefore = thrower->GetAP();
            grenadeAPCost = thrower->GetActionAP(NRPG::AC_THROW_GRENADE);
            std::printf("grenade inventory pre-command: attack allowed %d, action type %d, command %d\n",
              world->IsAttackAllowed() ? 1 : 0, int(NWorld::GetActionType(thrower)),
              thrower->HasCommand() ? 1 : 0);
            thrower->Do(new NWorld::CCmdSetCommand(thrower,
              new NWorld::CCmdShootTile(epicentre)));
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
          if (missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave)
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
          if (missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave)
            std::printf("grenade flight objects after 200 ticks: %zu, AP %d, slot2 %d\n",
              world->GetMiscObjects()->size(), thrower->GetAP(),
              thrower->GetUnitRPG()->GetInventory()->Get(NDb::SLOT_2) ? 1 : 0);
          if (missionPartyGrenadeInventorySave &&
              (thrower->GetAP() != grenadeAPBefore - grenadeAPCost ||
               thrower->GetUnitRPG()->GetInventory()->Get(NDb::SLOT_2))) return 55;
        } else {
          world->Explode(epicentre, 1024);
        }
        unsigned long long liveAfter = 0, hpAfter = 0, hashAfter = 0;
        info.pGrid->GetVoxelStatsForHarness(&liveAfter, &hpAfter, &hashAfter);
        std::printf("building %d %s explosion: live %llu -> %llu, HP %llu -> %llu, hash %016llx -> %016llx\n",
          buildingIndex, missionPartyGrenadeInventorySave ? "grenade inventory" :
            (missionPartyGrenadeFlightSave ? "grenade flight" :
            (missionPartyGrenadeSave ? "grenade" : "world")),
          liveBefore, liveAfter, hpBefore, hpAfter, hashBefore, hashAfter);
        if (hpAfter >= hpBefore || liveAfter > liveBefore || hashAfter == hashBefore ||
            ((missionPartyGrenadeSave || missionPartyGrenadeFlightSave || missionPartyGrenadeInventorySave) &&
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
          if (!observedPostLoadExecutor || postLoadShooter->HasCommand() ||
              postLoadWeapon->GetAmmoQuantity() != postLoadAmmo - 1 ||
              postLoadShooter->GetAP() != postLoadAP - 10 ||
              postLoadHeroAfter.nHP >= postLoadHeroBefore.nHP ||
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
