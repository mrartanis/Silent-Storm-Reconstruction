#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiPosition.h"
#include "../Main/aiWaypoint.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../FileIO/Streams.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataMap.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>

namespace {
std::uint64_t digest = UINT64_C(14695981039346656037);
void Add( std::uint32_t value )
{
	for ( int i = 0; i < 4; ++i )
	{
		digest ^= static_cast<unsigned char>( value >> ( i * 8 ) );
		digest *= UINT64_C(1099511628211);
	}
}
}

int main( int argc, char **argv )
{
	if ( argc != 3 ) return 2;
	CFileStream database;
	database.OpenRead( argv[1] );
	NDatabase::Serialize( database, CStructureSaver::READ );
	const std::filesystem::path root( argv[2] );
	NGScene::AddResourceDir( root.string().c_str() );
	unsigned int decoded = 0, failed = 0, routes = 0, references = 0, missingNames = 0;
	const char *names[] = { "Units", "Groups" };
	for ( int package = 0; package != 2; ++package )
	{
		S2FileIO::PortablePackageIndex index;
		if ( !index.Open( ( root / ( std::string( names[package] ) + ".res" ) ).string() ) ) return 4;
		for ( const auto &entry : index.Entries() )
		{
			try
			{
				NGScene::CResourceOpener resource( names[package], entry.first );
				CObj<NAI::CUnitAIInfo> strict = new NAI::CUnitAIInfo;
				strict->operator&( *resource.operator->() );
				CObj<NAI::CUnitAIInfo> loaded;
				if ( package == 0 )
				{
					CObj<NAI::CUnitAIInfoLoader> loader = new NAI::CUnitAIInfoLoader;
					loader->SetKey( entry.first );
					loaded = loader->GetValue();
				}
				else
				{
					CObj<NAI::CUnitGroupAIInfoLoader> loader = new NAI::CUnitGroupAIInfoLoader;
					loader->SetKey( entry.first );
					loaded = loader->GetValue();
				}
				if ( !loaded || strict->routes.size() != loaded->routes.size() ) return 5;
				Add( package + 1 );
				Add( entry.first );
				Add( static_cast<std::uint32_t>( loaded->routes.size() ) );
				for ( std::size_t i = 0; i < loaded->routes.size(); ++i )
				{
					const auto &route = loaded->routes[i].waypoints;
					if ( route != strict->routes[i].waypoints ) return 6;
					Add( static_cast<std::uint32_t>( route.size() ) );
					for ( int id : route )
					{
						Add( id );
						++references;
						if ( !NDb::GetWaypointName( id ) )
						{
							std::fprintf( stderr, "unresolved waypoint name %s source=%d id=%d\n",
								names[package], entry.first, id );
							++missingNames;
						}
					}
				}
				routes += static_cast<unsigned int>( loaded->routes.size() );
				++decoded;
			}
			catch ( ... )
			{
				std::fprintf( stderr, "%s decode failed id=%d bytes=%u\n",
					names[package], entry.first, entry.second.length );
				++failed;
			}
		}
	}
	NGScene::CloseAllResources();
	std::printf( "ai_routes decoded=%u failed=%u routes=%u refs=%u missing_names=%u digest=%016llX\n",
		decoded, failed, routes, references, missingNames,
		static_cast<unsigned long long>( digest ) );
	return decoded == 211 && failed == 0 && routes == 211 &&
		references == 561 && missingNames == 3 &&
		digest == UINT64_C(0xF76E40DB1564FEE0) ? 0 : 7;
}
