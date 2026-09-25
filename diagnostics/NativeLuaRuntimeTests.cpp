#include "../Script/StdAfx.h"
#include "../Script/lua.h"
#include "../Script/Script.h"
#ifndef _WIN32
#include "../Script/lstate.h"
#endif

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

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
#ifndef _WIN32
  bool persistenceBlocked = false;
  try {
    CStructureSaver unsupported;
    state->operator&(unsupported);
  } catch (const std::logic_error&) {
    persistenceBlocked = true;
  }
  if (!persistenceBlocked) return 9;
#endif
  std::printf("native-lua result %.0f\n", result);
  lua_close(state);
  Script wrapper;
  if (wrapper.DoString("wrapper_result = 13 * 3") != 0) return 10;
  wrapper.ExecuteThreads();
  if (wrapper.GetGlobal("wrapper_result").GetNumber() != 39.0) return 11;
  return 0;
}
