#if defined(_WIN32)
#include "../FileIO/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#endif
#include "../FileIO/FilesPackage.h"
#include "../FileIO/PortablePackageIndex.h"

#include <cstdio>
#include <vector>

int main( int argc, char **argv )
{
	if ( argc != 2 )
		return 2;
	S2FileIO::PortablePackageIndex index;
	if ( !index.Open( argv[1] ) || index.Entries().empty() )
		return 3;
	CPtr<IFilesPackage> package = OpenFilesPackage( argv[1] );
	if ( !package.GetPtr() )
		return 4;
	std::size_t count = 0, bytes = 0;
	for ( const auto &entry : index.Entries() )
	{
		if ( !DoesFileExist( package, entry.first ) )
			return 5;
		CPackageStream original( package, entry.first );
		std::vector<unsigned char> gameBytes( original.GetSize() );
		if ( !gameBytes.empty() )
			original.Read( gameBytes.data(), static_cast<unsigned int>( gameBytes.size() ) );
		std::vector<std::uint8_t> decoded;
		if ( !index.Read( entry.first, &decoded ) || gameBytes != decoded )
			return 6;
		++count;
		bytes += gameBytes.size();
	}
	std::printf( "resources=%zu bytes=%zu\n", count, bytes );
	return 0;
}
