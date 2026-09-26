#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../Main/MapBuild.h"
#include "../Main/aiGrid.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataMap.h"
#include "../DBFormat/DataRPG.h"
#include "../Script/lua.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

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
	if ( argc != 4 )
		return 2;
	CFileStream database;
	database.OpenRead( argv[1] );
	NDatabase::Serialize( database, CStructureSaver::READ );
	NGScene::AddResourceDir( argv[2] );
	const int variantID = std::atoi( argv[3] );
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
		lua_State *state = lua_open( 0 );
		if ( !state ) return 8;
		const int status = lua_parsebuffer( state, record->strCode.data(),
			record->strCode.size(), "game.db mission script" );
		lua_close( state );
		if ( status ) return 9;
		scriptBytes += record->strCode.size();
	}
	Add( built );
	Add( map.terrain.nWidth ); Add( map.terrain.nHeight );
	if ( map.terrain.nWidth > 0 && map.terrain.nHeight > 0 )
	{
		Add( map.terrain.heightMap[0][0] );
		Add( map.terrain.typeMap[0][0] );
	}
	for ( const SMapBuilding &building : map.buildings )
	{
		Add( building.pVariant ? building.pVariant->GetRecordID() : 0 );
		Add( building.mpos.nFloor );
		Add( Float2Int( building.mpos.ptPos.x * 1000 ) );
		Add( Float2Int( building.mpos.ptPos.y * 1000 ) );
	}
	Add( static_cast<std::uint32_t>( map.scripts.size() ) );
	Add( static_cast<std::uint32_t>( map.groups.size() ) );
	for ( const SMapUnit &unit : map.units )
	{
		Add( unit.nUnitID ); Add( unit.pos.nFloor );
		Add( Float2Int( unit.pos.ptPos.x * 1000 ) );
		Add( Float2Int( unit.pos.ptPos.y * 1000 ) );
		Add( Float2Int( unit.pos.ptPos.z * 1000 ) );
		Add( static_cast<std::uint32_t>( unit.route.size() ) );
		Add( unit.nDiplomacy );
	}
	for ( const CObj<CMapWaypoint> &waypoint : map.waypoints )
	{
		Add( waypoint->pos.nFloor );
		Add( Float2Int( waypoint->pos.ptPos.x * 1000 ) );
		Add( Float2Int( waypoint->pos.ptPos.y * 1000 ) );
		Add( Float2Int( waypoint->pos.ptPos.z * 1000 ) );
		Add( static_cast<std::uint32_t>( waypoint->commands.size() ) );
		Add( waypoint->b3DWaypoint );
	}
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
	if ( variantID == 218 &&
		( map.buildings.size() != 1 || !map.units.empty() ||
		  map.items.size() != 21 || !map.waypoints.empty() ||
		  !map.slots.empty() || !map.scripts.empty() ||
		  digest != UINT64_C(0x751202F4B6E394E0) ) )
		return 4;
	if ( variantID == 810 &&
		( map.buildings.size() != 1 || map.units.size() != 2 ||
		  map.items.size() != 24 || map.waypoints.size() != 2 ||
		  !map.slots.empty() || map.scripts.size() != 1 || scriptBytes != 1016 ||
		  digest != UINT64_C(0x558E222D9ED26DA6) ) )
		return 4;
	if ( variantID == 4526 &&
		( map.buildings.size() != 7 || map.units.size() != 49 ||
		  map.items.size() != 1514 || map.waypoints.size() != 24 ||
		  map.slots.size() != 2 || map.scripts.size() != 1 || scriptBytes != 4266 ||
		  digest != UINT64_C(0x8C92AD1F7E8B90FA) ) )
		return 4;
	return built ? 0 : 3;
}
