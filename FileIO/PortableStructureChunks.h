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


} // namespace S2FileIO
