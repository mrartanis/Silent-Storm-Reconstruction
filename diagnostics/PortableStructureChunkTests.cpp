#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <cstring>
#include <vector>

int main() {
  enum class TestMode : std::uint32_t { Active = 0x12345678 };
  struct OpaqueField { std::uint32_t value; std::uint8_t flag; };
  if (!S2FileIO::StructureFieldCodec<std::uint32_t>::kPortable ||
      S2FileIO::StructureFieldCodec<OpaqueField>::kPortable ||
      S2FileIO::StructureFieldCodec<OpaqueField>::kWireSize != sizeof(OpaqueField))
    return 1;
  const std::uint8_t scalar32[] = {0x78, 0x56, 0x34, 0x12};
  std::uint32_t unsignedValue = 0;
  const std::uint8_t signedWire[] = {0xfe, 0xff, 0xff, 0xff};
  std::int32_t signedValue = 0;
  TestMode mode = static_cast<TestMode>(0);
  std::uint8_t encoded32[4] = {};
  const std::uint8_t floatWire[] = {0, 0, 0x80, 0x3f};
  float floatValue = 0;
  bool boolValue = false;
  const std::uint8_t legacyTrue[] = {2};
  std::uint8_t boolWire = 0;
  const std::uint8_t arrayWire[] = {0x78, 0x56, 0x34, 0x12,
                                    0xfe, 0xff, 0xff, 0xff};
  std::int32_t arrayValues[2] = {};
  std::uint8_t arrayEncoded[sizeof(arrayWire)] = {};
  const std::uint8_t boolArrayWire[] = {0, 2};
  bool boolArray[2] = {};
  std::uint8_t boolArrayEncoded[2] = {};
  const std::uint8_t actionWire[] = {
      2, 0xaa, 0xbb, 0xcc, 0xfe, 0xff, 0xff, 0xff,
      0x34, 0x12, 0, 0, 1, 2, 0, 3, 0x78, 0x56, 0x34, 0x12};
  S2FileIO::StructureActionInfoFields action;
  std::uint8_t actionEncoded[sizeof(actionWire)] = {};
  const std::uint8_t voxelWire[] = {0x34, 0x12, 0x78, 0x56, 0xbc, 0x9a};
  S2FileIO::StructureVoxelCoords voxel;
  std::uint8_t voxelEncoded[sizeof(voxelWire)] = {};
  std::uint16_t voxelObject = 0;
  std::uint8_t voxelObjectEncoded[2] = {};
  if (!S2FileIO::DecodeStructureScalar(scalar32, sizeof(scalar32), &unsignedValue) ||
      unsignedValue != 0x12345678 ||
      !S2FileIO::EncodeStructureScalar(unsignedValue, encoded32, sizeof(encoded32)) ||
      std::memcmp(encoded32, scalar32, sizeof(scalar32)) != 0 ||
      !S2FileIO::DecodeStructureScalar(signedWire, sizeof(signedWire), &signedValue) ||
      signedValue != -2 ||
      !S2FileIO::EncodeStructureScalar(signedValue, encoded32, sizeof(encoded32)) ||
      std::memcmp(encoded32, signedWire, sizeof(signedWire)) != 0 ||
      !S2FileIO::DecodeStructureScalar(scalar32, sizeof(scalar32), &mode) ||
      mode != TestMode::Active ||
      !S2FileIO::EncodeStructureScalar(mode, encoded32, sizeof(encoded32)) ||
      std::memcmp(encoded32, scalar32, sizeof(scalar32)) != 0 ||
      !S2FileIO::DecodeStructureScalar(floatWire, sizeof(floatWire), &floatValue) ||
      floatValue != 1.0f ||
      !S2FileIO::DecodeStructureScalar(legacyTrue, 1, &boolValue) || !boolValue ||
      !S2FileIO::EncodeStructureScalar(boolValue, &boolWire, 1) || boolWire != 1 ||
      !S2FileIO::DecodeStructureScalarArray(arrayWire, sizeof(arrayWire), arrayValues, 2) ||
      arrayValues[0] != 0x12345678 || arrayValues[1] != -2 ||
      !S2FileIO::EncodeStructureScalarArray(arrayValues, 2, arrayEncoded, sizeof(arrayEncoded)) ||
      std::memcmp(arrayEncoded, arrayWire, sizeof(arrayWire)) != 0 ||
      !S2FileIO::DecodeStructureScalarArray(boolArrayWire, 2, boolArray, 2) ||
      boolArray[0] || !boolArray[1] ||
      !S2FileIO::EncodeStructureScalarArray(boolArray, 2, boolArrayEncoded, 2) ||
      boolArrayEncoded[0] != 0 || boolArrayEncoded[1] != 1 ||
      S2FileIO::DecodeStructureScalarArray(arrayWire, 7, arrayValues, 2) ||
      S2FileIO::EncodeStructureScalarArray(arrayValues, 2, arrayEncoded, 7) ||
      S2FileIO::DecodeStructureScalarArray(nullptr, 8, arrayValues, 2) ||
      !S2FileIO::DecodeStructureScalarArray<std::int32_t>(nullptr, 0, nullptr, 0) ||
      !S2FileIO::DecodeStructureActionInfo(actionWire, sizeof(actionWire), &action) ||
      !action.valid || action.minAP != -2 || action.maxAP != 0x1234 ||
      !action.ok || !action.enoughAP || action.enoughAPToStart ||
      !action.available || action.result != 0x12345678 ||
      !S2FileIO::EncodeStructureActionInfo(action, actionEncoded, sizeof(actionEncoded)) ||
      actionEncoded[0] != 1 || actionEncoded[1] != 0 || actionEncoded[2] != 0 ||
      actionEncoded[3] != 0 || actionEncoded[13] != 1 || actionEncoded[15] != 1 ||
      std::memcmp(actionEncoded + 4, actionWire + 4, 9) != 0 ||
      std::memcmp(actionEncoded + 16, actionWire + 16, 4) != 0 ||
      S2FileIO::DecodeStructureActionInfo(actionWire, 19, &action) ||
      S2FileIO::EncodeStructureActionInfo(action, actionEncoded, 19) ||
      !S2FileIO::DecodeStructureVoxelCoords(voxelWire, sizeof(voxelWire), &voxel) ||
      voxel.x != 0x1234 || voxel.y != 0x5678 || voxel.z != 0x9abc ||
      !S2FileIO::EncodeStructureVoxelCoords(voxel, voxelEncoded, sizeof(voxelEncoded)) ||
      std::memcmp(voxelWire, voxelEncoded, sizeof(voxelWire)) != 0 ||
      !S2FileIO::DecodeStructureScalar(voxelWire, 2, &voxelObject) ||
      voxelObject != 0x1234 ||
      !S2FileIO::EncodeStructureScalar(voxelObject, voxelObjectEncoded, 2) ||
      std::memcmp(voxelWire, voxelObjectEncoded, 2) != 0 ||
      S2FileIO::DecodeStructureVoxelCoords(voxelWire, 5, &voxel) ||
      S2FileIO::EncodeStructureVoxelCoords(voxel, voxelEncoded, 5) ||
      S2FileIO::DecodeStructureScalar(scalar32, 3, &unsignedValue) ||
      S2FileIO::EncodeStructureScalar(unsignedValue, encoded32, 3) ||
      S2FileIO::DecodeStructureScalar(nullptr, 4, &unsignedValue)) return 1;
  std::uint32_t length = 0;
  const std::uint8_t shortLength[] = {8};
  const std::uint8_t longLength[] = {0xf9, 0x0e, 0x00, 0x00};
  const std::uint8_t objectTable[] = {
      0x30, 0x31, 0x84, 0xa1, 0x01, 0x00, 0x00, 0x00, 0x01,
      0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff, 0x00};
  std::vector<S2FileIO::StructureObjectRecord> records;
  const std::uint8_t nested[] = {7, 8, 1, 2, 3, 4, 9, 0xf9, 0x0e, 0, 0};
  S2FileIO::StructureChunk chunk;
  const std::uint8_t utf16[] = {
      0x41, 0x00, 0x16, 0x04, 0x3d, 0xd8, 0x00, 0xde,
      0x00, 0xd8, 0x00, 0x00};
  std::wstring wide;
  std::vector<std::uint8_t> encoded;
  if (!S2FileIO::DecodeStructureUtf16(utf16, sizeof(utf16), &wide) ||
      wide.size() != (sizeof(wchar_t) == 2 ? 6u : 5u) ||
      wide[0] != L'A' || wide[1] != 0x416 ||
      wide[2] != (sizeof(wchar_t) == 2 ? 0xd83d : 0x1f600) ||
      !S2FileIO::EncodeStructureUtf16(wide, &encoded) ||
      encoded != std::vector<std::uint8_t>(utf16, utf16 + sizeof(utf16)) ||
      S2FileIO::DecodeStructureUtf16(utf16, sizeof(utf16) - 1, &wide) ||
      !S2FileIO::DecodeStructureUtf16(nullptr, 0, &wide) || !wide.empty())
    return 1;
  if (sizeof(wchar_t) == 4) {
    wide.push_back(static_cast<wchar_t>(0x110000));
    if (S2FileIO::EncodeStructureUtf16(wide, &encoded)) return 1;
  }
  const std::uint8_t field[] = {0x12, 0x34, 0x56, 0x78};
  std::uint8_t copied[] = {0, 0, 0, 0};
  if (!S2FileIO::CopyStructureField(field, sizeof(field), copied, sizeof(copied)) ||
      copied[0] != 0x12 || copied[3] != 0x78 ||
      S2FileIO::CopyStructureField(field, 3, copied, sizeof(copied)) ||
      S2FileIO::CopyStructureField(field, sizeof(field), copied, 3) ||
      S2FileIO::CopyStructureField(nullptr, sizeof(field), copied, sizeof(copied)) ||
      !S2FileIO::CopyStructureField(nullptr, 0, nullptr, 0))
    return 1;
  const std::uint8_t objectBody[] = {
      1, 20, 0, 8, 1, 0, 0, 0, 1, 4, 0xaa, 0xbb};
  std::vector<S2FileIO::StructureObjectBody> bodies;
  if (!S2FileIO::IndexStructureObjectBodies(objectBody, sizeof(objectBody), &bodies) ||
      bodies.size() != 1 || bodies[0].wireId != 1 ||
      bodies[0].bodyOffset != 10 || bodies[0].bodyLength != 2 ||
      S2FileIO::IndexStructureObjectBodies(objectBody, sizeof(objectBody) - 1, &bodies) ||
      !bodies.empty())
    return 1;
  return S2FileIO::DecodeStructureLength(shortLength, 1, 4, &length) && length == 4 &&
         S2FileIO::DecodeStructureLength(longLength, 4, 1916, &length) && length == 1916 &&
         !S2FileIO::DecodeStructureLength(shortLength, 1, 3, &length) &&
         !S2FileIO::DecodeStructureLength(longLength, 1, 1916, &length) &&
         !S2FileIO::DecodeStructureLength(nullptr, 0, 0, &length) &&
         S2FileIO::DecodeStructureChunkAt(nested, sizeof(nested), 0, &chunk) &&
         chunk.id == 7 && chunk.payloadOffset == 2 && chunk.length == 4 &&
         !S2FileIO::DecodeStructureChunkAt(nested, sizeof(nested), 6, &chunk) &&
         !S2FileIO::DecodeStructureChunkAt(nested, sizeof(nested), sizeof(nested), &chunk) &&
         !S2FileIO::DecodeStructureChunkAt(nested, 1, 0, &chunk) &&
         S2FileIO::DecodeStructureObjectTable(objectTable, sizeof(objectTable), &records) &&
         records.size() == 2 && records[0].typeId == 0xa1843130 &&
         records[0].wireId == 1 && records[0].valid &&
         records[1].typeId == 0x12345678 && records[1].wireId == 0xffffffff &&
         !records[1].valid &&
         !S2FileIO::DecodeStructureObjectTable(objectTable, sizeof(objectTable) - 1, &records) &&
         records.empty() &&
         !S2FileIO::DecodeStructureObjectTable(nullptr, 9, &records)
             ? 0 : 1;
}
