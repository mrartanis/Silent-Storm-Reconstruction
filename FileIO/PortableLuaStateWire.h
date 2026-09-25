#ifndef S2_FILEIO_PORTABLE_LUA_STATE_WIRE_H
#define S2_FILEIO_PORTABLE_LUA_STATE_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

inline bool DecodeLuaStateIntegers(const std::uint8_t* source,
                                   std::size_t length,
                                   std::int32_t* values,
                                   std::size_t count) {
  if (!source || !values || count > 15 || length != count * 4) return false;
  for (std::size_t i = 0; i < count; ++i)
    if (!DecodeStructureScalar(source + i * 4, 4, &values[i])) return false;
  return true;
}

inline bool EncodeLuaStateIntegers(const std::int32_t* values,
                                   std::size_t count,
                                   std::uint8_t* destination,
                                   std::size_t length) {
  if (!values || !destination || count > 15 || length != count * 4)
    return false;
  for (std::size_t i = 0; i < count; ++i)
    if (!EncodeStructureScalar(values[i], destination + i * 4, 4))
      return false;
  return true;
}

} // namespace S2FileIO

#endif
