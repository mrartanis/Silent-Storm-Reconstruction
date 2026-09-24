#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace S2FileIO {

#if defined(_MSC_VER)
#define S2_STRUCTURE_CALL __cdecl
#else
#define S2_STRUCTURE_CALL
#endif

struct StructureChunk {
  std::uint8_t id = 0;
  std::uint64_t payloadOffset = 0;
  std::uint32_t length = 0;
};

struct StructureObjectRecord {
  std::uint32_t typeId = 0;
  std::uint32_t wireId = 0;
  bool valid = false;
};

struct StructureObjectBody {
  std::uint32_t wireId = 0;
  std::uint64_t bodyOffset = 0;
  std::uint32_t bodyLength = 0;
};

// CStructureSaver stores a byte tag followed by a 1- or 4-byte LE value:
// encoded = (payload length << 1) | (extended ? 1 : 0).
// `remaining` is the number of file bytes after the encoded length field.
bool S2_STRUCTURE_CALL DecodeStructureLength(const std::uint8_t* encoded,
                                              std::size_t encodedSize,
                                              std::uint64_t remaining,
                                              std::uint32_t* length);
// Decode one nested chunk in an already-loaded byte span. Returns false at
// the end or for a truncated/out-of-range chunk; offset is relative to bytes.
bool S2_STRUCTURE_CALL DecodeStructureChunkAt(const std::uint8_t* bytes,
                                              std::size_t size,
                                              std::size_t offset,
                                              StructureChunk* chunk);
bool S2_STRUCTURE_CALL ScanStructureFile(const std::string& path,
                                         std::vector<StructureChunk>* chunks,
                                         std::string* error = nullptr);
// Chunk 0 contains fixed nine-byte descriptors, independent of host pointer size.
bool S2_STRUCTURE_CALL DecodeStructureObjectTable(
    const std::uint8_t* bytes, std::size_t length,
    std::vector<StructureObjectRecord>* records);
// Chunk 2 contains one id-1 record per object, with a disk id-0 wire ID and
// an id-1 serialized body. Offsets refer to the chunk-2 payload.
bool S2_STRUCTURE_CALL IndexStructureObjectBodies(
    const std::uint8_t* bytes, std::size_t length,
    std::vector<StructureObjectBody>* bodies);

// Wide strings on disk are UTF-16LE code units, not host wchar_t arrays.
// Unpaired surrogates are preserved as code units for legacy round trips.
bool S2_STRUCTURE_CALL DecodeStructureUtf16(const std::uint8_t* bytes,
                                            std::size_t length,
                                            std::wstring* value);
bool S2_STRUCTURE_CALL EncodeStructureUtf16(const std::wstring& value,
                                            std::vector<std::uint8_t>* bytes);
// A scalar/blob field may only be copied when its disk size matches the
// destination's declared size; never read into the next sibling chunk.
bool S2_STRUCTURE_CALL CopyStructureField(const std::uint8_t* source,
                                          std::size_t sourceSize,
                                          void* destination,
                                          std::size_t destinationSize);

// Arithmetic and enum fields are little-endian on disk. Keep this conversion
// separate from CopyStructureField, which is also used for opaque blobs.
template<class T>
typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value, bool>::type
DecodeStructureScalar(const std::uint8_t* source, std::size_t length, T* value) {
  if (!source || !value || length != sizeof(T) || sizeof(T) > 8) return false;
  if (std::is_same<T, bool>::value) {
    *value = static_cast<T>(source[0] != 0);
    return true;
  }
  const std::uint16_t one = 1;
  const bool little = *reinterpret_cast<const std::uint8_t*>(&one) != 0;
  std::uint8_t host[sizeof(T)];
  for (std::size_t i = 0; i < sizeof(T); ++i)
    host[i] = source[little ? i : sizeof(T) - 1 - i];
  std::memcpy(value, host, sizeof(T));
  return true;
}

template<class T>
typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value, bool>::type
EncodeStructureScalar(const T& value, std::uint8_t* destination, std::size_t length) {
  if (!destination || length != sizeof(T) || sizeof(T) > 8) return false;
  if (std::is_same<T, bool>::value) {
    destination[0] = static_cast<bool>(value) ? 1 : 0;
    return true;
  }
  const std::uint16_t one = 1;
  const bool little = *reinterpret_cast<const std::uint8_t*>(&one) != 0;
  std::uint8_t host[sizeof(T)];
  std::memcpy(host, &value, sizeof(T));
  for (std::size_t i = 0; i < sizeof(T); ++i)
    destination[i] = host[little ? i : sizeof(T) - 1 - i];
  return true;
}

template<class T>
typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value, bool>::type
DecodeStructureScalarArray(const std::uint8_t* source, std::size_t length,
                           T* values, std::size_t count) {
  if (count > static_cast<std::size_t>(-1) / sizeof(T) ||
      length != count * sizeof(T)) return false;
  if (count == 0) return true;
  if (!source || !values) return false;
  for (std::size_t i = 0; i < count; ++i)
    if (!DecodeStructureScalar(source + i * sizeof(T), sizeof(T), values + i))
      return false;
  return true;
}

template<class T>
typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value, bool>::type
EncodeStructureScalarArray(const T* values, std::size_t count,
                           std::uint8_t* destination, std::size_t length) {
  if (count > static_cast<std::size_t>(-1) / sizeof(T) ||
      length != count * sizeof(T)) return false;
  if (count == 0) return true;
  if (!values || !destination) return false;
  for (std::size_t i = 0; i < count; ++i)
    if (!EncodeStructureScalar(values[i], destination + i * sizeof(T), sizeof(T)))
      return false;
  return true;
}

// Mission action cache: historical tag payload is 20 bytes with three padding
// bytes at offsets 1..3. This value model has no host-layout requirements.
struct StructureActionInfoFields {
  bool valid = false;
  std::int32_t minAP = 0;
  std::int32_t maxAP = 0;
  bool ok = false;
  bool enoughAP = false;
  bool enoughAPToStart = false;
  bool available = false;
  std::int32_t result = 0;
};

inline bool DecodeStructureActionInfo(const std::uint8_t* source,
                                      std::size_t length,
                                      StructureActionInfoFields* value) {
  if (!source || !value || length != 20) return false;
  StructureActionInfoFields decoded;
  if (!DecodeStructureScalar(source, 1, &decoded.valid) ||
      !DecodeStructureScalar(source + 4, 4, &decoded.minAP) ||
      !DecodeStructureScalar(source + 8, 4, &decoded.maxAP) ||
      !DecodeStructureScalar(source + 12, 1, &decoded.ok) ||
      !DecodeStructureScalar(source + 13, 1, &decoded.enoughAP) ||
      !DecodeStructureScalar(source + 14, 1, &decoded.enoughAPToStart) ||
      !DecodeStructureScalar(source + 15, 1, &decoded.available) ||
      !DecodeStructureScalar(source + 16, 4, &decoded.result)) return false;
  *value = decoded;
  return true;
}

inline bool EncodeStructureActionInfo(const StructureActionInfoFields& value,
                                      std::uint8_t* destination,
                                      std::size_t length) {
  if (!destination || length != 20) return false;
  std::memset(destination, 0, length);
  return EncodeStructureScalar(value.valid, destination, 1) &&
         EncodeStructureScalar(value.minAP, destination + 4, 4) &&
         EncodeStructureScalar(value.maxAP, destination + 8, 4) &&
         EncodeStructureScalar(value.ok, destination + 12, 1) &&
         EncodeStructureScalar(value.enoughAP, destination + 13, 1) &&
         EncodeStructureScalar(value.enoughAPToStart, destination + 14, 1) &&
         EncodeStructureScalar(value.available, destination + 15, 1) &&
         EncodeStructureScalar(value.result, destination + 16, 4);
}

// An explosion wavefront cell is three little-endian 16-bit coordinates,
// independent of any padding in the game's host struct.
struct StructureVoxelCoords {
  std::uint16_t x = 0;
  std::uint16_t y = 0;
  std::uint16_t z = 0;
};

inline bool DecodeStructureVoxelCoords(const std::uint8_t* source,
                                       std::size_t length,
                                       StructureVoxelCoords* value) {
  if (!source || !value || length != 6) return false;
  StructureVoxelCoords decoded;
  if (!DecodeStructureScalar(source, 2, &decoded.x) ||
      !DecodeStructureScalar(source + 2, 2, &decoded.y) ||
      !DecodeStructureScalar(source + 4, 2, &decoded.z)) return false;
  *value = decoded;
  return true;
}

inline bool EncodeStructureVoxelCoords(const StructureVoxelCoords& value,
                                       std::uint8_t* destination,
                                       std::size_t length) {
  if (!destination || length != 6) return false;
  return EncodeStructureScalar(value.x, destination, 2) &&
         EncodeStructureScalar(value.y, destination + 2, 2) &&
         EncodeStructureScalar(value.z, destination + 4, 2);
}

// Camera records are flat float fields on the v1.2 wire. The limits record
// has a one-byte bool and three historical padding bytes between float groups.
inline bool DecodeStructureCameraPos(const std::uint8_t* source,
                                     std::size_t length, float (&fields)[8]) {
  if (!source || length != 32) return false;
  float decoded[8];
  for (std::size_t i = 0; i < 8; ++i)
    if (!DecodeStructureScalar(source + 4 * i, 4, &decoded[i])) return false;
  for (std::size_t i = 0; i < 8; ++i) fields[i] = decoded[i];
  return true;
}

inline bool EncodeStructureCameraPos(const float (&fields)[8],
                                     std::uint8_t* destination,
                                     std::size_t length) {
  if (!destination || length != 32) return false;
  for (std::size_t i = 0; i < 8; ++i)
    if (!EncodeStructureScalar(fields[i], destination + 4 * i, 4)) return false;
  return true;
}

inline bool DecodeStructureCameraLimits(const std::uint8_t* source,
                                        std::size_t length, float (&fields)[13],
                                        bool* movie) {
  if (!source || !movie || length != 56) return false;
  float decoded[13];
  for (std::size_t i = 0; i < 13; ++i) {
    const std::size_t offset = i < 4 ? 4 * i : 4 * i + 4;
    if (!DecodeStructureScalar(source + offset, 4, &decoded[i])) return false;
  }
  for (std::size_t i = 0; i < 13; ++i) fields[i] = decoded[i];
  *movie = source[16] != 0;
  return true;
}

inline bool EncodeStructureCameraLimits(const float (&fields)[13], bool movie,
                                        std::uint8_t* destination,
                                        std::size_t length) {
  if (!destination || length != 56) return false;
  for (std::size_t i = 0; i < 13; ++i) {
    const std::size_t offset = i < 4 ? 4 * i : 4 * i + 4;
    if (!EncodeStructureScalar(fields[i], destination + offset, 4)) return false;
  }
  destination[16] = movie ? 1 : 0;
  destination[17] = destination[18] = destination[19] = 0;
  return true;
}

// Specialize this for game PODs whose historical wire format must not depend
// on host padding or alignment. Unspecialized PODs retain the legacy raw path.
template<class T, class Enable = void>
struct StructureFieldCodec {
  static constexpr bool kPortable = false;
  static constexpr std::size_t kWireSize = sizeof(T);
};

template<class T>
struct StructureFieldCodec<T, typename std::enable_if<
    std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = sizeof(T);
  static bool Decode(const std::uint8_t* source, std::size_t length, T* value) {
    return DecodeStructureScalar(source, length, value);
  }
  static bool Encode(const T& value, std::uint8_t* destination, std::size_t length) {
    return EncodeStructureScalar(value, destination, length);
  }
};


} // namespace S2FileIO
