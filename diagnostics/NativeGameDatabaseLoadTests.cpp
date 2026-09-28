#include "../FileIO/StdAfx.h"
#include "../FileIO/Streams.h"
#include "../ADOImport/BasicDB.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataAI.h"
#include "../DBFormat/DataInterface.h"
#include "../DBFormat/DataLight.h"
#include "../FileIO/PortableGameDatabase.h"

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <exception>
#include <stdexcept>

namespace {
bool ReadIntField(const S2FileIO::GameDatabaseTable& table, std::size_t row,
                  const char* field, int* value) {
  const auto it = std::find(table.intNames.begin(), table.intNames.end(), field);
  if (it == table.intNames.end() || row >= table.intRows.size()) return false;
  const std::size_t column = it - table.intNames.begin();
  if (column >= table.intRows[row].size()) return false;
  *value = table.intRows[row][column];
  return true;
}

CVec3 DecodeColorWord(int value) {
  const std::uint32_t bits = static_cast<std::uint32_t>(value);
  return CVec3((bits & 0xffu) / 255.0f,
               ((bits >> 8) & 0xffu) / 255.0f,
               ((bits >> 16) & 0xffu) / 255.0f);
}

bool SameColor(const CVec3& actual, const CVec3& expected) {
  return actual.x == expected.x && actual.y == expected.y &&
         actual.z == expected.z;
}
}

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
		std::size_t textureColorChecks = 0, uiTypeChecks = 0, ambientColorChecks = 0;
		auto *textures = NDatabase::GetTable<NDb::CTexture>();
		auto *controls = NDatabase::GetTable<NDb::CUIControl>();
		auto *ambientLights = NDatabase::GetTable<NDb::CAmbientLight>();
		if (!textures || !controls || !ambientLights) return 24;
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
			if (table.tableId == 3 || table.tableId == 29 || table.tableId == 43)
			{
				for (std::size_t row = 0; row < table.intRows.size(); ++row)
				{
					int id = 0, value = 0;
					if (!ReadIntField(table, row, "ID", &id)) return 17;
					if (table.tableId == 3)
					{
						const auto *texture = textures->GetRecord(id);
						if (!texture || !ReadIntField(table, row, "AverageColor", &value) ||
							texture->dwAverageColor != static_cast<DWORD>(value)) return 18;
						++textureColorChecks;
					}
					else if (table.tableId == 43)
					{
						const auto *control = controls->GetRecord(id);
						if (!control || !ReadIntField(table, row, "Type", &value) ||
							static_cast<int>(control->type) != value) return 19;
						++uiTypeChecks;
					}
					else
					{
						const auto *light = ambientLights->GetRecord(id);
						if (!light) return 20;
						struct ColorField { const char *name; const CVec3 *actual; };
						const ColorField colors[] = {
							{"AmbientColor", &light->vAmbientColor},
							{"GlossColor", &light->vGlossColor},
							{"FogColor", &light->vFogColor},
							{"VapourColor", &light->vVapourColor},
							{"BackLightColor", &light->vBackColor},
							{"GroundAmbientColor", &light->vGroundAmbientColor},
							{"ShadowColor", &light->vShadowColor},
						};
						for (const auto& color : colors)
						{
							if (!ReadIntField(table, row, color.name, &value) ||
								!SameColor(*color.actual, DecodeColorWord(value))) return 21;
							++ambientColorChecks;
						}
						int ambient = 0, direct = 0;
						if (!ReadIntField(table, row, "AmbientColor", &ambient) ||
							!ReadIntField(table, row, "LightColor", &direct)) return 22;
						CVec3 lightColor = DecodeColorWord(direct) - DecodeColorWord(ambient);
						lightColor.Maximize(VNULL3);
						if (!SameColor(light->vLightColor, lightColor)) return 23;
						++ambientColorChecks;
					}
				}
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
		std::printf( "tables=%u record_ids=%zu materials=%u spec_checks=%u texture_colors=%zu ui_types=%zu ambient_colors=%zu\n",
			comparedTables, comparedIDs, count, specChecks,
			textureColorChecks, uiTypeChecks, ambientColorChecks );
		return comparedTables == decoded.tables.size() && count && specChecks == count &&
			textureColorChecks && uiTypeChecks && ambientColorChecks ? 0 : 4;
	}
	catch ( const std::exception &e )
	{
		std::fprintf( stderr, "database load failed: %s\n", e.what() );
		return 5;
	}
}
