#if defined(_WIN32)
#include "../FileIO/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#endif
#include "../FileIO/FilesPackage.h"
#include "../FileIO/PortablePackageIndex.h"

#include <cstdio>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
	std::uint64_t digest = UINT64_C(14695981039346656037);
	void AddByte( std::uint8_t byte )
	{
		digest ^= byte;
		digest *= UINT64_C(1099511628211);
	}
	void AddWord( std::uint32_t value )
	{
		for ( int i = 0; i < 4; ++i ) AddByte( static_cast<std::uint8_t>( value >> ( i * 8 ) ) );
	}
	bool ComparePackage( const std::filesystem::path &path, std::size_t *pCount,
		std::uint64_t *pBytes )
	{
		S2FileIO::PortablePackageIndex index;
		const std::string filename = path.string();
		if ( !index.Open( filename ) || index.Entries().empty() ) return false;
		CPtr<IFilesPackage> package = OpenFilesPackage( filename.c_str() );
		if ( !package.GetPtr() ) return false;
		const std::string name = path.filename().string();
		for ( unsigned char ch : name ) AddByte( ch );
		AddByte( 0 );
		std::vector<int> ids;
		ids.reserve( index.Entries().size() );
		for ( const auto &entry : index.Entries() ) ids.push_back( entry.first );
		std::sort( ids.begin(), ids.end() );
		AddWord( static_cast<std::uint32_t>( ids.size() ) );
		for ( int id : ids )
		{
			if ( !DoesFileExist( package, id ) )
			{
				std::fprintf( stderr, "%s: missing game resource %d\n", name.c_str(), id );
				return false;
			}
			CPackageStream original( package, id );
			std::vector<unsigned char> gameBytes( original.GetSize() );
			if ( !gameBytes.empty() )
				original.Read( gameBytes.data(), static_cast<unsigned int>( gameBytes.size() ) );
			std::vector<std::uint8_t> decoded;
			if ( !index.Read( id, &decoded ) || gameBytes != decoded )
			{
				std::fprintf( stderr, "%s: resource %d differs\n", name.c_str(), id );
				return false;
			}
			AddWord( static_cast<std::uint32_t>( id ) );
			AddWord( static_cast<std::uint32_t>( gameBytes.size() ) );
			for ( unsigned char byte : gameBytes ) AddByte( byte );
			++*pCount;
			*pBytes += gameBytes.size();
		}
		return true;
	}
}

int main( int argc, char **argv )
{
	if ( argc != 2 && ( argc != 3 || std::strcmp( argv[1], "--corpus" ) ) )
		return 2;
	std::size_t count = 0;
	std::uint64_t bytes = 0;
	if ( argc == 2 )
	{
		if ( !ComparePackage( argv[1], &count, &bytes ) ) return 3;
		std::printf( "resources=%zu bytes=%llu\n", count,
			static_cast<unsigned long long>( bytes ) );
		return 0;
	}
	static const char *names[] = {
		"AIBinds.res", "AIBSPTrees.res", "AIGeometries.res", "Animations.res",
		"Binds.res", "Buildings.res", "Chapters.res", "Effects.res",
		"Fonts.res", "Geometries.res", "Globals.res", "Groups.res",
		"Heads.res", "Lights.res", "Locators.res", "LRTextures.res",
		"Sequences.res", "Skeletons.res", "Sounds.res", "Terrain.res",
		"Textures.res", "Units.res", "Waypoints.res"
	};
	for ( const char *name : names )
	{
		if ( !ComparePackage( std::filesystem::path(argv[2]) / name, &count, &bytes ) )
		{
			std::fprintf( stderr, "resource package failed: %s\n", name );
			return 4;
		}
	}
	std::printf( "packages=%zu resources=%zu bytes=%llu fnv=%016llX\n",
		sizeof(names) / sizeof(names[0]), count,
		static_cast<unsigned long long>( bytes ),
		static_cast<unsigned long long>( digest ) );
	return count == 57880 && bytes == UINT64_C(2200910768) &&
		digest == UINT64_C(0xB90BEA736E6BF987) ? 0 : 5;
}
