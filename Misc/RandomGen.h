#ifndef __RANDOM_GEN_H__
#define __RANDOM_GEN_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "PortableIsaac.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
struct SRandomSeed
{
	int nSeed;
	SRandomSeed();
	SRandomSeed( int seed );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SRand
{
	SRandomSeed seed;
	
	SRand();
	SRand( const SRandomSeed &rseed ) : seed( rseed ) {}
	int Get( int nMax );
	float GetFloat( float fpMin, float fpMax );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CRoulette
{
	std::vector<float> m_aSectors;
public:
	CRoulette() : m_aSectors( 1, 0 ) {}
	void AddSector( float fValue );
	int GetRandomSector( SRand *pRand ) const;
	float GetSectorValue( int nIndex ) const { return m_aSectors[ nIndex + 1 ] - m_aSectors[ nIndex ]; }
	int operator&( CStructureSaver &f );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 	Random generator class header
	Uses ISAAC random generator, (c) Bob Jenkins with
	with using random file for initial random seeds

	Written by Ivanov Evgeny
	Nival Interactive, 1998
*/
////////////////////////////////////////////////////////////////////////////////////////////////////
const int RANDSIZL = 8;
const int RANDSIZ = 1 << RANDSIZL;
////////////////////////////////////////////////////////////////////////////////////////////////////
class CRandomGenerator
{
static_assert( sizeof(unsigned int) == sizeof(std::uint32_t), "game ISAAC words must be 32-bit" );
static_assert( RANDSIZ == S2Random::kIsaacSize, "game and portable ISAAC buffer sizes differ" );
private:
	S2Random::IsaacState state;
public:
	CRandomGenerator() { Init(); }
	void SeedForHarness( unsigned int seed ); // explicit deterministic state for paired architecture tests only
	unsigned int Get();
	unsigned int Get( unsigned int nMax ) { ASSERT( nMax != 0 ); return Get() % nMax; }
	unsigned int Get( unsigned int nMin, unsigned int nMax )	{ return Get( nMax-nMin ) + nMin; }
	float GetFloat( float fpMin, float fpMax );
	bool Check( int nCheck, int nRange = 100 ) { return (int)Get(nRange) < nCheck; }
	bool NegCheck( int nCheck, int nRange = 100 ) { return (int)Get(nRange) >= nCheck; }
private:
	void Init();  // very slow operation
	void InitState();
	void FillRandRsl();
	BOOL RecFindFile( std::string &szFoundName, const char *pszBaseDir, int nToFind, int* pnTotFinded );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
inline unsigned int CRandomGenerator::Get()
{
	return S2Random::IsaacNext( &state );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CRandomGenerator::GetFloat( float fpMin, float fpMax )
{
	ASSERT( fpMin <= fpMax );
	return fpMin + float( Get() * ( ( fpMax - fpMin ) * (1/double(0xFFFFFFFF)) ) );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline float SRand::GetFloat( float fpMin, float fpMax )
{
	ASSERT( fpMin <= fpMax );
	return fpMin + float( Get( 0xFFFF ) * ( ( fpMax - fpMin ) * (1/double(0xFFFF)) ) );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
extern CRandomGenerator random;
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
