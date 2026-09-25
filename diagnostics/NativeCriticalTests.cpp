#include "../Main/StdAfx.h"
#include "../Main/RPGCritical.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

static_assert(sizeof(NRPG::SCritical) == 20, "historical critical size");
static_assert(offsetof(NRPG::SCritical, nDC) == 0, "difficulty offset");
static_assert(offsetof(NRPG::SCritical, nDuration) == 4, "duration offset");
static_assert(offsetof(NRPG::SCritical, fValue) == 8, "value offset");
static_assert(offsetof(NRPG::SCritical, eCritical) == 12, "critical offset");
static_assert(offsetof(NRPG::SCritical, eCl) == 16, "location offset");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NRPG::SCritical>;
  if (!Codec::kPortable || Codec::kWireSize != 20) return 1;
  const NDb::ECritical criticals[] = {NDb::C_DEATH, NDb::C_BLEEDING,
                                      NDb::C_NONE, NDb::N_CRIT_TYPES};
  const NDb::ECriticalLocation locations[] = {NDb::CL_HEAD, NDb::CL_ANY, NDb::N_CL};
  for (auto critical : criticals) for (auto location : locations) {
    NRPG::SCritical original(location, critical, -2, 1.5f, 15);
    std::uint8_t wire[20] = {};
    NRPG::SCritical loaded;
    if (!Codec::Encode(original, wire, 20) ||
        std::memcmp(wire, &original, 20) != 0 ||
        !Codec::Decode(wire, 20, &loaded) ||
        std::memcmp(&loaded, &original, 20) != 0) return 2;
  }
  const S2FileIO::CriticalFields invalid{1, -1, 0.0f, -1, 0};
  std::uint8_t wire[20] = {};
  NRPG::SCritical loaded;
  if (!S2FileIO::EncodeCritical(invalid, wire, 20) ||
      Codec::Decode(wire, 20, &loaded)) return 3;
  return 0;
}
