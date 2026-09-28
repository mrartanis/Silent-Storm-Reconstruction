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
#include "../DBFormat/DataScenario.h"
#include "../Script/lua.h"
#include "../Script/lstate.h"
#include "../Script/lopcodes.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

static std::uint64_t HashIDs( const std::set<int> &ids )
{
	std::uint64_t hash = UINT64_C(14695981039346656037);
	for ( int id : ids )
		for ( int byte = 0; byte < 4; ++byte )
		{
			hash ^= static_cast<std::uint8_t>( id >> ( byte * 8 ) );
			hash *= UINT64_C(1099511628211);
		}
	return hash;
}

static std::set<int> ScenarioVariantIDs()
{
	auto *zones = NDatabase::GetTable<NDb::CDBScenarioZone>();
	auto *templates = NDatabase::GetTable<NDb::CTemplate>();
	if ( !zones || !templates ) throw 1;
	std::set<int> seenTemplates, variantIDs;
	std::vector<int> pending;
	CDBIterator<NDb::CDBScenarioZone> zone( *zones );
	while ( zone.MoveNext() )
		for ( int id : zone.Get()->templatesIDs )
			if ( id > 0 ) pending.push_back( id );
	while ( !pending.empty() )
	{
		int id = pending.back(); pending.pop_back();
		if ( !seenTemplates.insert( id ).second ) continue;
		auto *map = static_cast<NDb::CTemplate *>( templates->GetDBRecord( id ) );
		if ( !map ) throw 2;
		for ( const auto &variant : map->variants )
		{
			if ( !variant ) throw 3;
			variantIDs.insert( variant->GetRecordID() );
			for ( const auto &rect : variant->rects )
				if ( rect && rect->pTemplate )
					pending.push_back( rect->pTemplate->GetRecordID() );
		}
	}
	return variantIDs;
}

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
	std::size_t numericConstants = 0, wideIntegerConstants = 0;
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
			for ( Number number : proto->numbers )
			{
				++numericConstants;
				if ( std::isfinite(number) && std::trunc(number) == number &&
					(number < static_cast<Number>((std::numeric_limits<std::int32_t>::min)()) ||
					 number > static_cast<Number>((std::numeric_limits<std::int32_t>::max)())) )
				{
					++wideIntegerConstants;
					std::printf( "lua_wide_integer script=%d value=%.17g\n",
						script->GetRecordID(), static_cast<double>(number) );
				}
			}
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
	std::printf( "lua_numeric_constants=%zu wide_integer_constants=%zu\n",
		numericConstants, wideIntegerConstants );
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
	std::set<int> parsedIDs;
	for ( const auto *script : scripts )
		if ( !script->strCode.empty() ) parsedIDs.insert( script->GetRecordID() );
	std::map<std::string, std::set<int>> linked;
	linked["persona"]; linked["ui_container"];
	auto add = [&]( const char *family, const NDb::CScript *script ) {
		if ( script ) linked[family].insert( script->GetRecordID() );
	};
	const auto scenarioVariants = ScenarioVariantIDs();
	if ( auto *variants = NDatabase::GetTable<NDb::CTemplVariant>() )
	{
		CDBIterator<NDb::CTemplVariant> iter( *variants );
		while ( iter.MoveNext() )
		{
			add( "variant_all", iter.Get()->pScript );
			if ( scenarioVariants.count( iter.Get()->GetRecordID() ) )
				add( "variant_scenario", iter.Get()->pScript );
		}
	}
	if ( auto *globals = NDatabase::GetTable<NDb::CGlobalMap>() )
	{
		CDBIterator<NDb::CGlobalMap> iter( *globals );
		while ( iter.MoveNext() ) add( "global", iter.Get()->pScript );
	}
	if ( auto *chapters = NDatabase::GetTable<NDb::CChapterMap>() )
	{
		CDBIterator<NDb::CChapterMap> iter( *chapters );
		while ( iter.MoveNext() ) add( "chapter", iter.Get()->pScript );
	}
	if ( auto *personas = NDatabase::GetTable<NDb::CRPGPers>() )
	{
		CDBIterator<NDb::CRPGPers> iter( *personas );
		while ( iter.MoveNext() )
			for ( const auto &script : iter.Get()->scripts ) add( "persona", script );
	}
	if ( auto *containers = NDatabase::GetTable<NDb::CUIContainer>() )
	{
		CDBIterator<NDb::CUIContainer> iter( *containers );
		while ( iter.MoveNext() ) add( "ui_container", iter.Get()->pScript );
	}
	linked["main_menu"].insert( 85 ); // iMainMenu.cpp's shipped hardcoded DB script
	std::set<int> allLinked;
	for ( const auto &family : linked )
	{
		std::printf( "script_roots family=%s count=%zu digest=%016llX\n",
			family.first.c_str(), family.second.size(),
			static_cast<unsigned long long>(HashIDs(family.second)) );
		allLinked.insert( family.second.begin(), family.second.end() );
	}
	std::set<int> unparsed;
	std::set_difference( allLinked.begin(), allLinked.end(), parsedIDs.begin(), parsedIDs.end(),
		std::inserter( unparsed, unparsed.end() ) );
	std::printf( "script_roots scenario_variants=%zu linked=%zu unparsed=%zu linked_digest=%016llX\n",
		scenarioVariants.size(), allLinked.size(), unparsed.size(),
		static_cast<unsigned long long>(HashIDs(allLinked)) );
	for ( int id : unparsed ) std::printf( "script_root_unparsed id=%d\n", id );
	// The original baseline corpus is our fixed cross-architecture oracle.
	return scripts.size() == 113 && empty == 1 && bytes == 355575 &&
		numericConstants == 18 && wideIntegerConstants == 0 &&
		digest == UINT64_C(0xC2462A66D562BAF6) && windowReads == 6 &&
		globalReads["ShowObjectives"] == 0 &&
		windowScriptIDs.size() == 1 && *windowScriptIDs.begin() == 126 &&
		script126References == 0 && scenarioVariants.size() == 985 &&
		linked["variant_scenario"].size() == 47 &&
		HashIDs(linked["variant_scenario"]) == UINT64_C(0x20F53D173CF351DE) &&
		linked["variant_all"].size() == 91 &&
		HashIDs(linked["variant_all"]) == UINT64_C(0x90E59C7355171CA1) &&
		linked["global"].size() == 2 &&
		HashIDs(linked["global"]) == UINT64_C(0xAE2C4B7421D7D3F4) &&
		linked["chapter"].size() == 4 &&
		HashIDs(linked["chapter"]) == UINT64_C(0xCC819302C587EE7B) &&
		linked["persona"].empty() && linked["ui_container"].empty() &&
		allLinked.size() == 97 &&
		HashIDs(allLinked) == UINT64_C(0x7697CE69E943ED7E) &&
		unparsed.empty() ? 0 : 7;
}
