#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GSceneUtils.h"

#include <cstdint>
#include <cstdio>

int main()
{
	if ( !pSSClasses ) return 1;
	const int id = pSSClasses->GetTypeID( static_cast<NGScene::CCInt*>(0) );
	if ( id != 0x03031620 ) return 2;
	CMemoryStream stream;
	CObj<NGScene::CCInt> node = new NGScene::CCInt( 42 );
	try
	{
		CStructureSaver saver( stream, CStructureSaver::WRITE );
		saver.Add( 1, &node );
	}
	catch ( ... ) { return 3; }
	std::uint64_t digest = UINT64_C(14695981039346656037);
	for ( int i = 0; i < stream.GetSize(); ++i )
	{
		digest ^= stream.GetBuffer()[i];
		digest *= UINT64_C(1099511628211);
	}
	node = 0;
	stream.SetRMode();
	stream.Seek( 0 );
	try
	{
		CStructureSaver saver( stream, CStructureSaver::READ );
		saver.Add( 1, &node );
	}
	catch ( ... ) { return 4; }
	if ( !node || node->GetValue() != 42 ) return 5;
	std::printf( "CCInt class id=%08X value=%d bytes=%d fnv=%016llX\n",
		id, node->GetValue(), stream.GetSize(),
		static_cast<unsigned long long>( digest ) );
	return 0;
}
