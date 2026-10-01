#include "StdAfx.h"
#include "GParticleInfo.h"
#include "GPointLightGlow.h"
#include "GScene.h"
#include "..\Misc\RandomGen.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
class CPointGlowEffect : public CParticleEffect
{
	OBJECT_BASIC_METHODS(CPointGlowEffect);
public:
	CVec3 vPos;
	float fSize;
	int nAlpha;
	CPtr<CPointGlowAnimator> pParent;

	void AddParticles( IParticleOutput *pRender );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
const float F_NEAR_CLIP = 1;//0.2f;
void CPointGlowEffect::AddParticles( IParticleOutput *pRender )
{
	pParent->CalcSize();

	const SParticleOrientationInfo &orientation = pRender->GetOrientationInfo();
	CVec3 vRes[4];
	float fLeng = ( vPos - orientation.vBasic[3] ) * orientation.vBasic[2];
	if ( fLeng <= F_NEAR_CLIP )
		return;
	// Retail 0x5471f8 / v1.2 0x5472d8: build the billboard at the light,
	// not on the camera's near plane. Perspective supplies the distance scaling.
	vRes[0] = vPos - orientation.vBasic[0] * fSize - orientation.vBasic[1] * fSize;
	vRes[1] = vPos + orientation.vBasic[0] * fSize - orientation.vBasic[1] * fSize;
	vRes[2] = vPos + orientation.vBasic[0] * fSize + orientation.vBasic[1] * fSize;
	vRes[3] = vPos - orientation.vBasic[0] * fSize + orientation.vBasic[1] * fSize;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex( textures[0] );
	if ( !IsValid( pTex ) )
		return;
	pTex.Refresh();
	STransparentTexturePlace tPlace;
	GetTransparentTexturePlace( &tPlace, pTex->GetValue() );
	// Retail 0x5474ac / v1.2 0x54758c: fixed sorting depth, independent of camera motion.
	pRender->AddParticle( vRes, 0xffffff | (nAlpha<<24), tPlace, 1e10f );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CPointGlowAnimator
////////////////////////////////////////////////////////////////////////////////////////////////////
CPointGlowAnimator::CPointGlowAnimator( IGScene *_pScene, CFuncBase<STime> *_pTime, CFuncBase<CVec3> *_pPlace, 
	CPtrFuncBase<NGfx::CTexture> *_pTexture, float _fLightSize, float _fOnTime, float _fOffTime )
	: pScene(_pScene), pTime(_pTime), pPlacement(_pPlace), pTexture(_pTexture), tPrev(0), fSize(0), fLightSize(_fLightSize),
	fOnTime(_fOnTime), fOffTime(_fOffTime), pCamera( pScene->GetCamera() ), nLastMaskAny(0), tNextCheck(0), bIsVisible(false)
{
	fOnTime = Max( 1e-3f, fOnTime );
	fOffTime = Max( 1e-3f, fOffTime );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CPointGlowAnimator::CalcSize()
{
	bool bShortInterval = pCamera.Refresh();
	SGroupSelect mask( pScene->GetLastMask() );
	if ( nLastMaskAny != mask.nMaskAny )
	{
		bShortInterval = true;
		nLastMaskAny = mask.nMaskAny;
	}
	STime tCur = pTime->GetValue();
	if ( bShortInterval )
		tNextCheck = Min( tNextCheck, tCur + 150 );
	if ( tCur > tNextCheck )
	{
		CVec3 vCamera = pCamera->GetValue();
		CRay r;
		r.ptOrigin = vCamera;
		r.ptDir = pPlacement->GetValue() - vCamera;
		float f;
		CVec3 vPlaceHolder;
		mask.nMaskEvery |= N_MASK_CAST_SHADOW;
		if ( pScene->TraceScene( mask, r, &f, &vPlaceHolder, &vPlaceHolder, SPS_STATIC ) )
			bIsVisible = f > 1;
		else
			bIsVisible = true;
		if ( bShortInterval )
			tNextCheck = tCur + GlobalGameRandom().Get( 80, 150 );
		else
			tNextCheck = tCur + GlobalGameRandom().Get( 500, 1000 );
	}
	float fDeltaT = Max( 0.0f, ((float)tCur - tPrev)/1024.0f );
	if ( bIsVisible )
		fSize = 1 - ( 1 - fSize ) * exp( -fDeltaT / fOnTime );
	else
		fSize = fSize * exp( -fDeltaT / fOffTime );
	tPrev = tCur;

	CDynamicCast<CPointGlowEffect> pRealValue(pValue);
	CPointGlowEffect &value = *pRealValue;
	value.fSize = fLightSize * fSize;
	value.nAlpha = Float2Int(fSize * 255);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CPointGlowAnimator::Recalc()
{
	if ( !IsValid( pValue ) )
	{
		pValue = new CPointGlowEffect;
		pValue->textures.resize(1);
		pValue->textures[0] = pTexture;
	}
	CDynamicCast<CPointGlowEffect> pRealValue(pValue);
	CPointGlowEffect &value = *pRealValue;
	value.bEnd = false;
	value.vPos = pPlacement->GetValue();
	value.pParent = this;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x114a2180, CPointGlowEffect )
REGISTER_SAVELOAD_CLASS( 0x114a2181, CPointGlowAnimator )
