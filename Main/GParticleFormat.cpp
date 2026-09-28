#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "GParticleFormat.h"
#include "Bound.h"
#include "../FileIO/PortableEffectData.h"
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace NGScene
{
template<class TValue, class TWireValue, class TConvert>
static void CopyTrack( TKeyTrack<TValue> *dst,
	std::vector<TParticleKey<TValue>> *owned,
	const std::vector<S2FileIO::EffectKey<TWireValue>> &source,
	TConvert convert )
{
	if ( source.size() > static_cast<size_t>((std::numeric_limits<short>::max)()) )
		throw std::runtime_error( "effect key count overflow" );
	owned->resize( source.size() );
	for ( size_t i = 0; i < source.size(); ++i )
	{
		(*owned)[i].nT = source[i].frame;
		(*owned)[i].value = convert( source[i].value );
	}
	dst->nKeys = static_cast<short>( source.size() );
	dst->keys = owned->empty() ? 0 : &(*owned)[0];
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
	CMemoryStream *stream = pRequest->GetStream();
	if ( stream->GetSize() < 0 )
		throw std::runtime_error( "invalid effect file size" );
	S2FileIO::EffectData effect;
	std::string error;
	if ( !S2FileIO::DecodeEffectData(
		reinterpret_cast<const std::uint8_t*>(stream->GetBufferForWrite()),
		static_cast<size_t>(stream->GetSize()), &effect, &error ) )
		throw std::runtime_error( error );
	if ( effect.payloadSize > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()) ||
		effect.particles.size() > static_cast<size_t>((std::numeric_limits<int>::max)()) )
		throw std::runtime_error( "effect size exceeds runtime limits" );
	pValue = new CParticlesInfo;
	pValue->pData = pRequest;
	pValue->nBytes = static_cast<int>(effect.payloadSize);
	pValue->fTEnd = effect.endTime;
	pValue->fFrameRate = effect.frameRate;
	pValue->nParticles = static_cast<int>(effect.particles.size());
	pValue->particleStorage.resize( effect.particles.size() );
	pValue->keyStorage.resize( effect.particles.size() );
	pValue->particles = pValue->particleStorage.empty() ? 0 : &pValue->particleStorage[0];

	for ( int nP = 0; nP < pValue->nParticles; ++nP )
	{
		const S2FileIO::EffectParticle &src = effect.particles[nP];
		SParticle &particle = pValue->particles[nP];
		SParticleKeyStorage &owned = pValue->keyStorage[nP];
		particle.nTStart = src.start;
		particle.nTEnd = src.end;
		CopyTrack( &particle.pos, &owned.pos, src.position,
			[]( S2FileIO::EffectVec3 v ) { return CVec3(v.x, v.y, v.z); } );
		CopyTrack( &particle.rot, &owned.rot, src.rotation,
			[]( float v ) { return v; } );
		CopyTrack( &particle.scale, &owned.scale, src.scale,
			[]( S2FileIO::EffectVec2 v ) { return CVec2(v.x, v.y); } );
		CopyTrack( &particle.color, &owned.color, src.color,
			[]( std::uint32_t v ) { return static_cast<DWORD>(v); } );
		CopyTrack( &particle.sprite, &owned.sprite, src.sprite,
			[]( std::int16_t v ) { return static_cast<short>(v); } );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02541140, CParticlesLoader );
