#ifndef S2_MAIN_SELECTION_INFO_WIRE_H
#define S2_MAIN_SELECTION_INFO_WIRE_H

#include "../FileIO/GeometryWire.h"
#include "../FileIO/PortableSelectionInfoWire.h"

namespace NRender {

// Retail NRender::SSelectionInfo: color at +0, floor-mask flag at +16.
struct SSelectionInfo {
  CVec4 vColor;
  bool bIgnoreFloorMask;

  SSelectionInfo(): vColor(0, 1, 1, 1), bIgnoreFloorMask(false) {}
  SSelectionInfo(const CVec4& color, bool ignoreFloorMask)
      : vColor(color), bIgnoreFloorMask(ignoreFloorMask) {}
};

} // namespace NRender

static_assert(sizeof(NRender::SSelectionInfo) == 20, "selection info wire size");
static_assert(offsetof(NRender::SSelectionInfo, bIgnoreFloorMask) == 16,
              "selection info flag offset");

namespace S2FileIO {

template<>
struct StructureFieldCodec<NRender::SSelectionInfo, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 20;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NRender::SSelectionInfo* value) {
    if (!value) return false;
    SelectionInfoFields fields{};
    if (!DecodeSelectionInfo(source, length, &fields)) return false;
    value->vColor = CVec4(fields.color[0], fields.color[1],
                          fields.color[2], fields.color[3]);
    value->bIgnoreFloorMask = fields.ignoreFloorMask;
    return true;
  }
  static bool Encode(const NRender::SSelectionInfo& value,
                     std::uint8_t* destination, std::size_t length) {
    const SelectionInfoFields fields{{value.vColor.x, value.vColor.y,
                                      value.vColor.z, value.vColor.w},
                                     value.bIgnoreFloorMask};
    return EncodeSelectionInfo(fields, destination, length);
  }
};

} // namespace S2FileIO

#endif
