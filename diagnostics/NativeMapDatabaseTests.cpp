#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../DBFormat/DataMap.h"
#include "../DBFormat/DataScenario.h"
#include "../DBFormat/DataRPG.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <vector>

namespace {
std::uint64_t digest = 14695981039346656037ULL;

void Add( std::uint32_t value )
{
	for ( int byte = 0; byte < 4; ++byte )
	{
		digest ^= static_cast<unsigned char>( value >> ( byte * 8 ) );
		digest *= 1099511628211ULL;
	}
}

template<class T> int ID( const CPtr<T> &pointer )
{
	return pointer.GetPtr() ? pointer->GetRecordID() : 0;
}

template<class T> void AddRecords( const std::vector<CPtr<T> > &records )
{
	std::vector<int> ids;
	for ( const auto &record : records )
	{
		if ( !record.GetPtr() )
			throw 1;
		ids.push_back( ID( record ) );
	}
	std::sort( ids.begin(), ids.end() );
	Add( static_cast<std::uint32_t>( ids.size() ) );
	for ( int id : ids )
		Add( static_cast<std::uint32_t>( id ) );
}
}

int main( int argc, char **argv )
{
	const bool printRoots = argc == 3 && std::strcmp( argv[2], "--roots" ) == 0;
	const bool printCampaignIDs = argc == 3 && std::strcmp( argv[2], "--campaign-ids" ) == 0;
	if ( argc != 2 && !printRoots && !printCampaignIDs )
	{
		std::fprintf( stderr, "usage: NativeMapDatabaseTests <game.db> [--roots|--campaign-ids]\n" );
		return 2;
	}
	try
	{
		CFileStream database;
		database.OpenRead( argv[1] );
		NDatabase::Serialize( database, CStructureSaver::READ );
		auto *templates = NDatabase::GetTable<NDb::CTemplate>();
		auto *variants = NDatabase::GetTable<NDb::CTemplVariant>();
		if ( !templates || !variants || !templates->GetRecordCount() || !variants->GetRecordCount() )
			return 3;

		std::vector<int> variantIDs;
		CDBIterator<NDb::CTemplVariant> it( *variants );
		while ( it.MoveNext() )
			variantIDs.push_back( it.Get()->GetRecordID() );
		std::sort( variantIDs.begin(), variantIDs.end() );
		unsigned int rootVariants = 0;
		unsigned int rectangles = 0;
		unsigned int zeroSize = 0;
		for ( int id : variantIDs )
		{
			const NDb::CTemplVariant *variant = variants->GetRecord( id );
			const NDb::CTemplate *map = variant->pTemplate.GetPtr();
			if ( !map )
			{
				std::fprintf( stderr, "variant=%d has no template\n", id );
				return 4;
			}
			if ( map->nWidth <= 0 || map->nHeight <= 0 )
				++zeroSize;
			if ( std::find( map->variants.begin(), map->variants.end(), variant ) == map->variants.end() )
				return 5;
			if ( variant->nMinCutFloor > variant->nMaxCutFloor )
				return 6;
			Add( static_cast<std::uint32_t>( id ) );
			Add( static_cast<std::uint32_t>( map->GetRecordID() ) );
			Add( static_cast<std::uint32_t>( map->nWidth ) );
			Add( static_cast<std::uint32_t>( map->nHeight ) );
			Add( static_cast<std::uint32_t>( variant->bGrid ) );
			Add( static_cast<std::uint32_t>( variant->nBorder ) );
			Add( static_cast<std::uint32_t>( variant->nExitBorder ) );
			Add( static_cast<std::uint32_t>( variant->nMinCutFloor ) );
			Add( static_cast<std::uint32_t>( variant->nMaxCutFloor ) );
			Add( static_cast<std::uint32_t>( variant->bNoAttack ) );
			Add( static_cast<std::uint32_t>( variant->weatherType ) );
			Add( static_cast<std::uint32_t>( ID( variant->pScript ) ) );
			AddRecords( variant->rects );
			AddRecords( variant->pFinalElements );
			AddRecords( variant->pUnits );
			AddRecords( variant->explosions );
			AddRecords( variant->terrainSpots );
			AddRecords( variant->waypoints );
			if ( !variant->rects.empty() || !variant->pFinalElements.empty() || !variant->pUnits.empty() )
				++rootVariants;
			rectangles += static_cast<unsigned int>( variant->rects.size() );
		}
		if ( !rootVariants || !rectangles )
			return 7;
		auto *zones = NDatabase::GetTable<NDb::CDBScenarioZone>();
		auto *globalMaps = NDatabase::GetTable<NDb::CGlobalMap>();
		if ( !zones || !globalMaps ) return 9;
		std::set<int> activeScenarioIDs;
		CDBIterator<NDb::CGlobalMap> globalIt( *globalMaps );
		while ( globalIt.MoveNext() )
		{
			if ( printCampaignIDs )
				std::printf( "global_map_id=%d scenario=%d start_zone=%d base_zone=%d\n",
					globalIt.Get()->GetRecordID(), ID(globalIt.Get()->pScenario),
					ID(globalIt.Get()->pStartZone), ID(globalIt.Get()->pBaseZone) );
			if ( globalIt.Get()->pScenario )
				activeScenarioIDs.insert( globalIt.Get()->pScenario->GetRecordID() );
		}
		if ( printCampaignIDs )
		{
			auto *sides = NDatabase::GetTable<NDb::CSide>();
			if ( !sides ) return 14;
			CDBIterator<NDb::CSide> sideIt( *sides );
			while ( sideIt.MoveNext() )
				std::printf( "side_id=%d global_map_id=%d\n",
					sideIt.Get()->GetRecordID(), sideIt.Get()->nGlobalMapID );
			auto *chapterMaps = NDatabase::GetTable<NDb::CChapterMap>();
			if ( !chapterMaps ) return 13;
			CDBIterator<NDb::CChapterMap> chapterIt( *chapterMaps );
			while ( chapterIt.MoveNext() )
				std::printf( "chapter_map_id=%d\n", chapterIt.Get()->GetRecordID() );
		}
		// The shipped side menu offers only the Axis and Allies (DB IDs 1/2).
		// Both must lead to authored global-map records, independently of the
		// extra side rows and debug command that can request arbitrary IDs.
		NDb::CSide *axis = NDb::GetDBSide( 1 );
		NDb::CSide *allies = NDb::GetDBSide( 2 );
		if ( !axis || !allies || axis->nGlobalMapID != 3 ||
			allies->nGlobalMapID != 4 || !NDb::GetGlobalMap( 3 ) ||
			!NDb::GetGlobalMap( 4 ) ) return 15;
		std::set<int> scenarioRoots;
		std::set<int> activeRoots;
		std::size_t activeZones = 0;
		std::size_t missingRoots = 0;
		CDBIterator<NDb::CDBScenarioZone> zoneIt( *zones );
		while ( zoneIt.MoveNext() )
		{
			const bool active = zoneIt.Get()->pScenario &&
				activeScenarioIDs.count( zoneIt.Get()->pScenario->GetRecordID() );
			if ( active ) ++activeZones;
			for ( int id : zoneIt.Get()->templatesIDs )
			{
				if ( id <= 0 ) continue;
				if ( templates->GetDBRecord( id ) )
				{
					scenarioRoots.insert( id );
					if ( active ) activeRoots.insert( id );
				}
				else ++missingRoots;
			}
		}
		std::set<int> reachableTemplates;
		std::set<int> reachableVariants;
		std::set<int> rootVariantIDs;
		std::vector<int> pending( scenarioRoots.begin(), scenarioRoots.end() );
		std::size_t missingChildren = 0;
		while ( !pending.empty() )
		{
			const int id = pending.back();
			pending.pop_back();
			if ( !reachableTemplates.insert( id ).second ) continue;
			auto *map = static_cast<NDb::CTemplate*>( templates->GetDBRecord( id ) );
			if ( !map ) return 10;
			for ( const auto &variant : map->variants )
			{
				if ( !variant ) return 11;
				reachableVariants.insert( variant->GetRecordID() );
				if ( scenarioRoots.count( id ) ) rootVariantIDs.insert( variant->GetRecordID() );
				for ( const auto &rect : variant->rects )
				{
					if ( !rect || !rect->pTemplate )
					{
						++missingChildren;
						continue;
					}
					const int childID = rect->pTemplate->GetRecordID();
					if ( !reachableTemplates.count( childID ) ) pending.push_back( childID );
				}
			}
		}
		if ( printRoots )
		{
			for ( int id : rootVariantIDs )
				std::printf( "scenario_root_variant=%d\n", id );
			for ( int id : activeRoots )
			{
				auto *map = static_cast<NDb::CTemplate*>( templates->GetDBRecord( id ) );
				for ( const auto &variant : map->variants )
					std::printf( "active_root_variant=%d\n", variant->GetRecordID() );
			}
			for ( int id : reachableVariants )
				std::printf( "scenario_reachable_variant=%d\n", id );
		}
		std::uint64_t reachDigest = 14695981039346656037ULL;
		for ( int id : reachableVariants )
			for ( int byte = 0; byte < 4; ++byte )
			{
				reachDigest ^= static_cast<unsigned char>( static_cast<std::uint32_t>(id) >> (byte * 8) );
				reachDigest *= 1099511628211ULL;
			}
		std::printf( "scenario_zones=%zu root_templates=%zu root_variants=%zu reachable_templates=%zu reachable_variants=%zu missing_roots=%zu missing_children=%zu reach_digest=%016llX\n",
			zones->GetRecordCount(), scenarioRoots.size(), rootVariantIDs.size(),
			reachableTemplates.size(), reachableVariants.size(), missingRoots,
			missingChildren, static_cast<unsigned long long>( reachDigest ) );
		std::printf( "global_maps=%zu active_scenarios=%zu active_zones=%zu active_root_templates=%zu\n",
			globalMaps->GetRecordCount(), activeScenarioIDs.size(), activeZones, activeRoots.size() );
		if ( scenarioRoots.empty() || rootVariantIDs.empty() || reachableVariants.empty() ||
			activeScenarioIDs.size() != 3 || activeZones != zones->GetRecordCount() ||
			activeRoots != scenarioRoots || missingRoots || missingChildren ||
			reachDigest != UINT64_C(0xC568A7F7EB837585) )
			return 12;
		std::printf( "templates=%zu variants=%zu populated=%u rectangles=%u zero_size=%u digest=%016llX\n",
			templates->GetRecordCount(), variantIDs.size(), rootVariants, rectangles, zeroSize,
			static_cast<unsigned long long>( digest ) );
		return 0;
	}
	catch ( ... )
	{
		std::fprintf( stderr, "map database load or relationship failed\n" );
		return 8;
	}
}
