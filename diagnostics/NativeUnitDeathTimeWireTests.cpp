#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/wUnitServer.h"
#include "../FileIO/Streams.h"

#include <cstdint>
#include <cstdio>

int main()
{
	static_assert( sizeof(decltype(NWorld::CUnitServer::tDeathTime)) == 4,
		"retail death timestamp is a 32-bit game tick" );
	STime deathTime = UINT32_C(0x89abcdef);
	CMemoryStream stream;
	try {
		CStructureSaver saver( stream, CStructureSaver::WRITE );
		saver.Add( 35, &deathTime );
	} catch (...) { return 1; }
	std::uint64_t digest = UINT64_C(14695981039346656037);
	for ( int i = 0; i < stream.GetSize(); ++i ) {
		digest ^= stream.GetBuffer()[i];
		digest *= UINT64_C(1099511628211);
	}
	deathTime = 0;
	stream.SetRMode();
	stream.Seek( 0 );
	try {
		CStructureSaver saver( stream, CStructureSaver::READ );
		saver.Add( 35, &deathTime );
	} catch (...) { return 2; }
	if ( deathTime != UINT32_C(0x89abcdef) ) return 3;
	if ( stream.GetSize() != 18 || digest != UINT64_C(0xD132B87420B27244) ) return 4;
	std::printf( "unit death time=%08X wire=%d fnv=%016llX\n",
		static_cast<unsigned>( deathTime ), stream.GetSize(),
		static_cast<unsigned long long>( digest ) );
	return 0;
}
