#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../DBFormat/DataMap.h"
#include "../Main/BuildingInfo.h"
#include "../Main/MapBuildTerrain.h"
#include "../Main/METerrain.h"
#include "../Main/TerrainInfo.h"
#include "../Misc/BasicShare.h"
#include "../Misc/RandomGen.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace NGScene {
extern CBasicShare<int, NBuilding::CBuildInfoLoader> shareBuildings;
}
extern CBasicShare<int, CMETerrainLoader> shareTerrains;

static std::uint64_t digest = UINT64_C(14695981039346656037);
static void Add( std::uint32_t value )
{
	for ( int byte = 0; byte < 4; ++byte )
	{
		digest ^= static_cast<std::uint8_t>( value >> ( byte * 8 ) );
		digest *= UINT64_C(1099511628211);
	}
}

int main( int argc, char **argv )
{
	if ( argc != 3 )
		return 2;
	try
	{
		CFileStream database;
		database.OpenRead( argv[1] );
		NDatabase::Serialize( database, CStructureSaver::READ );
		S2FileIO::PortablePackageIndex buildings, terrainIndex;
		const std::string resDir = argv[2];
		if ( !buildings.Open( resDir + "/Buildings.res" ) ||
			!terrainIndex.Open( resDir + "/Terrain.res" ) )
			return 3;
		NGScene::AddResourceDir( resDir.c_str() );
		auto *variants = NDatabase::GetTable<NDb::CTemplVariant>();
		if ( !variants )
			return 4;
		std::vector<int> ids;
		CDBIterator<NDb::CTemplVariant> it( *variants );
		while ( it.MoveNext() )
		{
			const int id = it.Get()->GetRecordID();
			if ( buildings.Entries().count( id ) && terrainIndex.Entries().count( id ) )
				ids.push_back( id );
		}
		std::sort( ids.begin(), ids.end() );
		int checked = 0, attempted = 0, empty = 0;
		for ( int id : ids )
		{
			++attempted;
			CDGPtr<CPtrFuncBase<NBuilding::CBuildInfo>> loader =
				NGScene::shareBuildings.Get( id );
			loader.Refresh();
			NBuilding::CBuildInfo *building = loader->GetValue();
			if ( !IsValid( building ) || building->nMaxX < 0 || building->nMaxY < 0 ||
				building->nMinFloor > building->nMaxFloor )
				return 5;
			CDGPtr<CPtrFuncBase<CMETerrainInfo>> terrainLoader = shareTerrains.Get( id );
			terrainLoader.Refresh();
			CMETerrainInfo *rawTerrain = terrainLoader->GetValue();
			if ( !IsValid( rawTerrain ) )
				return 10;
			if ( building->nMaxX == 0 || building->nMaxY == 0 ||
				( building->wallFragments.empty() && building->solidFragments.empty() ) ||
				rawTerrain->info.nWidth <= 0 || rawTerrain->info.nHeight <= 0 )
			{
				++empty;
				continue;
			}
			SRand random( SRandomSeed( 123 ) );
			STerrainInfo terrain;
			if ( !LoadRootTerrain( id, &terrain, &random, std::vector<int>() ) ||
				terrain.nWidth < 8 || terrain.nHeight < 8 ||
				terrain.nWidth % 8 || terrain.nHeight % 8 ||
				terrain.heightMap.GetXSize() != terrain.nWidth + 1 ||
				terrain.heightMap.GetYSize() != terrain.nHeight + 1 )
				return 6;
			MakeSoundMap( &terrain );
			CalcAverageColor( &terrain );
			if ( terrain.soundmap.GetXSize() != terrain.typeMap.GetXSize() ||
				terrain.avrgColor.GetYSize() != terrain.typeMap.GetYSize() )
				return 7;
			Add( id );
			Add( building->nMaxX ); Add( building->nMaxY );
			Add( building->nMinFloor ); Add( building->nMaxFloor );
			Add( static_cast<std::uint32_t>( building->wallFragments.size() ) );
			Add( static_cast<std::uint32_t>( building->solidFragments.size() ) );
			Add( terrain.nWidth ); Add( terrain.nHeight );
			Add( terrain.heightMap[0][0] );
			Add( terrain.typeMap[0][0] );
			Add( terrain.avrgColor[0][0] );
			++checked;
			if ( checked == 8 ) break;
		}
		if ( checked != 8 )
			return 8;
		std::printf( "matched_variants=%zu attempted=%d empty=%d checked=%d digest=%016llX\n",
			ids.size(), attempted, empty, checked,
			static_cast<unsigned long long>(digest) );
		NGScene::CloseAllResources();
		return 0;
	}
	catch ( ... )
	{
		std::fputs( "building/terrain resource probe threw\n", stderr );
		return 9;
	}
}
