#include "../FileIO/StdAfx.h"
#include "../FileIO/Streams.h"
#include "../ADOImport/BasicDB.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataAI.h"
#include "../DBFormat/DataInterface.h"
#include "../FileIO/PortableGameDatabase.h"

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <exception>
#include <stdexcept>

int main( int argc, char **argv )
{
	const bool auditNonAscii = argc == 3 && std::strcmp( argv[2], "--nonascii" ) == 0;
	if ( argc != 2 && !auditNonAscii )
	{
		std::fprintf( stderr, "usage: NativeGameDatabaseLoadTests <game.db> [--nonascii]\n" );
		return 2;
	}
	try
	{
		CFileStream database;
		database.OpenRead( argv[1] );
		NDatabase::Serialize( database, CStructureSaver::READ );
		// The authored walk-step sound is one case where armor type 5 reaches
		// the retail accessor's sixth float slot. For this original-db record
		// the adjacent pSound is null, so the x86 instruction reads zero, not R5.
		NDb::CAISound *walkStep = NDb::GetAISound( 5 );
		if ( !walkStep || walkStep->bTileTypeIndependent ||
			IsValid( walkStep->pSound ) ||
			walkStep->GetRadiusFromAISoundType( 5 ) != 0 )
			return 15;
		for ( int type = 0; type != 5; ++type )
			if ( walkStep->GetRadiusFromAISoundType( type ) !=
				static_cast<int>( walkStep->vRadius[type] ) )
				return 16;
		S2FileIO::PortableGameDatabase decoded;
		std::string error;
		if ( !S2FileIO::LoadPortableGameDatabase( argv[1], &decoded, &error ) )
			throw std::runtime_error( error );
		unsigned int comparedTables = 0;
		std::size_t comparedIDs = 0;
		for ( const auto &table : decoded.tables )
		{
			CDBTableBase *loaded = NDatabase::GetTable( table.tableId );
			if ( !loaded || loaded->GetRecordCount() != table.intRows.size() )
			{
				std::fprintf( stderr, "table=%d source=%zu loaded=%zu\n",
					table.tableId, table.intRows.size(), loaded ? loaded->GetRecordCount() : 0 );
				return 6;
			}
			const auto idIt = std::find( table.intNames.begin(), table.intNames.end(), "ID" );
			if ( idIt == table.intNames.end() ) return 10;
			const std::size_t idColumn = idIt - table.intNames.begin();
			for ( const auto &row : table.intRows )
			{
				if ( idColumn >= row.size() ) return 11;
				const int id = row[idColumn];
				const CDBRecord *record = loaded->GetDBRecord( id );
				if ( !record || record->GetRecordID() != id )
				{
					std::fprintf( stderr, "table=%d record id=%d missing or mismatched\n",
						table.tableId, id );
					return 12;
				}
				++comparedIDs;
			}
			++comparedTables;
			if ( auditNonAscii )
			{
				for ( std::size_t column = 0; column < table.stringNames.size(); ++column )
				{
					std::size_t nonAsciiRows = 0;
					int firstID = 0;
					unsigned int firstCodePoint = 0;
					for ( std::size_t row = 0; row < table.stringRows.size(); ++row )
					{
						if ( column >= table.stringRows[row].size() ) return 13;
						for ( wchar_t character : table.stringRows[row][column] )
							if ( static_cast<unsigned int>(character) > 127 )
							{
								if ( !nonAsciiRows )
								{
									firstID = table.intRows[row][idColumn];
									firstCodePoint = static_cast<unsigned int>(character);
								}
								++nonAsciiRows;
								break;
							}
					}
					if ( nonAsciiRows )
						std::printf( "nonascii table=0x%08X field=%s rows=%zu first_id=%d first_codepoint=U+%04X\n",
							table.tableId, table.stringNames[column].c_str(), nonAsciiRows,
							firstID, firstCodePoint );
				}
			}
		}
		// Baseline UI IDText 2656 contains U+2018, which must import as the
		// CP1251 byte 0x91 rather than a truncated wide-character byte.
		auto *controls = NDatabase::GetTable<NDb::CUIControl>();
		const NDb::CUIControl *control = controls ? controls->GetRecord( 2656 ) : nullptr;
		if ( !control || control->szID.find( '\x91' ) == std::string::npos ) return 14;
		CDBTable<NDb::CMaterial> *materials = NDatabase::GetTable<NDb::CMaterial>();
		if ( !materials )
			return 3;
		CDBIterator<NDb::CMaterial> it( *materials );
		unsigned int count = 0;
		while ( it.MoveNext() )
			++count;
		unsigned int specChecks = 0;
		for ( const auto &table : decoded.tables )
		{
			if ( table.tableId != 9 )
				continue;
			const auto idIt = std::find( table.intNames.begin(), table.intNames.end(), "ID" );
			const auto specIt = std::find( table.floatNames.begin(), table.floatNames.end(), "SpecFactor" );
			if ( idIt == table.intNames.end() || specIt == table.floatNames.end() )
				return 7;
			const std::size_t idColumn = idIt - table.intNames.begin();
			const std::size_t specColumn = specIt - table.floatNames.begin();
			for ( std::size_t row = 0; row < table.intRows.size(); ++row )
			{
				if ( idColumn >= table.intRows[row].size() || specColumn >= table.floatRows[row].size() )
					return 8;
				const int id = table.intRows[row][idColumn];
				const NDb::CMaterial *material = materials->GetRecord( id );
				if ( !material || std::memcmp( &material->fSpecFactor,
					&table.floatRows[row][specColumn], sizeof(float) ) != 0 )
				{
					std::fprintf( stderr, "material=%d SpecFactor mismatch\n", id );
					return 9;
				}
				++specChecks;
			}
		}
		std::printf( "tables=%u record_ids=%zu materials=%u spec_checks=%u\n",
			comparedTables, comparedIDs, count, specChecks );
		return comparedTables == decoded.tables.size() && count && specChecks == count ? 0 : 4;
	}
	catch ( const std::exception &e )
	{
		std::fprintf( stderr, "database load failed: %s\n", e.what() );
		return 5;
	}
}
