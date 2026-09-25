#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/Streams.h"
#include "../DBFormat/DataMap.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {
std::uint64_t digest = 14695981039346656037ULL;

void Add( std::uint32_t value )
{
	for ( int byte = 0; byte < 4; ++byte )
	{
		digest ^= static_cast<unsigned char>( value >> ( byte * 8 ) );
		digest *= 1099511628211ULL;
	}
}

template<class T> int ID( const CPtr<T> &pointer )
{
	return pointer.GetPtr() ? pointer->GetRecordID() : 0;
}

template<class T> void AddRecords( const std::vector<CPtr<T> > &records )
{
	std::vector<int> ids;
	for ( const auto &record : records )
	{
		if ( !record.GetPtr() )
			throw 1;
		ids.push_back( ID( record ) );
	}
	std::sort( ids.begin(), ids.end() );
	Add( static_cast<std::uint32_t>( ids.size() ) );
	for ( int id : ids )
		Add( static_cast<std::uint32_t>( id ) );
}
}

int main( int argc, char **argv )
{
	if ( argc != 2 )
	{
		std::fprintf( stderr, "usage: NativeMapDatabaseTests <game.db>\n" );
		return 2;
	}
	try
	{
		CFileStream database;
		database.OpenRead( argv[1] );
		NDatabase::Serialize( database, CStructureSaver::READ );
		auto *templates = NDatabase::GetTable<NDb::CTemplate>();
		auto *variants = NDatabase::GetTable<NDb::CTemplVariant>();
		if ( !templates || !variants || !templates->GetRecordCount() || !variants->GetRecordCount() )
			return 3;

		std::vector<int> variantIDs;
		CDBIterator<NDb::CTemplVariant> it( *variants );
		while ( it.MoveNext() )
			variantIDs.push_back( it.Get()->GetRecordID() );
		std::sort( variantIDs.begin(), variantIDs.end() );
		unsigned int rootVariants = 0;
		unsigned int rectangles = 0;
		unsigned int zeroSize = 0;
		for ( int id : variantIDs )
		{
			const NDb::CTemplVariant *variant = variants->GetRecord( id );
			const NDb::CTemplate *map = variant->pTemplate.GetPtr();
			if ( !map )
			{
				std::fprintf( stderr, "variant=%d has no template\n", id );
				return 4;
			}
			if ( map->nWidth <= 0 || map->nHeight <= 0 )
				++zeroSize;
			if ( std::find( map->variants.begin(), map->variants.end(), variant ) == map->variants.end() )
				return 5;
			if ( variant->nMinCutFloor > variant->nMaxCutFloor )
				return 6;
			Add( static_cast<std::uint32_t>( id ) );
			Add( static_cast<std::uint32_t>( map->GetRecordID() ) );
			Add( static_cast<std::uint32_t>( map->nWidth ) );
			Add( static_cast<std::uint32_t>( map->nHeight ) );
			Add( static_cast<std::uint32_t>( variant->bGrid ) );
			Add( static_cast<std::uint32_t>( variant->nBorder ) );
			Add( static_cast<std::uint32_t>( variant->nExitBorder ) );
			Add( static_cast<std::uint32_t>( variant->nMinCutFloor ) );
			Add( static_cast<std::uint32_t>( variant->nMaxCutFloor ) );
			Add( static_cast<std::uint32_t>( variant->bNoAttack ) );
			Add( static_cast<std::uint32_t>( variant->weatherType ) );
			Add( static_cast<std::uint32_t>( ID( variant->pScript ) ) );
			AddRecords( variant->rects );
			AddRecords( variant->pFinalElements );
			AddRecords( variant->pUnits );
			AddRecords( variant->explosions );
			AddRecords( variant->terrainSpots );
			AddRecords( variant->waypoints );
			if ( !variant->rects.empty() || !variant->pFinalElements.empty() || !variant->pUnits.empty() )
				++rootVariants;
			rectangles += static_cast<unsigned int>( variant->rects.size() );
		}
		if ( !rootVariants || !rectangles )
			return 7;
		std::printf( "templates=%zu variants=%zu populated=%u rectangles=%u zero_size=%u digest=%016llX\n",
			templates->GetRecordCount(), variantIDs.size(), rootVariants, rectangles, zeroSize,
			static_cast<unsigned long long>( digest ) );
		return 0;
	}
	catch ( ... )
	{
		std::fprintf( stderr, "map database load or relationship failed\n" );
		return 8;
	}
}
