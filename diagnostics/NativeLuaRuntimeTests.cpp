#include "../Script/StdAfx.h"
#include "../Script/lua.h"
#include "../Script/Script.h"
#include "../Script/lobject.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/RandomGen.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>

namespace NScript { int luaRandom(lua_State*); }

namespace {
int outputCalls = 0;
int CountOutput(lua_State* state) {
  if (lua_tostring(state, 1)) ++outputCalls;
  return 0;
}
}

int main(int argc, char** argv) {
  if (argc == 3 && std::strcmp(argv[1], "--autoload") == 0) {
    Script wrapper;
    wrapper.Register("out", CountOutput);
    wrapper.Register("Sleep", LuaCFuncSleep);
    wrapper.Register("StartThread", LuaCFuncStartThread);
    const char* names[] = {"Constants.l", "TriggersManager.l", "Common.l", "Hint.l"};
    std::string directory = argv[2];
    if (!directory.empty() && directory.back() != '/' && directory.back() != '\\')
      directory += '/';
    for (const char* name : names) {
      const std::string path = directory + name;
      std::ifstream input(path, std::ios::binary);
      if (!input) return 12;
      const std::string source((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
      if (source.empty() || wrapper.DoBuffer(source.data(), source.size(), path.c_str()))
        return 13;
    }
    for (int frame = 0; frame != 25; ++frame) wrapper.ExecuteThreads();
    if (wrapper.GetGlobal("DIR_RIGHT").GetNumber() != 0.0 ||
        wrapper.GetGlobal("DIR_DOWNRIGHT").GetNumber() != 7.0 ||
        wrapper.GetGlobal("maxTriggerIndex").GetNumber() != 0.0 ||
        wrapper.GetGlobal("N_WAIT_TIME_TO_SLEEP").GetNumber() != 2.0 ||
        outputCalls < 2) return 14;
    std::printf("native-lua autoload scripts 4 outputs %d\n", outputCalls);
    return 0;
  }
  if (argc == 2) {
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 4;
    const std::string source((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
    if (source.empty()) return 5;
    lua_State* state = lua_open(0);
    if (!state) return 6;
    const int status = lua_parsebuffer(state, source.data(), source.size(), argv[1]);
    lua_close(state);
    if (status != 0) return 7;
    std::printf("native-lua parsed %zu bytes from %s\n", source.size(), argv[1]);
    return 0;
  }
  if (argc != 1) return 8;
  lua_State* state = lua_open(0);
  if (!state) return 1;
  const char source[] =
      "result = 0\n"
      "for i = 1, 6 do result = result + i * i end\n"
      "function add(v) result = result + v end\n"
      "add(5)\n";
  if (lua_dobuffer(state, source, std::strlen(source), "headless.lua") != 0)
    return 2;
  lua_executeThreads(state);
  lua_getglobal(state, "result");
  const double result = lua_tonumber(state, -1);
  lua_pop(state, 1);
  if (result != 96.0) return 3;
  std::printf("native-lua result %.0f\n", result);
  lua_close(state);
  Script wrapper;
  if (wrapper.DoString("wrapper_result = 13 * 3") != 0) return 10;
  wrapper.ExecuteThreads();
  if (wrapper.GetGlobal("wrapper_result").GetNumber() != 39.0) return 11;
  CMemoryStream stateFile;
  {
    CStructureSaver saver(stateFile, CStructureSaver::WRITE);
    saver.Add(1, &wrapper);
  }
  stateFile.Seek(0);
  Script restored;
  {
    CStructureSaver saver(stateFile, CStructureSaver::READ);
    saver.Add(1, &restored);
  }
  if (restored.GetGlobal("wrapper_result").GetNumber() != 39.0) return 9;
  if (restored.DoString("wrapper_result = wrapper_result + 3") != 0) return 15;
  restored.ExecuteThreads();
  if (restored.GetGlobal("wrapper_result").GetNumber() != 42.0) return 16;

  // Numeric Lua table keys exercise ObjectHash's Number -> long cast.
  // The baseline scripts have no wide integer constants, but runtime
  // arithmetic or scripts can still create such keys. Save/load must not
  // alias them or lose the signed-32-bit boundary values.
  Script numericKeys;
  const char keySource[] =
      "keys = {}\n"
      "keys[2147483647] = 11\n"
      "keys[2147483648] = 12\n"
      "keys[-2147483648] = 13\n"
      "keys[-2147483649] = 14\n"
      "keys[4294967295] = 15\n"
      "key_signature = keys[2147483647] + keys[2147483648] * 100 + "
      "keys[-2147483648] * 10000 + keys[-2147483649] * 1000000 + "
      "keys[4294967295] * 100000000\n";
  if (numericKeys.DoString(keySource) != 0) return 19;
  numericKeys.ExecuteThreads();
  if (numericKeys.GetGlobal("key_signature").GetNumber() != 1514131211.0) return 20;
  CMemoryStream numericState;
  {
    CStructureSaver saver(numericState, CStructureSaver::WRITE);
    saver.Add(1, &numericKeys);
  }
  if (numericState.GetSize() != 20365) return 23;
  ObjectHash numericHasher;
  const unsigned expectedHashes[] = {
      0x7FFFFFFFu, 0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u,
      0x7FFFFFFFu, 0x80000000u, 1u, 0xFFFFFFFFu};
  int hashIndex = 0;
  for (double value : {2147483647.0, 2147483648.0, -2147483648.0,
                       -2147483649.0, 4294967295.0, 2147483647.75,
                       -2147483648.75, 1.9, -1.9}) {
    TObject key;
    key.SetN(value);
#if defined(_WIN32)
    volatile double retailInput = value;
    const unsigned retailWord = static_cast<unsigned long>(static_cast<long>(retailInput));
    if (retailWord != expectedHashes[hashIndex]) return 25;
#endif
    if (static_cast<unsigned>(numericHasher(key)) != expectedHashes[hashIndex++])
      return 24;
    std::printf("native-lua numeric hash %.17g=%08X\n", value,
                static_cast<unsigned>(numericHasher(key)));
  }
  std::uint64_t numericWireDigest = UINT64_C(14695981039346656037);
  for (int byte = 0; byte < numericState.GetSize(); ++byte) {
    numericWireDigest ^= numericState.GetBuffer()[byte];
    numericWireDigest *= UINT64_C(1099511628211);
  }
  numericState.Seek(0);
  Script restoredKeys;
  {
    CStructureSaver saver(numericState, CStructureSaver::READ);
    saver.Add(1, &restoredKeys);
  }
  if (restoredKeys.DoString("key_signature = keys[2147483647] + "
                            "keys[2147483648] * 100 + keys[-2147483648] * 10000 + "
                            "keys[-2147483649] * 1000000 + keys[4294967295] * 100000000") != 0) return 21;
  restoredKeys.ExecuteThreads();
  if (restoredKeys.GetGlobal("key_signature").GetNumber() != 1514131211.0) return 22;
  std::printf("native-lua numeric keys 5 signature %.0f saved_bytes %d wire=%016llX\n",
              restoredKeys.GetGlobal("key_signature").GetNumber(), numericState.GetSize(),
              static_cast<unsigned long long>(numericWireDigest));

  // The authored DB scripts call the game's actual random binding (for
  // example, the patrol branch in mission variant 4526).
#if defined(_WIN32)
  CRandomGenerator& gameRandom = random;
#else
  CRandomGenerator& gameRandom = s2_game_random;
#endif
  gameRandom.SeedForHarness(2026);
  const unsigned int expectedOne = gameRandom.Get(29);
  const unsigned int expectedRange = gameRandom.Get(5, 20);
  gameRandom.SeedForHarness(2026);
  Script mission;
  mission.Register("random", NScript::luaRandom);
  if (mission.DoString("patrol = random(29); selected = random(5, 20); invalid = random('bad')") != 0)
    return 17;
  mission.ExecuteThreads();
  if (mission.GetGlobal("patrol").GetNumber() != expectedOne ||
      mission.GetGlobal("selected").GetNumber() != expectedRange ||
      mission.GetGlobal("invalid").GetNumber() != 0.0)
    return 18;
  std::printf("native-lua game random patrol=%u selected=%u\n",
              expectedOne, expectedRange);
  return 0;
}
