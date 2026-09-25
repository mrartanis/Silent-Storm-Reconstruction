#ifndef __MapBuildingInfo_H_
#define __MapBuildingInfo_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../DBFormat/DataMap.h"
#include "DiscretePos.h"   // NWorld::CFBTransform (retail SMapBuilding stores the placement as CObj<CFBTransform>)
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NBuilding
{
	class CBuildingGrid;
	class CSolidAndWallMap;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SMapPosition
{
	CVec3 ptPos;
	CVec3 ptScale;
	float fRotation; // � ��������
	int nFloor;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SMapBuilding
{
	struct SStorey
	{
		int nFloor; // relative to building
		int nRealFloor; // global floor
		
		SStorey() {}
		SStorey( int _nFloor, int _nRealFloor )
			: nFloor(_nFloor), nRealFloor(_nRealFloor) {}
	};
	struct SAmbientLight
	{
		CVec3 color;
		int nFloor;
		bool bLightmap;
	};

	CDBPtr<NDb::CTemplVariant> pVariant;
	CObj<CFBTransform> pPos;
	CObj<NBuilding::CBuildingGrid> pGrid;
	CObj<NBuilding::CSolidAndWallMap> pSWMap;
	vector<SStorey> stories;
	vector<SAmbientLight> lights;
	CVec2 ptAlignTo;

	SMapPosition mpos; // ���. � ���������, � ����� ��� ����������� ���������� ������� �������������

	const SStorey& GetStorey( int nLocalFloor ) const
	{
		for ( vector<SStorey>::const_iterator i = stories.begin(); i != stories.end(); ++i )
		{
			if ( i->nFloor == nLocalFloor )
				return *i;
		}
#ifndef _MAPEDIT
		ASSERT( 0 );
#endif
		static SStorey fake;
		return fake;
	}

	int operator&( CStructureSaver &f )
	{
		// Retail v1.2 @0x7462a0 stores the placement node itself. Its contents may
		// be read AFTER this object, so do not copy the matrix during deserialization.
		f.Add( 2, &pVariant );
		f.Add( 3, &pPos );
		f.Add( 4, &pGrid );
		f.Add( 5, &pSWMap );
		f.Add( 6, &stories );
		f.Add( 7, &ptAlignTo );
		return 0;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../FileIO/PortableMapStoreyWire.h"

static_assert(sizeof(SMapBuilding::SStorey) == 8, "map storey wire size");
namespace S2FileIO {
template<>
struct StructureFieldCodec<SMapBuilding::SStorey, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 8;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     SMapBuilding::SStorey* value) {
    if (!value) return false;
    MapStoreyFields fields{};
    if (!DecodeMapStorey(source, length, &fields)) return false;
    value->nFloor = fields.localFloor;
    value->nRealFloor = fields.globalFloor;
    return true;
  }
  static bool Encode(const SMapBuilding::SStorey& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeMapStorey({value.nFloor, value.nRealFloor}, destination, length);
  }
};
} // namespace S2FileIO
#endif
