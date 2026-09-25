#ifndef S2_MAIN_PORTABLE_CLIP_SHARE_CODECS_H
#define S2_MAIN_PORTABLE_CLIP_SHARE_CODECS_H

#include "GGeometry.h"
#include "GObjectInfo.h"
#include "../FileIO/PortableClipShareWire.h"

#include <cstddef>

static_assert(sizeof(NGScene::SClipShare) == 20, "building clip cache key size");
static_assert(offsetof(NGScene::SClipShare, src) == 0 &&
              offsetof(NGScene::SClipShare, dwParts) == 8 &&
              offsetof(NGScene::SClipShare, nSubBlockID) == 12 &&
              offsetof(NGScene::SClipShare, nClip) == 16,
              "building clip cache key offsets");

namespace S2FileIO {

template<>
struct StructureFieldCodec<NGScene::SClipShare, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 20;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NGScene::SClipShare* value) {
    if (!value) return false;
    ClipShareFields fields{};
    if (!DecodeClipShare(source, length, &fields)) return false;
    value->src.nID = fields.source.id;
    value->src.nPart = fields.source.option;
    value->dwParts = fields.parts;
    value->nSubBlockID = fields.subBlockId;
    value->nClip = fields.clip;
    return true;
  }
  static bool Encode(const NGScene::SClipShare& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeClipShare(ClipShareFields{
        {value.src.nID, value.src.nPart}, value.dwParts,
        value.nSubBlockID, value.nClip}, destination, length);
  }
};

} // namespace S2FileIO

#endif
