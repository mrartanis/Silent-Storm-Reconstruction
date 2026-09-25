#pragma once

// Runtime-only Linux bridge for the game's modified Lua 4 VM. Persistence is
// intentionally unavailable until CStructureSaver itself is ported: any
// attempted save/load fails loudly instead of silently corrupting state.
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <new>

#define ASSERT(value) assert(value)
#define externA5 extern
#define __cdecl
#define ZDATA
#define ZEND
#define ZDATA_(base)
#define REGISTER_SAVELOAD_CLASS(id, type)
#define dbgnew new

using namespace std;

typedef char chunk_id;
class CStructureSaver {
 public:
  bool IsReading() const {
    throw std::logic_error("Lua persistence is not ported to Linux");
  }
  template<class T>
  void Add(chunk_id, T*, int = 1) {
    throw std::logic_error("Lua persistence is not ported to Linux");
  }
};

inline void OutputDebugString(const char* message) { std::fputs(message, stderr); }
inline void DebugTrace(const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
}

#include "../Misc/Basic2.h"
