#include "StdAfx.h"
#include "GfxEffects.h"
#include "GfxBuffers.h"
#include "GfxRender.h"
#include "GfxUtils.h"
#include "GfxShaders.h"
#include "Transform.h"
#include "TextureSampling.h"
namespace NGfx
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// universal rects buffer
////////////////////////////////////////////////////////////////////////////////////////////////////
static vector<STriangle> universalRectsBuffer;
////////////////////////////////////////////////////////////////////////////////////////////////////
static void RefreshUniversalRectTrisBuffer()
{
	if ( !universalRectsBuffer.empty() )
		return;
	universalRectsBuffer.resize( N_MAX_RECTANGLES * 2 );
	for ( int i = 0; i < N_MAX_RECTANGLES; ++i )
	{
		WORD wStart = i * 4;
		universalRectsBuffer[i*2  ] = STriangle( wStart + 0, wStart + 1, wStart + 2 );
		universalRectsBuffer[i*2+1] = STriangle( wStart + 0, wStart + 2, wStart + 3 );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static CObj<CTexture> pWhiteTexture;
static CTexture* GetWhiteTexture()
{
	if ( IsValid( pWhiteTexture ) )
		return pWhiteTexture;
	pWhiteTexture = MakeTexture( 3, 3, 1, SPixel8888::ID, TEXTURE_2D, CLAMP );
	CTextureLock<SPixel8888> lock( pWhiteTexture, 0, NGfx::INPLACE );
	for ( int y = 0; y < lock.GetYSize(); ++y )
	{
		for ( int x = 0; x < lock.GetXSize(); ++x )
			lock[y][x] = SPixel8888( 255, 255, 255, 255 );
	}
	return pWhiteTexture;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void MakeQuadTriList( int nRects, STriangleList *pRes )
{
	ASSERT( nRects <= N_MAX_RECTANGLES );
	RefreshUniversalRectTrisBuffer();
	*pRes = STriangleList( &universalRectsBuffer[0], Min( nRects, N_MAX_RECTANGLES ) * 2, 0 );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// C2DQuadsRenderer
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef SGeomVecFull SRectVertex;
struct SFakeCPPInitOrder
{
	CObj<CGeometry> pGeom;
};
const int N_RECTS_PER_BUF = 128;
struct S2DRectInfoLock : public SFakeCPPInitOrder
{
	int nRects;

	virtual ~S2DRectInfoLock() {}
	bool IsFull() const { return nRects == N_RECTS_PER_BUF; }
	int GetRectsNum() const { return nRects; }
};
template<class T>
struct SRealRectInfoLock : public S2DRectInfoLock
{
	CBufferLock<T> data;

	SRealRectInfoLock(): data(&pGeom, N_RECTS_PER_BUF * 4) { nRects = 0; ASSERT( !IsBadWritePtr( &data[0], 4 ) ); }
	T* AddRect() { return &data[ (nRects++) * 4 ];}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
C2DQuadsRenderer::C2DQuadsRenderer( const NGfx::CRenderContext &_rc, const CVec2 &vSize, int _dm )
: dm(_dm), rc(_rc), pLock(0)
{
	SetupRC( vSize );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void C2DQuadsRenderer::SetTarget( const NGfx::CRenderContext &_rc, const CVec2 &vSize, int _dm )
{
	ASSERT( pLock == 0 );
	Flush();
	dm = _dm;
	rc = _rc;
	SetupRC( vSize );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void C2DQuadsRenderer::SetTarget( NGfx::CTexture *pTarget, const CVec2 &vSize, int _dm )
{
	ASSERT( pLock == 0 );
	Flush();
	dm = _dm;
	rc.SetAlphaCombine( NGfx::COMBINE_NONE );
	rc.SetStencil( NGfx::STENCIL_NONE );
	if ( pTarget )
		rc.SetTextureRT( pTarget );
	SetupRC( vSize );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void C2DQuadsRenderer::SetTarget( const CVec2 &vSize, int _dm )
{
	ASSERT( pLock == 0 );
	Flush();
	dm = _dm;
	rc.SetAlphaCombine( NGfx::COMBINE_NONE );
	rc.SetStencil( NGfx::STENCIL_NONE );
	SetupRC( vSize );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void C2DQuadsRenderer::SetupRC( const CVec2 &vSize )
{
	CTransformStack ts;
	ts.MakeDirect( vSize );
	rc.SetTransform( ts.Get() );

	switch ( dm & QRM_STENCIL_MASK )
	{
		case QRM_KEEP_STENCIL: break;
		case QRM_TEST_STENCIL: rc.SetStencil( NGfx::STENCIL_TEST, 0x80, 0x80 ); break;
		default: ASSERT(0); break;
	}
	switch ( dm & QRM_DEPTH_MASK )
	{
		case QRM_DEPTH_NONE: rc.SetDepth( NGfx::DEPTH_NONE ); break;
		case QRM_OVERWRITE: rc.SetDepth( NGfx::DEPTH_OVERWRITE ); break;
		default: ASSERT(0); break;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
S2DRectInfoLock* C2DQuadsRenderer::GetRectInfoLock( CTexture *pContainer, const STexturePlaceInfo &region, float uvPackingSteps )
{
	if ( pContainer != pPrevContainer || ( pLock && ( pLock->IsFull() || fUVMult != 1.0f / uvPackingSteps ) ) )
		Flush();
	if ( pLock == 0 )
	{
		if ( IsTnLDevice() )
		{
			fUVMult = 1;
			pLock = new SRealRectInfoLock<SGeomVecT1C1>();
		}
		else
		{
			fUVMult = 1.0f / uvPackingSteps;
			pLock = new SRealRectInfoLock<SRectVertex>();
		}
		pPrevContainer = pContainer;
	}
	const bool unnormalizedNP2 = pContainer && NGfx::IsNVidiaNP2Bug() && !region.IsHolderAPow2Texture();
	fScaleU = S2TextureSampling::PackedUVScale( uvPackingSteps, pContainer ? region.size.x : 0, unnormalizedNP2 );
	fScaleV = S2TextureSampling::PackedUVScale( uvPackingSteps, pContainer ? region.size.y : 0, unnormalizedNP2 );
	return pLock;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void FillVertex( SRectVertex *pRes, float x, float y, float u, float v, DWORD dwColor, float fZ, float uvPackingSteps )
{
	pRes->pos.x = x;
	pRes->pos.y = y;
	pRes->pos.z = fZ;
	pRes->normal.dw = 0;
	const int packedU = Float2Int( u * uvPackingSteps ), packedV = Float2Int( v * uvPackingSteps );
	ASSERT( packedU >= -32768 && packedU <= 32767 && packedV >= -32768 && packedV <= 32767 );
	pRes->tex.nU = packedU;
	pRes->tex.nV = packedV;
	//CalcTexCoords( &pRes->tex, u, v );
	pRes->texLM.dw = 0;
	pRes->texU.dw = dwColor;
	pRes->texV.dw = 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void FillVertex( SGeomVecT1C1 *pRes, float x, float y, float u, float v, DWORD dwColor, float fScaleU, float fScaleV, float fZ )
{
	pRes->pos.x = x;
	pRes->pos.y = y;
	pRes->pos.z = fZ;
	pRes->tex.x = u * fScaleU;
	pRes->tex.y = v * fScaleV;
	pRes->color.color = dwColor;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void C2DQuadsRenderer::AddRect( const CTRect<float> &_rTarget, NGfx::CTexture *pTex, const CTRect<float> &_rSrc,
	SPixel8888 color, float fZ, bool logicalSource )
{
	float fXAdd = 0, fYAdd = 0;
	STexturePlaceInfo region;
	CTexture *pContainer = 0;
	if ( ( dm & QRM_RENDER_MASK ) != QRM_SOLID )
	{
		if ( !pTex )
		{
			AddRect( _rTarget, GetWhiteTexture(), CTRect<float>(1,1,2,2), color, fZ );
			return;
		}
		pContainer = GetTextureContainer( pTex, &region );
		if ( pContainer )
		{
			fXAdd = region.place.x1;
			fYAdd = region.place.y1;
		}
	}
	// determine true target rect
	CTRect<float> rTarget(_rTarget);
	rTarget.x1 = Float2Int( rTarget.x1 );
	rTarget.x2 = Float2Int( rTarget.x2 );
	rTarget.y1 = Float2Int( rTarget.y1 );
	rTarget.y2 = Float2Int( rTarget.y2 );
	const bool bSampleTexture = logicalSource && ( dm & QRM_RENDER_MASK ) != QRM_SOLID && pTex;
	const auto sample = S2TextureSampling::SampleRect(
		{rTarget.x1, rTarget.y1, rTarget.x2, rTarget.y2},
		{_rSrc.x1, _rSrc.y1, _rSrc.x2, _rSrc.y2},
		bSampleTexture ? pTex->GetTexelScaleX() : 1.0f,
		bSampleTexture ? pTex->GetTexelScaleY() : 1.0f, fXAdd, fYAdd );
	CTRect<float> rSrc( sample.x1, sample.y1, sample.x2, sample.y2 );
	const float uvPackingSteps = IsTnLDevice() ? 1.0f : S2TextureSampling::UVPackingSteps(
		sample, pContainer ? region.size.x : 0, pContainer ? region.size.y : 0 );
	// color.a == 0 - could also be optimized
	fZ = fZ * 998.0f + 1.0f; // fZ in [0...1]
	DWORD dwColor = color.color;
	if ( IsTnLDevice() )
	{
		SGeomVecT1C1 *pGeom = ((SRealRectInfoLock<SGeomVecT1C1>*)GetRectInfoLock( pContainer, region, uvPackingSteps ))->AddRect();
		FillVertex( pGeom + 0, rTarget.x1 - 0.5f, rTarget.y1 - 0.5f, rSrc.x1, rSrc.y1, dwColor, fScaleU, fScaleV, fZ );
		FillVertex( pGeom + 1, rTarget.x1 - 0.5f, rTarget.y2 - 0.5f, rSrc.x1, rSrc.y2, dwColor, fScaleU, fScaleV, fZ );
		FillVertex( pGeom + 2, rTarget.x2 - 0.5f, rTarget.y2 - 0.5f, rSrc.x2, rSrc.y2, dwColor, fScaleU, fScaleV, fZ );
		FillVertex( pGeom + 3, rTarget.x2 - 0.5f, rTarget.y1 - 0.5f, rSrc.x2, rSrc.y1, dwColor, fScaleU, fScaleV, fZ );
	}
	else
	{
		SRectVertex *pGeom = ((SRealRectInfoLock<SRectVertex>*)GetRectInfoLock( pContainer, region, uvPackingSteps ))->AddRect();
		FillVertex( pGeom + 0, rTarget.x1 - 0.5f, rTarget.y1 - 0.5f, rSrc.x1, rSrc.y1, dwColor, fZ, uvPackingSteps );
		FillVertex( pGeom + 1, rTarget.x1 - 0.5f, rTarget.y2 - 0.5f, rSrc.x1, rSrc.y2, dwColor, fZ, uvPackingSteps );
		FillVertex( pGeom + 2, rTarget.x2 - 0.5f, rTarget.y2 - 0.5f, rSrc.x2, rSrc.y2, dwColor, fZ, uvPackingSteps );
		FillVertex( pGeom + 3, rTarget.x2 - 0.5f, rTarget.y1 - 0.5f, rSrc.x2, rSrc.y1, dwColor, fZ, uvPackingSteps );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void C2DQuadsRenderer::Flush()
{
	if ( !pLock )
		return;
	ASSERT( pPrevContainer || (dm & QRM_RENDER_MASK) == QRM_SOLID );
	rc.Use();
	if ( IsTnLDevice() )
	{
		switch ( dm & QRM_RENDER_MASK )
		{
		case QRM_SIMPLE:
			{
				rc.SetPixelShader( psDiffuseTexture );
				rc.SetTexture( 0, pPrevContainer );
			}
			break;
		case QRM_NOCOLOR:
			{
				rc.SetPixelShader( psTextureCopyAlpha );
				rc.SetTexture( 0, pPrevContainer );
			}
			break;
		case QRM_SOLID:
			{
				rc.SetPixelShader( psDiffuse );
			}
			break;
		default: ASSERT(0); break;
		}
	}
	else
	{
		switch ( dm & QRM_RENDER_MASK )
		{
		case QRM_SIMPLE:
			{
				rc.SetPixelShader( psDiffuseTexture );
				rc.SetVertexShader( vsRender2D );
				rc.SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
				rc.SetTexture( 0, pPrevContainer );
			}
			break;
		case QRM_NOCOLOR:
			{
				rc.SetPixelShader( psTextureCopyAlpha );
				//rc.SetVertexShader( vsTextureScale );
				rc.SetVertexShader( vsRender2D );//TextureScale );
				rc.SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
				rc.SetTexture( 0, pPrevContainer );
			}
			break;
		case QRM_SOLID:
			{
				rc.SetPixelShader( psDiffuse );
				rc.SetVertexShader( vsDrawTexU );
			}
			break;
		case QRM_USER_EFFECT:
			{
				ASSERT( IsValid(pUserEffect) );
				if ( IsValid(pUserEffect) )
					pUserEffect->SetEffect( &rc, pPrevContainer, fScaleU, fScaleV );
			}
			break;
		default: ASSERT(0); break;
		}
	}
	CObj<CGeometry> pGeom( pLock->pGeom );
	STriangleList triList;
	MakeQuadTriList( pLock->GetRectsNum(), &triList );
	delete pLock;
	pLock = 0;
	rc.DrawPrimitive( pGeom, triList );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CopyTexture( const NGfx::CRenderContext &_rc, const CVec2 &vTargetViewport, const CTRect<float> &rTarget, 
	NGfx::CTexture *pTex, const CTRect<float> &rSrc, const CVec4 &vColor, I2DEffect *pEffect )
{
	NGfx::SPixel8888 color;
	color.color = GetDWORDColor( vColor );
	if ( pEffect )
	{
		C2DQuadsRenderer r( _rc, vTargetViewport, QRM_DEPTH_NONE|QRM_USER_EFFECT );
		r.SetUserEffect( pEffect );
		r.AddRect( rTarget, pTex, rSrc, color );
	}
	else
	{
		int dm = color.color == 0xffffffff ? QRM_NOCOLOR|QRM_DEPTH_NONE : QRM_DEPTH_NONE;
		C2DQuadsRenderer r( _rc, vTargetViewport, dm );
		r.AddRect( rTarget, pTex, rSrc, color );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void AddFullTextureRect( C2DQuadsRenderer *pR, NGfx::CTexture *pTex )
{
	CDynamicCast<I2DBuffer> p2D( pTex );
	int nXSize = p2D->GetXSize(), nYSize = p2D->GetYSize();
	CTRect<float> r;
	r.x1 = 0; r.x2 = nXSize;
	r.y1 = 0; r.y2 = nYSize;
	pR->AddRect( r, pTex, r );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void ShowTexture( NGfx::CTexture *pTex, float fMag )
{
	C2DQuadsRenderer rend;
	rend.SetTarget( CVec2( 800 / fMag, 600 / fMag ), QRM_NOCOLOR );
	AddFullTextureRect( &rend, pTex );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void ShowTexture( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, const CVec2 &vScreenSize )
{
	CObj<NGfx::CTexture> pHold(pTex);
	pRC->SetAlphaCombine( NGfx::COMBINE_ALPHA );
	C2DQuadsRenderer rend;
	rend.SetTarget( *pRC, vScreenSize, QRM_NOCOLOR );
	AddFullTextureRect( &rend, pTex );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class CBlurEffect : public I2DEffect
{
	OBJECT_NOCOPY_METHODS(CBlurEffect);
	float fBlurShift;
public:
	CBlurEffect( float _f = 0 ) : fBlurShift(_f) {}
	virtual void SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
	{
		pRC->SetPixelShader( psShadowBlur );//psTexture );//
		pRC->SetVertexShader( vsShadowBlur );
		// Blur offsets are physical texels, independent of SHORT2 packing precision.
		const int width = Max( 1, pTex->GetXSize() ), height = Max( 1, pTex->GetYSize() );
		const bool unnormalizedNP2 = NGfx::IsNVidiaNP2Bug() && pTex->IsNP2();
		float fOffsetX = fBlurShift * S2TextureSampling::TexelUVScale( width, unnormalizedNP2 );
		float fOffsetY = fBlurShift * S2TextureSampling::TexelUVScale( height, unnormalizedNP2 );
		pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
		pRC->SetVSConst( 17, CVec4( 0, fOffsetY, 0, 0 ) );
		pRC->SetVSConst( 18, CVec4(  fOffsetX, -fOffsetY, 0, 0 ) );
		pRC->SetVSConst( 19, CVec4( -fOffsetX, -fOffsetY, 0, 0 ) );
		pRC->SetTexture( 0, pTex );
		pRC->SetTexture( 1, pTex );
		pRC->SetTexture( 2, pTex );
		pRC->SetTexture( 3, pTex );
		pRC->SetPSConst( 0, CVec4( 0, 0, 0, 0.5f - 0.0f / 256 ) );
		pRC->SetPSConst( 1, CVec4( 0, 0, 0, 1.0f / 2 ) );
		pRC->SetPSConst( 2, CVec4( 0, 0, 0, 2.0f / 3 ) );
		pRC->SetPSConst( 3, CVec4( 0, 0, 0, 3.0f / 4 ) );
	}
};
void BlurLight( NGfx::CRenderContext *pRC, int nSrcRegister, int nDestRegister, float fBlurStrength )
{
	CTRect<float> rDest;
	NGfx::GetRegisterSize( &rDest );
	pRC->SetRegister( nDestRegister );
	NGfx::C2DQuadsRenderer qr( *pRC, CVec2( rDest.x2, rDest.y2 ), QRM_USER_EFFECT );
	qr.SetUserEffect( new CBlurEffect( fBlurStrength ) );
	qr.AddRect( rDest, GetRegisterTexture( nSrcRegister ), rDest );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void ModulateRegister( NGfx::CRenderContext *pRC, int nDestRegister, const CVec4 &vColor )
{
	CTRect<float> rDest;
	NGfx::GetRegisterSize( &rDest );
	NGfx::CRenderContext rc( *pRC );
	rc.SetRegister( nDestRegister );
	rc.SetAlphaCombine( COMBINE_MUL	);
	NGfx::C2DQuadsRenderer qr( rc, CVec2( rDest.x2, rDest.y2 ), QRM_SOLID );
	NGfx::SPixel8888 color;
	color.color = GetDWORDColor( vColor );
	qr.AddRect( rDest, 0, rDest, color );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class CAlphaSqrtEffect : public I2DEffect
{
	OBJECT_NOCOPY_METHODS(CAlphaSqrtEffect );
	float fMul;
public:
	CAlphaSqrtEffect( float _fMul = 0 ) : fMul(_fMul) {}
	virtual void SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
	{
		pRC->SetPixelShader( psAlphaSqrtModulate );
		pRC->SetVertexShader( vsTextureSqrtModulate );
		pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
		pRC->SetVSConst( 17, CVec4( 0, 0, 0, fMul ) );
		pRC->SetTexture( 0, pTex );
	}
};
void AlphaSqrtModulateRegister( NGfx::CRenderContext *pRC, int nDestRegister, int nSrcRegister, float fMul )
{
	CTRect<float> rDest;
	NGfx::GetRegisterSize( &rDest );
	pRC->SetRegister( nDestRegister );
	NGfx::C2DQuadsRenderer qr( *pRC, CVec2( rDest.x2, rDest.y2 ), QRM_USER_EFFECT );
	qr.SetUserEffect( new CAlphaSqrtEffect( fMul ) );
	qr.AddRect( rDest, GetRegisterTexture( nSrcRegister ), rDest );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CShowAlphaEffect
////////////////////////////////////////////////////////////////////////////////////////////////////
void CShowAlphaEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	pRC->SetPixelShader( psTextureAlpha );
	pRC->SetVertexShader( vsRender2D );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
	pRC->SetTexture( 0, pTex );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CCLAmbientMulDiffuseEffect
////////////////////////////////////////////////////////////////////////////////////////////////////
void CCLAmbientMulDiffuseEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	pRC->SetPixelShader( psCLTexTexture4 );
	pRC->SetVertexShader( vsRender2D2 );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
	pRC->SetPSConst( 0, vAmbient );
	pRC->SetTexture( 0, NGfx::GetRegisterTexture( nReg ) ); // cl
	pRC->SetTexture( 1, pTex ); // diffuse
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}
