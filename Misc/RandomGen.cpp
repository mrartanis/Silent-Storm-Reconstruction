#include "StdAfx.h"
#include "RandomGen.h"
#include "PortableRand.h"
#include "..\FileIO\basicChunk1.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Disabled in every normal run.  The harness sets this only before paired
// actions so lazily constructed SRand instances (including critical injury)
// do not pick different wall-clock ticks in x86 and x64 processes.
static bool g_bHarnessSeedActive = false;
static unsigned int g_nHarnessSeed = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////
SRandomSeed::SRandomSeed() : nSeed( g_bHarnessSeedActive ? g_nHarnessSeed : GetTickCount() )
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
SRandomSeed::SRandomSeed( int seed ) : nSeed( seed )
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
SRand::SRand() : seed( g_bHarnessSeedActive ? g_nHarnessSeed : GetTickCount() )
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
CRandomGenerator random;
////////////////////////////////////////////////////////////////////////////////////////////////////
const LPCSTR PSZ_MASK_TO_FIND_FILES = "C:\\*.*";
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
// --------------------------- FillRandRsl() ---------------------------------------------------------------
const int N_FROM_START = 1024;
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CRandomGenerator::RecFindFile( std::string &szFoundName, const char *pszBaseMask, int nToFind, int* pnTotFinded )
{
	WIN32_FIND_DATA ff;
	HANDLE hf = FindFirstFile( pszBaseMask, &ff );
	std::string szPath( pszBaseMask );
	szPath = szPath.substr( 0, szPath.length() - 3 );
	if ( hf != INVALID_HANDLE_VALUE )
	{
		for ( BOOL bCont = TRUE; bCont; bCont = FindNextFile( hf, &ff ) )
		{
			if ( ff.cFileName[0] == '.' || (ff.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)) != 0 )
				continue;
			if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if ( RecFindFile( szFoundName, (szPath + ff.cFileName + "\\*.*").c_str(), nToFind, pnTotFinded ) == TRUE )
					return TRUE;
				continue;
			}
			if ( *pnTotFinded >= nToFind )
			{
				if ( ff.nFileSizeLow >= N_FROM_START + sizeof( state.results ) )
				{
					szFoundName = szPath + ff.cFileName;
					return TRUE;
				}
				( *pnTotFinded )--;
			}
			( *pnTotFinded )++;
		}
		FindClose( hf );
	}
	return FALSE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// The purpose of this func is to fill randrsl[RANDSIZ] arrays
// with initial random values
// It's uses first RANDSIZ values from random file for this
void CRandomGenerator::FillRandRsl()
{
	srand( GetTickCount() );
	std::string szFoundName;
	BOOL bSuccess = FALSE;
	while ( !bSuccess )
	{
		int nTotFinded = 0;
		if ( !RecFindFile( szFoundName, PSZ_MASK_TO_FIND_FILES, rand() % 512 + 1, &nTotFinded ) )
		{
			int nToFind = rand() % ( nTotFinded - 1 ) + 1;
			nTotFinded = 0;
			if ( !RecFindFile( szFoundName, PSZ_MASK_TO_FIND_FILES, nToFind, &nTotFinded ) )
				continue;
		}
		bSuccess = TRUE;
		HANDLE hf = CreateFile( szFoundName.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0 );
		if ( hf != INVALID_HANDLE_VALUE )
		{
			SetFilePointer( hf, N_FROM_START - rand() % ( N_FROM_START - 512 ), 0, FILE_BEGIN );
			DWORD dwRes;
			if ( !ReadFile( hf, state.results, sizeof(state.results), &dwRes, 0 ) )
				bSuccess = FALSE;
			CloseHandle( hf );
		}
		BOOL bHaveNotZero = FALSE;
		for ( int i = 0; i < RANDSIZ; i++ )
			if ( state.results[i] )
			{
				bHaveNotZero = TRUE;
				break;
			}
		if ( bHaveNotZero == FALSE )
		{
			bSuccess = FALSE;
			Sleep( 10 );
		}
		for ( int i = 0; i < RANDSIZ; i++ )
			state.results[i] ^= rand();
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
