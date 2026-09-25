#include "../Main/StdAfx.h"
#include "../Main/RPGVisionQuery.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

static_assert(offsetof(NRPG::SVisionQuery, vFrom) == 0 &&
              offsetof(NRPG::SVisionQuery, vWhat) == 12 &&
              offsetof(NRPG::SVisionQuery, fRange) == 24 &&
              offsetof(NRPG::SVisionQuery, fCosFOV) == 28,
              "vision query field offsets");

int main() {
  using Codec = S2FileIO::StructureFieldCodec<NRPG::SVisionQuery>;
  if (!Codec::kPortable || Codec::kWireSize != 32) return 1;
  NRPG::SVisionQuery query, decoded;
  query.vFrom = CVec3(1.0f, -2.0f, 3.0f);
  query.vWhat = CVec3(-4.0f, 5.0f, -0.0f);
  query.fRange = 40.0f;
  query.fCosFOV = 0.5f;
  std::uint8_t wire[32] = {};
  if (!Codec::Encode(query, wire, sizeof(wire)) ||
      std::memcmp(wire, &query, sizeof(wire)) != 0 ||
      !Codec::Decode(wire, sizeof(wire), &decoded) ||
      std::memcmp(&query, &decoded, sizeof(query)) != 0 ||
      Codec::Decode(wire, 31, &decoded) ||
      Codec::Encode(query, wire, 31)) return 1;
  return 0;
}
