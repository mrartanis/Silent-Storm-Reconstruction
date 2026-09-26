#ifndef __BUILDING_PART_H
#define __BUILDING_PART_H
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../Misc/PortableBuildingPart.h"
#include "../FileIO/PortableStructureChunks.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NBuilding
{
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SPart
{
	std::uint32_t nID;
	SPart() : nID(0) {}
	SPart( int nFloor, int nX, int nY ): nID(S2Building::MakePart(nFloor, nX, nY)) {}
	int GetFloor() const { return S2Building::PartFloor(nID); }
	int GetX() const { return S2Building::PartX(nID); }
	int GetY() const { return S2Building::PartY(nID); }

	bool operator==( const SPart &op ) const { return nID == op.nID; }
	int operator() ( const SPart &op ) const { return op.nID; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}
static_assert(sizeof(NBuilding::SPart) == 4, "building part is one wire word");
namespace S2FileIO {
template<>
struct StructureFieldCodec<NBuilding::SPart, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 4;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NBuilding::SPart* value) {
    return value && S2Building::DecodePart(source, length, &value->nID);
  }
  static bool Encode(const NBuilding::SPart& value,
                     std::uint8_t* destination, std::size_t length) {
    return S2Building::EncodePart(value.nID, destination, length);
  }
};
} // namespace S2FileIO
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __BUILDING_PART_H
