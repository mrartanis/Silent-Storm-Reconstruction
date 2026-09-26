#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataRPG.h"
#include "../Script/lua.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

int main( int argc, char **argv )
{
	if ( argc != 2 )
		return 2;
	CFileStream database;
	database.OpenRead( argv[1] );
	NDatabase::Serialize( database, CStructureSaver::READ );
	auto *table = NDatabase::GetTable<NDb::CScript>();
	if ( !table || !table->GetRecordCount() )
		return 3;
	std::vector<const NDb::CScript *> scripts;
	CDBIterator<NDb::CScript> it( *table );
	while ( it.MoveNext() ) scripts.push_back( it.Get() );
	std::sort( scripts.begin(), scripts.end(), []( const auto *a, const auto *b ) {
		return a->GetRecordID() < b->GetRecordID();
	} );
	std::uint64_t digest = UINT64_C(14695981039346656037);
	std::size_t bytes = 0;
	std::size_t empty = 0;
	for ( const NDb::CScript *script : scripts )
	{
		if ( script->strCode.empty() )
		{
			++empty;
			continue;
		}
		lua_State *state = lua_open( 0 );
		if ( !state ) return 5;
		const int status = lua_parsebuffer( state, script->strCode.data(),
			script->strCode.size(), "game.db script" );
		lua_close( state );
		if ( status )
		{
			std::fprintf( stderr, "Lua parse failed id=%d bytes=%zu status=%d\n",
				script->GetRecordID(), script->strCode.size(), status );
			return 6;
		}
		std::uint32_t id = static_cast<std::uint32_t>( script->GetRecordID() );
		for ( int i = 0; i < 4; ++i )
		{
			digest ^= static_cast<unsigned char>( id >> ( 8 * i ) );
			digest *= UINT64_C(1099511628211);
		}
		for ( unsigned char c : script->strCode )
		{
			digest ^= c;
			digest *= UINT64_C(1099511628211);
		}
		bytes += script->strCode.size();
	}
	std::printf( "scripts=%zu empty=%zu bytes=%zu digest=%016llX\n",
		scripts.size(), empty, bytes, static_cast<unsigned long long>( digest ) );
	// The original baseline corpus is our fixed cross-architecture oracle.
	return scripts.size() == 113 && empty == 1 && bytes == 355575 &&
		digest == UINT64_C(0xB5163E4E76664106) ? 0 : 7;
}
