#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/BuildingGrid.h"
#include "../Main/BuildingSchema.h"

#include <cstdint>
#include <cstdio>

int main()
{
	const NBuilding::CIVec3 signedJunction( 4, -2, -4 );
	const CTPoint<int> signedGround( 4, -2 );
	if ( std::uint32_t( NBuilding::SJunctionHash()( signedJunction ) ) !=
			UINT32_C( 0xFFFE0004 ) ||
		std::uint32_t( NBuilding::SGroundHash()( signedGround ) ) !=
			UINT32_C( 0xFFFE0004 ) )
		return 9;
	CObj<NBuilding::CBuildingGrid> grid = new NBuilding::CBuildingGrid;
	grid->Setup( 2, 2, 0, 0, CVec2( 0, 0 ) );
	const NBuilding::SPoint3 target( 2, 2, 0 );
	grid->AddHP( target, 100 );
	if ( grid->GetHP( target ) != 100 || grid->IsDestroyed( target ) )
		return 1;
	if ( grid->DamageSpot( target, 40, true ) || grid->GetHP( target ) != 60 )
		return 2;
	if ( !grid->DamageSpot( target, 60, true ) || !grid->IsDestroyed( target ) )
		return 3;
	std::vector<NBuilding::SPoint3> broken;
	grid->GetBrokenSpots( &broken );
	if ( broken.size() != 1 || broken[0].x != 2 || broken[0].y != 2 || broken[0].z != 0 )
		return 4;
	std::vector<NBuilding::SPart> updated;
	grid->GetUpdatedParts( &updated );
	if ( updated.size() != 8 )
		return 5;
	const NBuilding::SPoint3 cellar( 1, 1, 0 );
	grid->SetCellar( cellar );
	if ( !grid->IsCellar( cellar ) || grid->DamageSpot( cellar, 255 ) )
		return 6;
	const NBuilding::SPoint3 permanent( 3, 3, 0 );
	grid->SetIndestructible( permanent );
	if ( grid->DamageSpot( permanent, 255 ) || grid->GetHP( permanent ) != 255 )
		return 7;
	const int cellarHp = grid->GetHP( cellar );
	const int permanentHp = grid->GetHP( permanent );
	grid->ToggleStability();
	grid->Reset();
	SFBTransform transform;
	Identity( &transform.forward );
	Identity( &transform.backward );
	grid->Explode( transform, CVec3( 1, 1, 0.5f ), 255, 2.0f );
	std::uint64_t hash = UINT64_C(14695981039346656037);
	int destroyed = 0;
	for ( int z = 0; z < 5; ++z )
		for ( int y = 0; y < 6; ++y )
			for ( int x = 0; x < 6; ++x )
			{
				const int hp = grid->GetHP( NBuilding::SPoint3( x, y, z ) );
				destroyed += hp == 0;
				hash = (hash ^ static_cast<std::uint8_t>(hp)) * UINT64_C(1099511628211);
			}
	if ( destroyed == 0 )
		return 8;
	std::printf( "broken=%zu updated=%zu cellar=%d permanent=%d explosion_destroyed=%d hash=%016llX\n",
		broken.size(), updated.size(), cellarHp, permanentHp,
		destroyed, static_cast<unsigned long long>(hash) );
	return 0;
}
