#include "StdAfx.h"
#include "RandomGen.h"
#include "PortableRand.h"
#include "PortableIsaacSeed.h"
#include "PortableClockSeed.h"
#include "../FileIO/BasicChunk1.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Disabled in every normal run.  The harness sets this only before paired
// actions so lazily constructed SRand instances (including critical injury)
// do not pick different wall-clock ticks in x86 and x64 processes.
static bool g_bHarnessSeedActive = false;
static unsigned int g_nHarnessSeed = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////
SRandomSeed::SRandomSeed() : nSeed( g_bHarnessSeedActive ? S2Random::SeedFromBits( g_nHarnessSeed ) : S2Random::ClockSeed32() )
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
SRandomSeed::SRandomSeed( int seed ) : nSeed( seed )
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
SRand::SRand() : seed( g_bHarnessSeedActive ? S2Random::SeedFromBits( g_nHarnessSeed ) : S2Random::ClockSeed32() )
{
	Get( 4 );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int SRand::Get( int nMax )
{
	static_assert( sizeof(int) == sizeof(std::int32_t), "SRand seed is a 32-bit game value" );
	return S2Random::Next( &seed.nSeed, nMax );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CRoulette
////////////////////////////////////////////////////////////////////////////////////////////////////
int CRoulette::GetRandomSector( SRand *pRand ) const
{
	ASSERT( pRand );
	if ( m_aSectors.back() == 0 )
		return 0;
	float fValue = pRand->GetFloat( 0.0f, m_aSectors.back() );
	int nBegin = 0;
	int nEnd = m_aSectors.size() - 1;
	while ( nBegin + 1 < nEnd )
	{
		int nCenter = nBegin + ( nEnd - nBegin ) / 2;
		if ( fValue < m_aSectors[nCenter] )
			nEnd = nCenter;
		else
			nBegin = nCenter;
	}
	return nBegin;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CRoulette::AddSector( float fValue )
{
	m_aSectors.push_back( m_aSectors.back() + fValue );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int CRoulette::operator&( CStructureSaver &f )
{
	f.Add( 1, &m_aSectors );
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
CRandomGenerator random;
#else
CRandomGenerator s2_game_random;
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////
void CRandomGenerator::Init()
{
	FillRandRsl();
	InitState();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CRandomGenerator::SeedForHarness( unsigned int seed )
{
	// The normal initializer samples a random host file and clock.  Paired x86/x64
	// tests instead need identical ISAAC input immediately before the tested action.
	g_bHarnessSeedActive = true;
	g_nHarnessSeed = seed;
	S2Random::IsaacSeedHarness( &state, seed );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CRandomGenerator::InitState()
{
	S2Random::IsaacInitialize( &state );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Only the initial entropy source changes: the historical C:\ drive scan
// cannot run on other platforms and could retry indefinitely. ISAAC's
// deterministic initialization, output order and saved state are unchanged.
void CRandomGenerator::FillRandRsl()
{
	S2Random::FillIsaacSeedFromSystem( &state );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
