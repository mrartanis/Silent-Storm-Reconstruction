#ifndef S2_MAIN_RPG_VISION_QUERY_H
#define S2_MAIN_RPG_VISION_QUERY_H

#include "../Misc/Geom.h"
#include "../FileIO/PortableVisionQueryWire.h"

namespace NRPG {

// Retail MakeVisionQuery uses this full 0x20-byte cache key.
struct SVisionQuery {
  CVec3 vFrom, vWhat;
  float fRange, fCosFOV;
  SVisionQuery(): fRange(0), fCosFOV(0) {}
  bool operator==(const SVisionQuery& other) const {
    return vFrom == other.vFrom && vWhat == other.vWhat &&
           fRange == other.fRange && fCosFOV == other.fCosFOV;
  }
};

struct SVisionQueryHash {
  int operator()(const SVisionQuery& query) const {
    SVec3Hash hash;
    return hash(query.vFrom) + hash(query.vWhat) +
           static_cast<int>(query.fRange * 16) +
           static_cast<int>(query.fCosFOV * 1024);
  }
};

} // namespace NRPG

static_assert(sizeof(NRPG::SVisionQuery) == 32, "vision query wire size");
namespace S2FileIO {
template<>
struct StructureFieldCodec<NRPG::SVisionQuery, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 32;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NRPG::SVisionQuery* value) {
    if (!value) return false;
    VisionQueryFields fields{};
    if (!DecodeVisionQuery(source, length, &fields)) return false;
    value->vFrom = CVec3(fields.from[0], fields.from[1], fields.from[2]);
    value->vWhat = CVec3(fields.target[0], fields.target[1], fields.target[2]);
    value->fRange = fields.range;
    value->fCosFOV = fields.cosFov;
    return true;
  }
  static bool Encode(const NRPG::SVisionQuery& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeVisionQuery(VisionQueryFields{
        {value.vFrom.x, value.vFrom.y, value.vFrom.z},
        {value.vWhat.x, value.vWhat.y, value.vWhat.z},
        value.fRange, value.fCosFOV}, destination, length);
  }
};
} // namespace S2FileIO

#endif
