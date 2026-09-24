#include "../Main/StdAfx.h"
#include "../Main/wExplTracker.h"

#include <cstdint>
#include <cstring>

int main() {
  using CoordsCodec = S2FileIO::StructureFieldCodec<NWorld::SExplVoxelCoords>;
  using ObjectCodec = S2FileIO::StructureFieldCodec<NAI::SExplVoxel>;
  if (!CoordsCodec::kPortable || CoordsCodec::kWireSize != 6 ||
      !ObjectCodec::kPortable || ObjectCodec::kWireSize != 2) return 1;

  const std::uint8_t expectedCoords[] = {0x34, 0x12, 0x78, 0x56, 0xbc, 0x9a};
  NWorld::SExplVoxelCoords coords(0x1234, 0x5678, 0x9abc);
  std::uint8_t coordsWire[6] = {};
  NWorld::SExplVoxelCoords decodedCoords;
  if (!CoordsCodec::Encode(coords, coordsWire, sizeof(coordsWire)) ||
      std::memcmp(coordsWire, expectedCoords, sizeof(coordsWire)) != 0 ||
      !CoordsCodec::Decode(expectedCoords, sizeof(expectedCoords), &decodedCoords) ||
      decodedCoords.nX != coords.nX || decodedCoords.nY != coords.nY ||
      decodedCoords.nZ != coords.nZ ||
      CoordsCodec::Decode(expectedCoords, 5, &decodedCoords)) return 1;

  const std::uint8_t expectedObject[] = {0x34, 0x12};
  NAI::SExplVoxel object(0x1234);
  std::uint8_t objectWire[2] = {};
  NAI::SExplVoxel decodedObject;
  if (!ObjectCodec::Encode(object, objectWire, sizeof(objectWire)) ||
      std::memcmp(objectWire, expectedObject, sizeof(objectWire)) != 0 ||
      !ObjectCodec::Decode(expectedObject, sizeof(expectedObject), &decodedObject) ||
      decodedObject.nObject != object.nObject ||
      ObjectCodec::Decode(expectedObject, 1, &decodedObject)) return 1;
  return 0;
}
