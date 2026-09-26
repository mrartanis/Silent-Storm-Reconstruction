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
#include "../DBFormat/DataScript.h"
#include "../Main/GSceneUtils.h"
#include "../Main/A5Script.h"
#include "../Main/rpgGlobal.h"
#include "../Main/wMain.h"
#include "../Misc/RandomGen.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
  const bool mission = argc == 5 && std::strcmp(argv[3], "--mission") == 0;
  if (argc != 3 && !mission) return 2;
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
  CObj<NRPG::CGlobalGame> game = mission
    ? NRPG::CreateGlobalGame() : new NRPG::CGlobalGame;
  CObj<NWorld::CWorld> world = new NWorld::CWorld(game);
  if (!world || world->GetGlobalGame() != game.GetPtr()) return 6;
  world->ExecuteOwnScript();
  if (!world->GetOwnScript() ||
      world->GetOwnScript()->GetGlobal("DIR_DOWNRIGHT").GetNumber() != 7.0 ||
      world->GetOwnScript()->GetGlobal("maxTriggerIndex").GetNumber() != 0.0 ||
      world->GetOwnScript()->GetGlobal("N_WAIT_TIME_TO_SLEEP").GetNumber() != 2.0)
    return 7;
  std::printf("world initialized with %d autoload scripts\n", scriptCount);
  if (mission) {
    const int variant = std::atoi(argv[4]);
    CObj<NWorld::CPostWorldCreateInfo> post;
    world->CreateRandom(variant, std::vector<std::string>(), true,
      std::list<CPtr<NScenario::CScenarioClue>>(), 0, &post,
      SRandomSeed(123));
    if (!world->GetAIMap() || !world->GetPathNetwork() || !post)
      return 9;
    std::printf("world mission variant %d built\n", variant);
    std::printf("mission scripts queued: %zu\n", post->scripts.size());
    world->RunPostInit(post);
    std::printf("world mission variant %d post-init completed\n", variant);
  }
  return 0;
}
