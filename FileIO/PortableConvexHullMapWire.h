#ifndef S2_FILEIO_PORTABLE_CONVEX_HULL_MAP_WIRE_H
#define S2_FILEIO_PORTABLE_CONVEX_HULL_MAP_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace S2FileIO {

struct ConvexHullMapFields {
  std::int32_t pieceId, userId;
};

inline bool DecodeConvexHullMap(const std::uint8_t* source,
                                std::size_t length,
                                ConvexHullMapFields* value) {
  if (!source || !value || length != 8) return false;
  ConvexHullMapFields result{};
  if (!DecodeStructureScalar(source, 4, &result.pieceId) ||
      !DecodeStructureScalar(source + 4, 4, &result.userId)) return false;
  *value = result;
  return true;
}

inline bool EncodeConvexHullMap(const ConvexHullMapFields& value,
                                std::uint8_t* destination,
                                std::size_t length) {
  return destination && length == 8 &&
      EncodeStructureScalar(value.pieceId, destination, 4) &&
      EncodeStructureScalar(value.userId, destination + 4, 4);
}

template<class...> using ConvexHullMapVoid = void;
template<class T, class = void>
struct HasConvexHullMapWireTag : std::false_type {};
template<class T>
struct HasConvexHullMapWireTag<T,
    ConvexHullMapVoid<typename T::S2ConvexHullMapWire>> : std::true_type {};

// The game type is declared only in aiMap.cpp. A marker keeps its nested
// identity and serialization ID unchanged while selecting this codec.
template<class T>
struct StructureFieldCodec<T, typename std::enable_if<
    HasConvexHullMapWireTag<T>::value>::type> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 8;
  static bool Decode(const std::uint8_t* source, std::size_t length, T* value) {
    if (!value) return false;
    ConvexHullMapFields fields{};
    if (!DecodeConvexHullMap(source, length, &fields)) return false;
    value->nPieceID = fields.pieceId;
    value->nUserID = fields.userId;
    return true;
  }
  static bool Encode(const T& value, std::uint8_t* destination,
                     std::size_t length) {
    return EncodeConvexHullMap(ConvexHullMapFields{
        value.nPieceID, value.nUserID}, destination, length);
  }
};

} // namespace S2FileIO

#endif
