#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiActionPlaceSource.h"
#include "../FileIO/Streams.h"

#include <cstdint>
#include <cstdio>

int main()
{
	static_assert( sizeof(NAI::CUnitAreaPlaceKey) == 4, "retail area keys are 32-bit" );
	const NAI::SPathPlace from( 0x55, 0xaa, 3, 0, NAI::WALK, 0 );
	const NAI::SPathPlace differentDirection( 0x55, 0xaa, 3, 7, NAI::WALK, 1 );
	const NAI::SPathPlace differentPose( 0x55, 0xaa, 3, 0, NAI::CROUCH, 0 );
	const NAI::CUnitAreaPlaceKey key = NAI::UnitAreaPlaceKey( from );
	if ( key != UINT32_C(0x0203aa55) ||
		 NAI::UnitAreaPlaceKey( differentDirection ) != key ||
		 NAI::UnitAreaPlaceKey( differentPose ) == key )
		return 1;

	std::unordered_map<NAI::CUnitAreaPlaceKey, int> places;
	places[key] = 7;
	CMemoryStream stream;
	try {
		CStructureSaver saver( stream, CStructureSaver::WRITE );
		saver.Add( 6, &places );
	} catch (...) { return 2; }
	std::uint64_t digest = UINT64_C(14695981039346656037);
	for ( int i = 0; i < stream.GetSize(); ++i ) {
		digest ^= stream.GetBuffer()[i];
		digest *= UINT64_C(1099511628211);
	}
	places.clear();
	stream.SetRMode();
	stream.Seek( 0 );
	try {
		CStructureSaver saver( stream, CStructureSaver::READ );
		saver.Add( 6, &places );
	} catch (...) { return 3; }
	if ( places.size() != 1 || places[key] != 7 ) return 4;
	if ( stream.GetSize() != 26 || digest != UINT64_C(0xA9A0A029C7FBC717) ) return 5;
	std::printf( "unit-area key=%08X wire=%d fnv=%016llX\n",
		static_cast<unsigned>( key ), stream.GetSize(),
		static_cast<unsigned long long>( digest ) );
	return 0;
}
