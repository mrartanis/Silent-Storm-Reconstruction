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
#include "../DBFormat/DataScenario.h"
#include "../Main/BuildingInfo.h"
#include "../Main/BuildingGrid.h"
#include "../Main/MakeBuilding.h"
#include "../Main/MakeBuildingInternal.h"
#include "../Main/MapBuildTerrain.h"
#include "../Main/METerrain.h"
#include "../Main/TerrainInfo.h"
#include "../Misc/BasicShare.h"
#include "../Misc/RandomGen.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <set>
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

static void AddTo( std::uint64_t *hash, std::uint32_t value )
{
	for ( int byte = 0; byte < 4; ++byte )
	{
		*hash ^= static_cast<std::uint8_t>( value >> ( byte * 8 ) );
		*hash *= UINT64_C(1099511628211);
	}
}

static std::set<int> ScenarioVariantIDs()
{
	std::set<int> templatesSeen, variantsSeen;
	auto *zones = NDatabase::GetTable<NDb::CDBScenarioZone>();
	auto *templates = NDatabase::GetTable<NDb::CTemplate>();
	if ( !zones || !templates ) throw 1;
	std::vector<int> pending;
	CDBIterator<NDb::CDBScenarioZone> zone( *zones );
	while ( zone.MoveNext() )
		for ( int id : zone.Get()->templatesIDs )
			if ( id > 0 ) pending.push_back( id );
	while ( !pending.empty() )
	{
		const int id = pending.back();
		pending.pop_back();
		if ( !templatesSeen.insert( id ).second ) continue;
		auto *map = static_cast<NDb::CTemplate*>( templates->GetDBRecord( id ) );
		if ( !map ) throw 2;
		for ( const auto &variant : map->variants )
		{
			if ( !variant ) throw 3;
			variantsSeen.insert( variant->GetRecordID() );
			for ( const auto &rect : variant->rects )
				if ( rect && rect->pTemplate )
					pending.push_back( rect->pTemplate->GetRecordID() );
		}
	}
	return variantsSeen;
}

static bool CheckScenarioTypedResources()
{
	const auto ids = ScenarioVariantIDs();
	if ( ids.size() != 985 ) return false;
	std::uint64_t buildingDigest = UINT64_C(14695981039346656037);
	std::uint64_t terrainDigest = UINT64_C(14695981039346656037);
	std::size_t buildings = 0, terrains = 0, failed = 0;
	for ( int id : ids )
	{
		if ( NGScene::CResourceFileOpener::DoesExist( "Buildings", id ) )
		{
			try
			{
				NGScene::CResourceOpener file( "Buildings", id );
				CObj<NBuilding::CBuildInfo> value = new NBuilding::CBuildInfo;
				value->operator&( *file.operator->() );
				if ( value->nMaxX < 0 || value->nMaxY < 0 ||
					value->nMinFloor > value->nMaxFloor ) return false;
				AddTo( &buildingDigest, id );
				AddTo( &buildingDigest, value->nMaxX ); AddTo( &buildingDigest, value->nMaxY );
				AddTo( &buildingDigest, value->nMinFloor ); AddTo( &buildingDigest, value->nMaxFloor );
				AddTo( &buildingDigest, static_cast<std::uint32_t>(value->wallFragments.size()) );
				AddTo( &buildingDigest, static_cast<std::uint32_t>(value->solidFragments.size()) );
				AddTo( &buildingDigest, static_cast<std::uint32_t>(value->spots.size()) );
				AddTo( &buildingDigest, static_cast<std::uint32_t>(value->ladders.size()) );
				AddTo( &buildingDigest, value->cellar.GetXSize() );
				AddTo( &buildingDigest, value->cellar.GetYSize() );
				++buildings;
			}
			catch ( ... ) { std::fprintf( stderr, "scenario building decode failed id=%d\n", id ); ++failed; }
		}
		if ( NGScene::CResourceFileOpener::DoesExist( "Terrain", id ) )
		{
			try
			{
				NGScene::CResourceOpener file( "Terrain", id );
				CObj<CMETerrainInfo> value = new CMETerrainInfo;
				value->operator&( *file.operator->() );
				if ( value->info.nWidth < 0 || value->info.nHeight < 0 ) return false;
				AddTo( &terrainDigest, id );
				AddTo( &terrainDigest, value->info.nWidth ); AddTo( &terrainDigest, value->info.nHeight );
				AddTo( &terrainDigest, value->info.heightMap.GetXSize() );
				AddTo( &terrainDigest, value->info.heightMap.GetYSize() );
				AddTo( &terrainDigest, value->info.typeMap.GetXSize() );
				AddTo( &terrainDigest, value->info.typeMap.GetYSize() );
				AddTo( &terrainDigest, value->alphaMap.GetXSize() );
				AddTo( &terrainDigest, value->alphaMap.GetYSize() );
				++terrains;
			}
			catch ( ... ) { std::fprintf( stderr, "scenario terrain decode failed id=%d\n", id ); ++failed; }
		}
	}
	std::printf( "scenario_typed variants=%zu buildings=%zu terrain=%zu failed=%zu building_digest=%016llX terrain_digest=%016llX\n",
		ids.size(), buildings, terrains, failed,
		static_cast<unsigned long long>(buildingDigest), static_cast<unsigned long long>(terrainDigest) );
	return buildings == 903 && terrains == 371 && failed == 0 &&
		buildingDigest == UINT64_C(0xA924C3A98CD24D04) &&
		terrainDigest == UINT64_C(0xA29501ED83DC7587);
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
		S2FileIO::PortablePackageIndex buildings, terrainIndex, aiGeometryIndex;
		const std::string resDir = argv[2];
		if ( !buildings.Open( resDir + "/Buildings.res" ) ||
			!terrainIndex.Open( resDir + "/Terrain.res" ) ||
			!aiGeometryIndex.Open( resDir + "/AIGeometries.res" ) )
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
		int checked = 0, attempted = 0, empty = 0, positiveHp = 0;
		std::vector<int> checkedIDs;
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
			CObj<NBuilding::CSolidAndWallMap> wallMap =
				NBuilding::MakeSWMap( id, SRandomSeed( 123 ) );
			CDGPtr<NBuilding::CSolidAndWallMap> wallPin( wallMap );
			wallPin.Refresh();
			const auto &wallGrid = wallMap->GetWallGrid();
			if ( wallGrid.GetWidth() != building->nMaxX + 2 ||
				wallGrid.GetHeight() != building->nMaxY + 2 ||
				wallGrid.GetMinFloor() != building->nMinFloor ||
				wallGrid.GetMaxFloor() != building->nMaxFloor )
				return 11;
			Add( wallGrid.GetWidth() ); Add( wallGrid.GetHeight() );
			Add( static_cast<std::uint32_t>( wallMap->GetSolidMap().size() ) );
			CObj<NBuilding::CBuildingGrid> hpGrid = new NBuilding::CBuildingGrid;
			hpGrid->Setup( building->nMaxX, building->nMaxY,
				building->nMinFloor, building->nMaxFloor, CVec2( 0, 0 ) );
			NBuilding::BuildingHP( building, hpGrid, wallMap );
			int variantPositiveHp = 0;
			for ( int z = building->nMinFloor * 4;
				z <= building->nMaxFloor * 4 + 4; ++z )
				for ( int y = 0; y < 2 + building->nMaxY * 2; ++y )
					for ( int x = 0; x < 2 + building->nMaxX * 2; ++x )
						variantPositiveHp += hpGrid->GetHP(
							NBuilding::SPoint3( x, y, z ) ) > 0;
			positiveHp += variantPositiveHp;
			Add( variantPositiveHp );
			checkedIDs.push_back( id );
			++checked;
			if ( checked == 8 ) break;
		}
		if ( checked != 8 || positiveHp == 0 )
			return 8;
		std::printf( "matched_variants=%zu attempted=%d empty=%d checked=%d positive_hp=%d digest=%016llX\n",
			ids.size(), attempted, empty, checked, positiveHp,
			static_cast<unsigned long long>(digest) );
		std::printf( "checked_ids=" );
		for ( std::size_t i = 0; i < checkedIDs.size(); ++i )
			std::printf( "%s%d", i ? "," : "", checkedIDs[i] );
		std::printf( "\n" );
		if ( !CheckScenarioTypedResources() ) return 12;
		NGScene::CloseAllResources();
		return 0;
	}
	catch ( ... )
	{
		std::fputs( "building/terrain resource probe threw\n", stderr );
		return 9;
	}
}
