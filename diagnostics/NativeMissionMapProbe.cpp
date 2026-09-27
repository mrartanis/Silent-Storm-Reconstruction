#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../Main/MapBuild.h"
#include "../Main/aiGrid.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataMap.h"
#include "../DBFormat/DataRPG.h"
#include "../Misc/RandomGen.h"
#include "../Script/lua.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>

static std::uint64_t digest = UINT64_C(14695981039346656037);
static std::uint64_t routeDigest = UINT64_C(14695981039346656037);
static std::uint64_t behaviorDigest = UINT64_C(14695981039346656037);
static void Add( std::uint32_t value )
{
	for ( int byte = 0; byte < 4; ++byte )
	{
		digest ^= static_cast<std::uint8_t>( value >> ( byte * 8 ) );
		digest *= UINT64_C(1099511628211);
	}
}
static void AddRouteWord( std::uint32_t value )
{
	for ( int byte = 0; byte < 4; ++byte )
	{
		routeDigest ^= static_cast<std::uint8_t>( value >> ( byte * 8 ) );
		routeDigest *= UINT64_C(1099511628211);
	}
}
static void AddBehaviorWord( std::uint32_t value )
{
	for ( int byte = 0; byte < 4; ++byte )
	{
		behaviorDigest ^= static_cast<std::uint8_t>( value >> ( byte * 8 ) );
		behaviorDigest *= UINT64_C(1099511628211);
	}
}
static std::uint32_t FloatBits( float value )
{
	std::uint32_t bits = 0;
	static_assert( sizeof(bits) == sizeof(value), "32-bit game float expected" );
	std::memcpy( &bits, &value, sizeof(bits) );
	return bits;
}
static void AddBehaviorFloat( float value )
{
	AddBehaviorWord( FloatBits( value ) );
}
static void AddBehaviorCommands( const std::vector<NAI::SCommand> &commands )
{
	AddBehaviorWord( static_cast<std::uint32_t>( commands.size() ) );
	for ( const NAI::SCommand &command : commands )
	{
		AddBehaviorWord( command.cmd );
		AddBehaviorFloat( command.ptPos.x );
		AddBehaviorFloat( command.ptPos.y );
		AddBehaviorFloat( command.ptPos.z );
		AddBehaviorWord( command.time );
		AddBehaviorWord( command.pose );
		AddBehaviorWord( command.dir );
	}
}
static std::size_t AddRoute( const std::vector<CPtr<CMapWaypoint> > &route )
{
	AddRouteWord( static_cast<std::uint32_t>( route.size() ) );
	AddBehaviorWord( static_cast<std::uint32_t>( route.size() ) );
	for ( const CPtr<CMapWaypoint> &point : route )
	{
		if ( !point || !point->pName ) return 0;
		AddRouteWord( point->pName->GetRecordID() );
		AddRouteWord( point->bExists );
		AddRouteWord( static_cast<std::uint32_t>( point->commands.size() ) );
		AddBehaviorWord( point->pName->GetRecordID() );
		AddBehaviorWord( point->bExists );
		// An unresolved route point carries only its database name. Its
		// position is not initialized until a matching map waypoint exists.
		if ( point->bExists )
		{
			AddBehaviorFloat( point->pos.ptPos.x );
			AddBehaviorFloat( point->pos.ptPos.y );
			AddBehaviorFloat( point->pos.ptPos.z );
			AddBehaviorWord( point->pos.nFloor );
		}
		AddBehaviorCommands( point->commands );
	}
	return route.size();
}

int main( int argc, char **argv )
{
	const bool printScripts = argc == 5 && !std::strcmp( argv[4], "--print-scripts" );
	const bool deterministic = argc == 5 && !std::strcmp( argv[4], "--deterministic" );
	if ( argc != 4 && !printScripts && !deterministic )
		return 2;
	CFileStream database;
	database.OpenRead( argv[1] );
	NDatabase::Serialize( database, CStructureSaver::READ );
	NGScene::AddResourceDir( argv[2] );
	const int variantID = std::atoi( argv[3] );
	if ( variantID == -1 )
	{
		S2FileIO::PortablePackageIndex units;
		if ( !units.Open( ( std::string( argv[2] ) + "/Units.res" ) ) ) return 11;
		auto *variants = NDatabase::GetTable<NDb::CTemplVariant>();
		if ( !variants ) return 5;
		int shown = 0;
		CDBIterator<NDb::CTemplVariant> it( *variants );
		while ( it.MoveNext() && shown < 30 )
		{
			const NDb::CTemplVariant *variant = it.Get();
			const NDb::CTemplate *map = variant->pTemplate.GetPtr();
			if ( !map || map->nWidth < 16 || map->nHeight < 16 ) continue;
			int routed = 0;
			for ( const auto &unit : variant->pUnits )
				if ( unit && units.Entries().count( unit->GetRecordID() ) ) ++routed;
			if ( !routed ) continue;
			std::printf( "route_candidate=%d size=%dx%d units=%zu routed=%d waypoints=%zu\n",
				variant->GetRecordID(), map->nWidth, map->nHeight,
				variant->pUnits.size(), routed, variant->waypoints.size() );
			++shown;
		}
		return shown ? 0 : 6;
	}
	if ( variantID == 0 )
	{
		auto *variants = NDatabase::GetTable<NDb::CTemplVariant>();
		if ( !variants ) return 5;
		int shown = 0;
		CDBIterator<NDb::CTemplVariant> it( *variants );
		while ( it.MoveNext() && shown < 20 )
		{
			const NDb::CTemplVariant *variant = it.Get();
			const NDb::CTemplate *map = variant->pTemplate.GetPtr();
			if ( !map || map->nWidth < 16 || map->nHeight < 16 ||
				variant->pUnits.empty() || variant->waypoints.empty() )
				continue;
			std::printf( "candidate=%d size=%dx%d units=%zu waypoints=%zu script=%d\n",
				variant->GetRecordID(), map->nWidth, map->nHeight,
				variant->pUnits.size(), variant->waypoints.size(),
				variant->pScript.GetPtr() != nullptr );
			++shown;
		}
		return shown ? 0 : 6;
	}
	// BuildMap has retail clock-seeded auxiliary SRand instances (e.g. item
	// models and border objects) in addition to its explicit map seed. Freeze
	// those only for paired architecture probes; the game path is unchanged.
	if ( deterministic )
	{
#if defined(_WIN32)
		random.SeedForHarness( 123 );
#else
		s2_game_random.SeedForHarness( 123 );
#endif
	}
	CObj<NAI::CPathNetwork> network = new NAI::CPathNetwork;
	SMapInfo map;
	const bool built = BuildMap( variantID, std::vector<std::string>(),
		network, &map, -1, SRandomSeed( 123 ) );
	// Mission scripts are stored in game.db, not in the startup .l corpus.
	// Parse every script selected by the real builder with the game's Lua VM.
	// This does not claim that the game-specific bindings can run headlessly.
	std::size_t scriptBytes = 0;
	for ( const CDBPtr<NDb::CScript> &record : map.scripts )
	{
		if ( !record || record->strCode.empty() ) return 7;
		if ( printScripts )
			std::printf( "\nSCRIPT ID %d (%zu bytes)\n%.*s\n", record->GetRecordID(),
				record->strCode.size(), static_cast<int>( record->strCode.size() ),
				record->strCode.data() );
		lua_State *state = lua_open( 0 );
		if ( !state ) return 8;
		const int status = lua_parsebuffer( state, record->strCode.data(),
			record->strCode.size(), "game.db mission script" );
		lua_close( state );
		if ( status ) return 9;
		scriptBytes += record->strCode.size();
		AddBehaviorWord( record->GetRecordID() );
		AddBehaviorWord( static_cast<std::uint32_t>( record->strCode.size() ) );
		for ( unsigned char byte : record->strCode )
		{
			behaviorDigest ^= byte;
			behaviorDigest *= UINT64_C(1099511628211);
		}
	}
	const std::uint64_t scriptBehaviorDigest = behaviorDigest;
	Add( built );
	Add( map.terrain.nWidth ); Add( map.terrain.nHeight );
	if ( map.terrain.nWidth > 0 && map.terrain.nHeight > 0 )
	{
		Add( map.terrain.heightMap[0][0] );
		Add( map.terrain.typeMap[0][0] );
	}
	const std::uint64_t terrainDigest = digest;
	for ( const SMapBuilding &building : map.buildings )
	{
		Add( building.pVariant ? building.pVariant->GetRecordID() : 0 );
		Add( building.mpos.nFloor );
		Add( Float2Int( building.mpos.ptPos.x * 1000 ) );
		Add( Float2Int( building.mpos.ptPos.y * 1000 ) );
	}
	const std::uint64_t buildingDigest = digest;
	Add( static_cast<std::uint32_t>( map.scripts.size() ) );
	Add( static_cast<std::uint32_t>( map.groups.size() ) );
	std::size_t unitRoutePoints = 0, groupRoutePoints = 0, routedUnits = 0;
	for ( const SMapUnit &unit : map.units )
	{
		Add( unit.nUnitID ); Add( unit.pos.nFloor );
		Add( Float2Int( unit.pos.ptPos.x * 1000 ) );
		Add( Float2Int( unit.pos.ptPos.y * 1000 ) );
		Add( Float2Int( unit.pos.ptPos.z * 1000 ) );
		Add( static_cast<std::uint32_t>( unit.route.size() ) );
		Add( unit.nDiplomacy );
		AddRouteWord( unit.nUnitID );
		AddBehaviorWord( unit.nUnitID );
		AddBehaviorWord( unit.pPers ? unit.pPers->GetRecordID() : 0 );
		AddBehaviorWord( unit.eInitialPose );
		AddBehaviorWord( unit.eLogic );
		AddBehaviorWord( unit.nRoamingRadius );
		AddBehaviorWord( unit.bFearUseToHit );
		AddBehaviorWord( unit.pGuardAnimation ? unit.pGuardAnimation->GetRecordID() : 0 );
		AddBehaviorWord( unit.nDiplomacy );
		AddBehaviorWord( unit.nScenarioPlayer );
		AddBehaviorWord( unit.nRelativeLevel );
		if ( !unit.route.empty() ) ++routedUnits;
		unitRoutePoints += AddRoute( unit.route );
	}
	const std::uint64_t unitDigest = digest;
	const std::uint64_t unitBehaviorDigest = behaviorDigest;
	std::vector<int> groupIDs;
	for ( const auto &group : map.groups ) groupIDs.push_back( group.first );
	std::sort( groupIDs.begin(), groupIDs.end() );
	for ( int id : groupIDs )
	{
		AddRouteWord( id );
		AddBehaviorWord( id );
		AddBehaviorWord( static_cast<std::uint32_t>( map.groups.at( id ).units.size() ) );
		for ( int unitID : map.groups.at( id ).units ) AddBehaviorWord( unitID );
		groupRoutePoints += AddRoute( map.groups.at( id ).route );
	}
	const std::uint64_t groupBehaviorDigest = behaviorDigest;
	for ( const CObj<CMapWaypoint> &waypoint : map.waypoints )
	{
		Add( waypoint->pos.nFloor );
		Add( Float2Int( waypoint->pos.ptPos.x * 1000 ) );
		Add( Float2Int( waypoint->pos.ptPos.y * 1000 ) );
		Add( Float2Int( waypoint->pos.ptPos.z * 1000 ) );
		Add( static_cast<std::uint32_t>( waypoint->commands.size() ) );
		Add( waypoint->b3DWaypoint );
		AddBehaviorWord( waypoint->pName ? waypoint->pName->GetRecordID() : 0 );
		AddBehaviorWord( waypoint->bExists );
		AddBehaviorWord( waypoint->b3DWaypoint );
		AddBehaviorFloat( waypoint->pos.ptPos.x );
		AddBehaviorFloat( waypoint->pos.ptPos.y );
		AddBehaviorFloat( waypoint->pos.ptPos.z );
		AddBehaviorWord( waypoint->pos.nFloor );
		AddBehaviorCommands( waypoint->commands );
	}
	const std::uint64_t waypointDigest = digest;
	const std::uint64_t waypointBehaviorDigest = behaviorDigest;
	Add( static_cast<std::uint32_t>( map.slots.size() ) );
	for ( const SClueSlot &slot : map.slots )
	{
		Add( Float2Int( slot.pos.ptPos.x * 1000 ) );
		Add( Float2Int( slot.pos.ptPos.y * 1000 ) );
		Add( Float2Int( slot.pos.ptPos.z * 1000 ) );
		Add( Float2Int( slot.ptAlignTo.x * 1000 ) );
		Add( Float2Int( slot.ptAlignTo.y * 1000 ) );
		Add( slot.bPersSlot ); Add( slot.bInventorySlot );
		if ( slot.bPersSlot || slot.bInventorySlot )
			Add( slot.nUnitID );
	}
	const std::uint64_t slotDigest = digest;
	for ( const SMapElement &item : map.items )
	{
		Add( item.pObject ? item.pObject->nParentID : 0 );
		Add( item.pos.nFloor );
		Add( Float2Int( item.pos.ptPos.x * 1000 ) );
		Add( Float2Int( item.pos.ptPos.y * 1000 ) );
		Add( Float2Int( item.pos.ptPos.z * 1000 ) );
		Add( item.nRelFloor );
		Add( item.bOpen ); Add( item.bBorder );
	}
	std::printf( "built=%d buildings=%zu units=%zu items=%zu waypoints=%zu slots=%zu scripts=%zu script_bytes=%zu digest=%016llX\n",
		built, map.buildings.size(), map.units.size(), map.items.size(),
		map.waypoints.size(), map.slots.size(), map.scripts.size(), scriptBytes,
		static_cast<unsigned long long>( digest ) );
	std::printf( "routes units=%zu unit_points=%zu group_points=%zu route_digest=%016llX\n",
		routedUnits, unitRoutePoints, groupRoutePoints,
		static_cast<unsigned long long>( routeDigest ) );
	std::printf( "behavior_digest=%016llX\n",
		static_cast<unsigned long long>( behaviorDigest ) );
	std::printf( "component_digest terrain=%016llX buildings=%016llX units=%016llX waypoints=%016llX slots=%016llX items=%016llX\n",
		static_cast<unsigned long long>( terrainDigest ),
		static_cast<unsigned long long>( buildingDigest ),
		static_cast<unsigned long long>( unitDigest ),
		static_cast<unsigned long long>( waypointDigest ),
		static_cast<unsigned long long>( slotDigest ),
		static_cast<unsigned long long>( digest ) );
	std::printf( "behavior_components scripts=%016llX units=%016llX groups=%016llX waypoints=%016llX\n",
		static_cast<unsigned long long>( scriptBehaviorDigest ),
		static_cast<unsigned long long>( unitBehaviorDigest ),
		static_cast<unsigned long long>( groupBehaviorDigest ),
		static_cast<unsigned long long>( waypointBehaviorDigest ) );
	if ( variantID == 218 &&
		( map.buildings.size() != 1 || !map.units.empty() ||
		  map.items.size() != 21 || !map.waypoints.empty() ||
		  !map.slots.empty() || !map.scripts.empty() ||
		  routedUnits != 0 || unitRoutePoints != 0 || groupRoutePoints != 0 ||
		  routeDigest != UINT64_C(0xCBF29CE484222325) ||
		  behaviorDigest != UINT64_C(0xCBF29CE484222325) ||
		  digest != UINT64_C(0x751202F4B6E394E0) ) )
		return 4;
	if ( variantID == 810 &&
		( map.buildings.size() != 1 || map.units.size() != 2 ||
		  map.items.size() != 24 || map.waypoints.size() != 2 ||
		  !map.slots.empty() || map.scripts.size() != 1 || scriptBytes != 1016 ||
		  routedUnits != 0 || unitRoutePoints != 0 || groupRoutePoints != 0 ||
		  routeDigest != UINT64_C(0xC3F6B0B1E282EE0F) ||
		  behaviorDigest != UINT64_C(0x1150A6A5921D1F4D) ||
		  digest != UINT64_C(0x558E222D9ED26DA6) ) )
		return 4;
	if ( variantID == 2400 &&
		( map.buildings.size() != 1 || map.units.size() != 5 ||
		  map.items.size() != 16 || map.waypoints.size() != 2 ||
		  routedUnits != 5 || unitRoutePoints != 10 || groupRoutePoints != 0 ||
		  routeDigest != UINT64_C(0xBDADCF3CD005A847) ||
		  behaviorDigest != UINT64_C(0x12659D562097CF78) ||
		  digest != UINT64_C(0x29C99EC264C4C6E7) ) )
		return 4;
	if ( variantID == 3830 && deterministic &&
		( map.buildings.size() != 7 || map.units.size() != 26 ||
		  map.items.size() != 1047 || map.waypoints.size() != 54 ||
		  map.slots.size() != 5 || map.scripts.size() != 1 || scriptBytes != 111 ||
		  routedUnits != 10 || unitRoutePoints != 36 || groupRoutePoints != 17 ||
		  routeDigest != UINT64_C(0xF11ECF559130C3B2) ||
		  behaviorDigest != UINT64_C(0x04DA549A759E4957) ||
		  digest != UINT64_C(0x2DD3DDC11A704B89) ) )
		return 4;
	if ( variantID == 4526 &&
		( map.buildings.size() != 7 || map.units.size() != 49 ||
		  map.items.size() != 1514 || map.waypoints.size() != 24 ||
		  map.slots.size() != 2 || map.scripts.size() != 1 || scriptBytes != 4266 ||
		  routedUnits != 0 || unitRoutePoints != 0 || groupRoutePoints != 14 ||
		  routeDigest != UINT64_C(0xC9FCDC98D9DCD6F3) ||
		  behaviorDigest != UINT64_C(0x7EAF4F97AD129EED) ||
		  digest != UINT64_C(0xA65A2070B18C1067) ) )
		return 4;
	return built ? 0 : 3;
}
