#include "../Script/lopcodes.h"

#include <cstdint>
#include <cstdio>
#include <limits>

static_assert(sizeof(Instruction) == 4, "Lua VM instructions must be 32 bits");
static_assert(sizeof(lint32) == 4, "Lua allocator words must be 32 bits");
static_assert(SIZE_INSTRUCTION == 32, "Lua bytecode uses 32-bit instructions");

int main() {
  const Instruction words[] = {
      CREATE_0(OP_END), CREATE_U(OP_PUSHSTRING, 1234),
      CREATE_AB(OP_CALL, 17, 271), CREATE_S(OP_PUSHINT, -42)};
  if (GET_OPCODE(words[0]) != OP_END ||
      GET_OPCODE(words[1]) != OP_PUSHSTRING || GETARG_U(words[1]) != 1234 ||
      GET_OPCODE(words[2]) != OP_CALL || GETARG_A(words[2]) != 17 ||
      GETARG_B(words[2]) != 271 || GET_OPCODE(words[3]) != OP_PUSHINT ||
      GETARG_S(words[3]) != -42) return 1;

  int stackValue = 0;
  const auto pointer = reinterpret_cast<std::uintptr_t>(&stackValue);
  if (IntPoint(&stackValue) != (pointer >> 3) ||
      IntPoint(-1) != (static_cast<std::uintptr_t>(UINT32_MAX) >> 3)) return 2;

  std::uint64_t hash = UINT64_C(14695981039346656037);
  for (Instruction word : words)
    for (unsigned byte = 0; byte != 4; ++byte)
      hash = (hash ^ static_cast<std::uint8_t>(word >> (8 * byte))) *
          UINT64_C(1099511628211);
  std::printf("lua-words %016llx\n", static_cast<unsigned long long>(hash));
  return 0;
}
