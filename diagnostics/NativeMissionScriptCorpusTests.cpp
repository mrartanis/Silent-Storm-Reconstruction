#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataInterface.h"
#include "../DBFormat/DataMap.h"
#include "../DBFormat/DataRPG.h"
#include "../Script/lua.h"
#include "../Script/lstate.h"
#include "../Script/lopcodes.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <string>
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
	std::map<std::string, std::size_t> globalReads;
	std::set<int> windowScriptIDs;
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
		if ( status )
		{
			std::fprintf( stderr, "Lua parse failed id=%d bytes=%zu status=%d\n",
				script->GetRecordID(), script->strCode.size(), status );
			lua_close( state );
			return 6;
		}
		for ( int protoIndex = 0; protoIndex < state->protos.size(); ++protoIndex )
		{
			Proto *proto = state->protos[protoIndex];
			if ( !proto ) continue;
			for ( Instruction instruction : proto->code )
			{
				if ( GET_OPCODE(instruction) != OP_GETGLOBAL ) continue;
				const int stringIndex = GETARG_U(instruction);
				if ( stringIndex < 0 || stringIndex >= static_cast<int>( proto->strings.size() ) ||
					!proto->strings[stringIndex] )
				{
					lua_close( state );
					return 8;
				}
				const std::string &name = proto->strings[stringIndex]->GetStr();
				++globalReads[name];
				if ( name == "CreateWindow" || name == "GetWindow" ||
					name == "windowGetProperty" || name == "windowSetProperty" ||
					name == "ButtonGetState" || name == "ButtonCreateState" ||
					name == "GetCursorPos" || name == "GetUITime" )
				{
					windowScriptIDs.insert( script->GetRecordID() );
					std::printf( "window_global_script id=%d name=%s\n",
						script->GetRecordID(), name.c_str() );
				}
			}
		}
		lua_close( state );
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
	static const char *windowGlobals[] = {
		"CreateWindow", "GetWindow", "windowGetProperty", "windowSetProperty",
		"ButtonGetState", "ButtonCreateState", "GetCursorPos", "GetUITime" };
	std::size_t windowReads = 0;
	for ( const char *name : windowGlobals )
	{
		const std::size_t count = globalReads[name];
		windowReads += count;
		std::printf( "game_db_lua_global %s=%zu\n", name, count );
	}
	std::printf( "game_db_lua_unique_globals=%zu window_reads=%zu\n",
		globalReads.size(), windowReads );
	static const char *interfaceGlobals[] = {
		"ShowObjectives", "uiShowStore", "uiShowTeamMngMenu",
		"ShowLoseDialog", "ShowLeaveZoneDialog", "ShowHint", "AddHints",
		"SetTutorialMode", "ClueShow", "ExitToChapter" };
	for ( const char *name : interfaceGlobals )
		std::printf( "game_db_lua_interface %s=%zu\n", name, globalReads[name] );
	if ( !NDatabase::GetTable<NDb::CUIContainer>() ||
		!NDatabase::GetTable<NDb::CTemplVariant>() ||
		!NDatabase::GetTable<NDb::CGlobalMap>() ||
		!NDatabase::GetTable<NDb::CChapterMap>() ||
		!NDatabase::GetTable<NDb::CRPGPers>() ) return 9;
	std::size_t script126References = 0;
	if ( auto *containers = NDatabase::GetTable<NDb::CUIContainer>() )
	{
		CDBIterator<NDb::CUIContainer> iter( *containers );
		while ( iter.MoveNext() )
			if ( iter.Get()->pScript && iter.Get()->pScript->GetRecordID() == 126 )
				++script126References;
	}
	if ( auto *variants = NDatabase::GetTable<NDb::CTemplVariant>() )
	{
		CDBIterator<NDb::CTemplVariant> iter( *variants );
		while ( iter.MoveNext() )
			if ( iter.Get()->pScript && iter.Get()->pScript->GetRecordID() == 126 )
				++script126References;
	}
	if ( auto *globalMaps = NDatabase::GetTable<NDb::CGlobalMap>() )
	{
		CDBIterator<NDb::CGlobalMap> iter( *globalMaps );
		while ( iter.MoveNext() )
			if ( iter.Get()->pScript && iter.Get()->pScript->GetRecordID() == 126 )
				++script126References;
	}
	if ( auto *chapterMaps = NDatabase::GetTable<NDb::CChapterMap>() )
	{
		CDBIterator<NDb::CChapterMap> iter( *chapterMaps );
		while ( iter.MoveNext() )
			if ( iter.Get()->pScript && iter.Get()->pScript->GetRecordID() == 126 )
				++script126References;
	}
	if ( auto *personas = NDatabase::GetTable<NDb::CRPGPers>() )
	{
		CDBIterator<NDb::CRPGPers> iter( *personas );
		while ( iter.MoveNext() )
			for ( const CPtr<NDb::CScript> &linked : iter.Get()->scripts )
				if ( linked && linked->GetRecordID() == 126 )
					++script126References;
	}
	std::printf( "script_126_db_refs=%zu\n", script126References );
	// The original baseline corpus is our fixed cross-architecture oracle.
	return scripts.size() == 113 && empty == 1 && bytes == 355575 &&
		digest == UINT64_C(0xB5163E4E76664106) && windowReads == 6 &&
		globalReads["ShowObjectives"] == 0 &&
		windowScriptIDs.size() == 1 && *windowScriptIDs.begin() == 126 &&
		script126References == 0 ? 0 : 7;
}
