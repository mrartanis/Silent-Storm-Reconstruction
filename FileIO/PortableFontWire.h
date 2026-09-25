#ifndef S2_FILEIO_PORTABLE_FONT_WIRE_H
#define S2_FILEIO_PORTABLE_FONT_WIRE_H

#include "PortableStructureChunks.h"

#include <cstddef>
#include <cstdint>

namespace S2FileIO {

// The game font resource stores seven consecutive signed 32-bit fields.
struct FontCharacterFields {
  std::int32_t x1, y1, x2, y2;
  std::int32_t advanceA, advanceBC, width;
};

inline bool DecodeFontCharacter(const std::uint8_t* source, std::size_t length,
                                FontCharacterFields* value) {
  if (!source || !value || length != 28) return false;
  FontCharacterFields result{};
  std::int32_t* fields[] = {&result.x1, &result.y1, &result.x2, &result.y2,
                           &result.advanceA, &result.advanceBC, &result.width};
  for (std::size_t i = 0; i < 7; ++i)
    if (!DecodeStructureScalar(source + 4 * i, 4, fields[i])) return false;
  *value = result;
  return true;
}

inline bool EncodeFontCharacter(const FontCharacterFields& value,
                                std::uint8_t* destination, std::size_t length) {
  if (!destination || length != 28) return false;
  const std::int32_t* fields[] = {&value.x1, &value.y1, &value.x2, &value.y2,
                                 &value.advanceA, &value.advanceBC, &value.width};
  for (std::size_t i = 0; i < 7; ++i)
    if (!EncodeStructureScalar(*fields[i], destination + 4 * i, 4)) return false;
  return true;
}

} // namespace S2FileIO

#endif
