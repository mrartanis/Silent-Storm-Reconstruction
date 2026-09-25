#ifndef S2_FILEIO_PORTABLE_BOOL_SYNC_WIRE_H
#define S2_FILEIO_PORTABLE_BOOL_SYNC_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace S2FileIO {

struct BoolSyncObjectFields {
  std::int32_t mask, trackId;
};

inline bool DecodeBoolSyncObject(const std::uint8_t* source,
                                 std::size_t length,
                                 BoolSyncObjectFields* value) {
  if (!source || !value || length != 8) return false;
  BoolSyncObjectFields result{};
  if (!DecodeStructureScalar(source, 4, &result.mask) ||
      !DecodeStructureScalar(source + 4, 4, &result.trackId)) return false;
  *value = result;
  return true;
}

inline bool EncodeBoolSyncObject(const BoolSyncObjectFields& value,
                                 std::uint8_t* destination,
                                 std::size_t length) {
  return destination && length == 8 &&
      EncodeStructureScalar(value.mask, destination, 4) &&
      EncodeStructureScalar(value.trackId, destination + 4, 4);
}

template<class...> using BoolSyncVoid = void;
template<class T, class = void>
struct HasBoolSyncObjectWireTag : std::false_type {};
template<class T>
struct HasBoolSyncObjectWireTag<T,
    BoolSyncVoid<typename T::S2BoolSyncObjectInfoWire>> : std::true_type {};

// CBoolSyncSrc has a private nested value type for every T/TFunc pair. The
// marker selects only those values without exposing or renaming their types.
template<class T>
struct StructureFieldCodec<T, typename std::enable_if<
    HasBoolSyncObjectWireTag<T>::value>::type> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 8;
  static bool Decode(const std::uint8_t* source, std::size_t length, T* value) {
    if (!value) return false;
    BoolSyncObjectFields fields{};
    if (!DecodeBoolSyncObject(source, length, &fields)) return false;
    value->nMask = fields.mask;
    value->nTrackID = fields.trackId;
    return true;
  }
  static bool Encode(const T& value, std::uint8_t* destination,
                     std::size_t length) {
    return EncodeBoolSyncObject(BoolSyncObjectFields{
        value.nMask, value.nTrackID}, destination, length);
  }
};

} // namespace S2FileIO

#endif
