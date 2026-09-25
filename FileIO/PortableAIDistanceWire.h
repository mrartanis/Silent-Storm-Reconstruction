#ifndef S2_FILEIO_PORTABLE_AI_DISTANCE_WIRE_H
#define S2_FILEIO_PORTABLE_AI_DISTANCE_WIRE_H

#include "PortableStructureChunks.h"
#include "../Misc/PortableZone.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace S2FileIO {

struct DistanceZoneFields {
  std::uint16_t color;
  std::int32_t layer;
};

struct AIDistanceFields {
  std::uint16_t distance;
  DistanceZoneFields parent;
  std::uint8_t proceeded, inList;
  DistanceZoneFields previous, next;
};

inline bool DecodeAIDistance(const std::uint8_t* source, std::size_t length,
                             AIDistanceFields* value) {
  if (!source || !value || length != 32) return false;
  AIDistanceFields result{};
  if (!DecodeStructureScalar(source, 2, &result.distance) ||
      !S2AI::DecodeZone(source + 4, 8, &result.parent.color,
                       &result.parent.layer) ||
      !S2AI::DecodeZone(source + 16, 8, &result.previous.color,
                       &result.previous.layer) ||
      !S2AI::DecodeZone(source + 24, 8, &result.next.color,
                       &result.next.layer)) return false;
  result.proceeded = source[12];
  result.inList = source[13];
  *value = result;
  return true;
}

inline bool EncodeAIDistance(const AIDistanceFields& value,
                             std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 32) return false;
  std::memset(destination, 0, length);
  if (!EncodeStructureScalar(value.distance, destination, 2) ||
      !S2AI::EncodeZone(value.parent.color, value.parent.layer,
                       destination + 4, 8) ||
      !S2AI::EncodeZone(value.previous.color, value.previous.layer,
                       destination + 16, 8) ||
      !S2AI::EncodeZone(value.next.color, value.next.layer,
                       destination + 24, 8)) return false;
  destination[12] = value.proceeded ? 1 : 0;
  destination[13] = value.inList ? 1 : 0;
  return true;
}

} // namespace S2FileIO

#endif
