#include "../FileIO/StdAfx.h"
#include "../FileIO/Streams.h"
#include "../ADOImport/BasicDB.h"
#include "../DBFormat/DataFormat.h"
#include "../FileIO/PortableGameDatabase.h"

#include <cstdio>
#include <exception>
#include <stdexcept>

int main( int argc, char **argv )
{
	if ( argc != 2 )
	{
		std::fprintf( stderr, "usage: NativeGameDatabaseLoadTests <game.db>\n" );
		return 2;
	}
	try
	{
		CFileStream database;
		database.OpenRead( argv[1] );
		NDatabase::Serialize( database, CStructureSaver::READ );
		S2FileIO::PortableGameDatabase decoded;
		std::string error;
		if ( !S2FileIO::LoadPortableGameDatabase( argv[1], &decoded, &error ) )
			throw std::runtime_error( error );
		unsigned int comparedTables = 0;
		for ( const auto &table : decoded.tables )
		{
			CDBTableBase *loaded = NDatabase::GetTable( table.tableId );
			if ( !loaded || loaded->GetRecordCount() != table.intRows.size() )
			{
				std::fprintf( stderr, "table=%d source=%zu loaded=%zu\n",
					table.tableId, table.intRows.size(), loaded ? loaded->GetRecordCount() : 0 );
				return 6;
			}
			++comparedTables;
		}
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
		std::printf( "tables=%u materials=%u spec_checks=%u\n",
			comparedTables, count, specChecks );
		return comparedTables == decoded.tables.size() && count && specChecks == count ? 0 : 4;
	}
	catch ( const std::exception &e )
	{
		std::fprintf( stderr, "database load failed: %s\n", e.what() );
		return 5;
	}
}
