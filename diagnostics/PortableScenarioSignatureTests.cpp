#include "../Main/ScenarioSignature.h"

#include <array>
#include <cstdint>
#include <initializer_list>

int main() {
  std::array<std::uint32_t, 3> signature{};
  for (int id : {0, 31, 32, 63, 64})
    signature[NScenario::ScenarioSignatureWord(id)] |=
      NScenario::ScenarioSignatureBit(id);
  if (signature[0] != UINT32_C(0x80000001) ||
      signature[1] != UINT32_C(0x80000001) ||
      signature[2] != UINT32_C(0x00000001)) return 1;
  for (int id : {0, 31, 32, 63, 64})
    if ((signature[NScenario::ScenarioSignatureWord(id)] &
         NScenario::ScenarioSignatureBit(id)) == 0) return 2;
  for (int id : {1, 30, 33, 62, 65})
    if ((signature[NScenario::ScenarioSignatureWord(id)] &
         NScenario::ScenarioSignatureBit(id)) != 0) return 3;
  return 0;
}
