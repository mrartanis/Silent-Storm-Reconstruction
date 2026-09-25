#pragma once

// Linux bridge for the game's modified Lua 4 VM and native object serializer.
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
#define dbgnew new

using namespace std;

#include "../FileIO/BasicChunk1.h"

inline void OutputDebugString(const char* message) { std::fputs(message, stderr); }
inline void DebugTrace(const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
}
