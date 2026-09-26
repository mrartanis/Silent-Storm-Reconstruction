#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiGrid.h"
#include "../Main/wHeightLayers.h"

#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

int main()
{
	// GetFlipper appends and may reallocate before it assigns the door state.
	// Moving an uninitialized bool there is UB in a real mission grid build.
	std::vector<NAI::CPathNetwork::SFlipper> flippers;
	flippers.emplace_back();
	flippers.emplace_back();
	for ( const auto &flipper : flippers )
		if ( flipper.nFlipper != 0 || flipper.bOpen || flipper.nFixedFlags != 0 )
			return 5;
	CObj<NAI::CPathNetwork> network = new NAI::CPathNetwork;
	const int layerIndex = network->CreateLayer( 4, 4, CVec2( 0, 0 ), 0.0f, 0 );
	if ( layerIndex != 0 || network->GetLayers().size() != 1 )
		return 1;
	NAI::CNodesLayer *layer = network->GetLayer( 0 );
	layer->tiles[2][2].nFloor = 0;
	layer->tiles[2][2].nHeight = 1000;

	CObj<NWorld::CHeightLayers> heights = new NWorld::CHeightLayers;
	heights->ComputeLayers( 4, 4, 0, network );
	NWorld::SHLayer *floor = heights->GetLayer( 0 );
	if ( heights->HasTerrain() || floor == heights->GetTerrainLayer() )
		return 2;
	float maximum = 0;
	std::uint64_t fieldHash = 14695981039346656037ULL;
	for ( int y = 0; y < floor->heights.GetYSize(); ++y )
		for ( int x = 0; x < floor->heights.GetXSize(); ++x )
		{
			const float height = floor->heights[y][x];
			maximum = Max( maximum, height );
			std::uint32_t bits = 0;
			std::memcpy( &bits, &height, sizeof(bits) );
			for ( int byte = 0; byte < 4; ++byte )
			{
				fieldHash ^= static_cast<unsigned char>( bits >> ( byte * 8 ) );
				fieldHash *= 1099511628211ULL;
			}
		}
	if ( maximum <= 0.0f || maximum > NAI::GetFHeight( 1000 ) )
		return 3;
	// The recovered x86 build and Windows/Linux x64/ARM64 all produce this
	// complete 5x5 post-spline field, not just the same rounded maximum.
	if ( std::fabs( maximum - 0.27720881f ) > 1e-7f ||
		fieldHash != 0xCC09769B5563B4C2ULL )
		return 4;
	std::printf( "height-network maximum=%.8f field=%016llX tiles=%d\n", maximum,
		static_cast<unsigned long long>(fieldHash),
		layer->tiles.GetXSize() * layer->tiles.GetYSize() );
	return 0;
}
