#include "StdAfx.h"
#include "GParticleFormat.h"
#include "Bound.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>
namespace NGScene
{
// Effect files store 32-bit offsets, not in-memory pointers. The old loader
// overlaid SParticle on the file and happened to work only with x86 pointers.
#pragma pack( push, 2 )
struct SKeyTrackDisk
{
	short nKeys;
	std::uint32_t keysOffset;
};
struct SParticleDisk
{
	short nTStart;
	short nTEnd;
	SKeyTrackDisk pos, rot, scale, color, sprite;
};
#pragma pack( pop )
static_assert( sizeof(SKeyTrackDisk) == 6, "effect track wire size" );
static_assert( sizeof(SParticleDisk) == 34, "effect particle wire size" );

template<class TValue>
static void ResolveTrack( TKeyTrack<TValue> *dst, const SKeyTrackDisk &src,
	char *data, size_t dataBytes )
{
	if ( src.nKeys < 0 || src.keysOffset > dataBytes ||
		static_cast<size_t>(src.nKeys) > (dataBytes - src.keysOffset) / sizeof(TKey<TValue>) )
		throw std::runtime_error( "invalid effect key track" );
	dst->nKeys = src.nKeys;
	dst->keys = src.nKeys ? reinterpret_cast<TKey<TValue>*>(data + src.keysOffset) : 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CParticlesInfo
////////////////////////////////////////////////////////////////////////////////////////////////////
void CParticlesInfo::CalcBound( SBound *pRes )
{
	CVec3 ptMin, ptMax;
	if ( nParticles )
	{
		float fMaxSize = 0;
		ptMin.x = ptMin.y = ptMin.z = 1e10f;
		ptMax.x = ptMax.y = ptMax.z = -1e10f;
		for ( int nP = 0; nP < nParticles; ++nP )
		{
			SParticle &part = particles[nP];
			if ( !part.pos.nKeys )
				continue;
			for ( int i = 0; i < part.pos.nKeys; ++i )
			{
				CVec3 pos = part.pos.keys[i].value;
				ptMin.Minimize( pos );
				ptMax.Maximize( pos );
			}
			for ( int i = 0; i < part.scale.nKeys; ++i )
			{
				CVec2 scale = part.scale.keys[i].value;
				fMaxSize = Max( fabs(scale.x), fMaxSize );
				fMaxSize = Max( fabs(scale.y), fMaxSize );
			}
		}
		fMaxSize *= (FP_SQRT_2 * 0.5f);
		CVec3 edge( fMaxSize, fMaxSize, fMaxSize );
		ptMin -= edge;
		ptMax += edge;
	}
	else
	{
		ptMin = VNULL3;
		ptMax = VNULL3;
	}
	pRes->BoxInit( ptMin, ptMax );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void Interpolate( const CVec3 &v1, const CVec3 &v2, float fAlpha, CVec3 *pRes )
{
	*pRes = v1 * (1 - fAlpha) + v2 * fAlpha;
}
void Interpolate( const CVec2 &v1, const CVec2 &v2, float fAlpha, CVec2 *pRes )
{
	*pRes = v1 * (1 - fAlpha) + v2 * fAlpha;
}
void Interpolate( const float &v1, const float &v2, float fAlpha, float *pRes )
{
	*pRes = v1 * (1 - fAlpha) + v2 * fAlpha;
}
void Interpolate( const DWORD &v1, const DWORD &v2, float fAlpha, DWORD *pRes )
{
	DWORD b = DWORD( (v1 & 0x000000FF) * (1 - fAlpha) + (v2 & 0x000000FF) * fAlpha );
	DWORD g = DWORD( (v1 & 0x0000FF00) * (1 - fAlpha) + (v2 & 0x0000FF00) * fAlpha );
	DWORD r = DWORD( (v1 & 0x00FF0000) * (1 - fAlpha) + (v2 & 0x00FF0000) * fAlpha );
	DWORD a = DWORD( (v1 & 0xFF000000) * (1 - fAlpha) + (v2 & 0xFF000000) * fAlpha );
	*pRes = b | (g & 0x0000FF00) | (r & 0x00FF0000) | (a & 0xFF000000);
}
void Interpolate( const short &v1, const short &v2, float fAlpha, short *pRes )
{
	*pRes = v1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CParticlesLoader
////////////////////////////////////////////////////////////////////////////////////////////////////
CFileRequest* CParticlesLoader::CreateRequest()
{
	return new CFileRequest( "Effects", GetKey() );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CParticlesLoader::RecalcValue( CFileRequest *pRequest )
{
	pValue = new CParticlesInfo;
	pValue->pData = pRequest;
	CMemoryStream *stream = pRequest->GetStream();
	if ( stream->GetSize() < 4 )
		throw std::runtime_error( "effect file too short" );
	char *pData = reinterpret_cast<char*>(stream->GetBufferForWrite());
	std::memcpy( &pValue->nBytes, pData, sizeof(int) );
	if ( pValue->nBytes < 12 || pValue->nBytes > stream->GetSize() - 4 )
		throw std::runtime_error( "invalid effect payload size" );
	pData += 4;
	std::memcpy( &pValue->fTEnd, pData, sizeof(float) );
	std::memcpy( &pValue->fFrameRate, pData + 4, sizeof(float) );
	std::memcpy( &pValue->nParticles, pData + 8, sizeof(int) );
	if ( pValue->nParticles < 0 ||
		static_cast<size_t>(pValue->nParticles) >
		(static_cast<size_t>(pValue->nBytes) - 12) / sizeof(SParticleDisk) )
		throw std::runtime_error( "invalid effect particle count" );
	const char *diskData = pData + 12;
	pValue->particleStorage.resize( pValue->nParticles );
	pValue->particles = pValue->particleStorage.empty() ? 0 : &pValue->particleStorage[0];

	for ( int nP = 0; nP < pValue->nParticles; ++nP )
	{
		SParticleDisk disk;
		std::memcpy( &disk, diskData + nP * sizeof(disk), sizeof(disk) );
		SParticle &particle = pValue->particles[nP];
		particle.nTStart = disk.nTStart;
		particle.nTEnd = disk.nTEnd;
		ResolveTrack( &particle.pos, disk.pos, pData, pValue->nBytes );
		ResolveTrack( &particle.rot, disk.rot, pData, pValue->nBytes );
		ResolveTrack( &particle.scale, disk.scale, pData, pValue->nBytes );
		ResolveTrack( &particle.color, disk.color, pData, pValue->nBytes );
		ResolveTrack( &particle.sprite, disk.sprite, pData, pValue->nBytes );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02541140, CParticlesLoader );
