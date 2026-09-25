#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../Main/MapBuild.h"
#include "../DBFormat/DataConst.h"

#include <cstdio>

int main( int argc, char **argv )
{
	if ( argc != 2 )
		return 2;
	CFileStream database;
	database.OpenRead( argv[1] );
	NDatabase::Serialize( database, CStructureSaver::READ );
	auto *attributes = NDatabase::GetTable<NDb::CAttribute>();
	if ( !attributes )
		return 3;
	int day = 0, night = 0;
	CDBIterator<NDb::CAttribute> it( *attributes );
	while ( it.MoveNext() )
	{
		if ( it.Get()->szName == "Day" ) day = it.Get()->GetRecordID();
		if ( it.Get()->szName == "Night" ) night = it.Get()->GetRecordID();
	}
	if ( !day || !night )
		return 4;
	std::vector<int> flags;
	ConvertFlags( &flags, std::vector<std::string>{ "Night", "Day", "NotAnAttribute" }, false );
	if ( flags.size() != 2 || flags[0] != Min(day,night) || flags[1] != Max(day,night) )
		return 5;
	std::printf( "attributes=%zu day=%d night=%d flags=%d,%d\n",
		attributes->GetRecordCount(), day, night, flags[0], flags[1] );
	return 0;
}
