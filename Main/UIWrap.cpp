#include "StdAfx.h"
#include "DisplayLayout.h"
#include "../MiscDll/Commands.h"
#include "GPixelFormat.h"
#include "Transform.h"
#include "GSceneUtils.h"
#include "RectLayout.h"
#include "GView.h"
#include "G2DView.h"
#include "GText.h"
#include "Interface.h"
#include "UIWrap.h"
#include "UIML.h"
#include "DiscretePos.h"
#include "..\Misc\StrProc.h"
#include "..\DBFormat\DataLight.h"
#include "..\DBFormat\DataFormat.h"
#include "..\DBFormat\DataInterface.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NUI
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// Retail @0x329500: UI quads use premultiplied vertex colors. C2DScene pairs these with
// COMBINE_SMART_ALPHA (ONE/INVSRCALPHA), preserving ordinary translucency while allowing
// bright additive-looking assets such as the store category flash to remain visible.
void MakeColor( NGfx::SPixel8888 *pResult, const NGfx::SPixel8888 &sColor )
{
	pResult->r = (unsigned int)( sColor.r ) * sColor.a / 0xFF;
	pResult->g = (unsigned int)( sColor.g ) * sColor.a / 0xFF;
	pResult->b = (unsigned int)( sColor.b ) * sColor.a / 0xFF;
	pResult->a = sColor.a;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CTextDraw
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail ctor @0x329c20: sSize = sRealSize = size; sPosition = pos; nSize = 0; wsText(text);
// pML = CreateML(); pML->SetText(wsText, 0).
CTextDraw::CTextDraw( const SPoint &_sPosition, const SPoint &_sSize, const wstring &_wsText ):
	sPosition( _sPosition ), sSize( _sSize ), sRealSize( _sSize ), wsText(_wsText), nSize( 0 )
{
	pML = CreateML();
	pML->SetText( wsText, 0 );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail GetSize @0x329a30: a dead/null view returns the requested sSize untouched; otherwise
// refresh the layout (UpdateText), copy requested -> resolved, and overwrite each -1 component with
// the layout's measured extent mapped back into virtual 1024x768 coordinates. NOTE retail always
// recomputes sRealSize on a live view (the old dev body only did so when a component was -1).
const SPoint& CTextDraw::GetSize( NGScene::I2DGameView *pView )
{
	if ( !IsValid( pView ) )
		return sSize;

	UpdateText( pView );

	const SPoint &sMLSize = pML->GetSize();
	const CVec2 &vScreenRect = pView->GetViewportSize();

	sRealSize = sSize;
	if ( sSize.x == -1 )
		sRealSize.x = sMLSize.x / S2UI::Scale();
	if ( sSize.y == -1 )
		sRealSize.y = sMLSize.y / S2UI::Scale();

	return sRealSize;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CTextDraw::SetSize( const SPoint &_sSize )
{
	sSize = _sSize;
	sRealSize = _sSize;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const SPoint& CTextDraw::GetPosition() const
{
	return sPosition;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CTextDraw::SetPosition( const SPoint &_sPosition )
{
	sPosition = _sPosition;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const wstring& CTextDraw::GetText() const
{
	return wsText;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail SetText @0x329ce0: nSize = 0 (force a regenerate); wsText = arg (self-assign guarded in
// retail); push the text into the layout object.
void CTextDraw::SetText( const wstring &_wsText )
{
	nSize = 0;
	if ( &_wsText != &wsText )
		wsText = _wsText;
	pML->SetText( wsText, 0 );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail Draw @0x329b10: resolve the size, map position+rect to screen (through the window, or the
// raw 1024x768 scale when free-standing), then let the layout render. NOTE retail's tail does NOT
// re-run UpdateText here (GetSize already did) -- the old dev tail call is gone.
void CTextDraw::Draw( CWindow *pWindow, const STime &sTime, NGScene::I2DGameView *pView )
{
	SPoint sTextSize = GetSize( pView );
	SRect sScrWindow( sPosition.x, sPosition.y, sPosition.x + sTextSize.x, sPosition.y + sTextSize.y );
	SPoint sScrPosition( sPosition );
	if ( pWindow )
	{
		if ( !pWindow->ClientToScreen( &sScrPosition, &sScrWindow, false ) )
			return;
		pWindow->VirtualToScreen( &sScrPosition, &sScrWindow );
	}
	else
	{
		CVec2 vScreenRect = pView->GetViewportSize();

		float fXCoef = S2UI::Scale();
		float fYCoef = S2UI::Scale();
		sScrPosition.x *= fXCoef;
		sScrPosition.y *= fYCoef;
		sScrWindow.x1 *= fXCoef;
		sScrWindow.y1 *= fYCoef;
		sScrWindow.x2 *= fXCoef;
		sScrWindow.y2 *= fYCoef;
	}

	pML->Render( pView, sScrPosition, sScrWindow );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail UpdateText @0x329680: resolve the layout width in screen pixels (a positive requested
// width scaled by viewport/1024, else the raw viewport width) and regenerate the layout only when
// it changed since the cached nSize.
void CTextDraw::UpdateText( NGScene::I2DGameView *pView )
{
	CVec2 vScreenRect = pView->GetViewportSize();
	int nNewSize;
	if ( sSize.x > 0 )
		nNewSize = int( float( sSize.x ) * S2UI::Scale() );   // retail truncates here (or ah,0xc)
	else
		nNewSize = int( vScreenRect.x );

	if ( nNewSize != nSize || nDisplayRevision != S2Platform::Display().revision ||
	     bVectorFonts != (NGlobal::GetVar("ui_vector_fonts", 1).GetFloat() != 0) )
	{
		nDisplayRevision = S2Platform::Display().revision;
		bVectorFonts = NGlobal::GetVar("ui_vector_fonts", 1).GetFloat() != 0;
		nSize = nNewSize;
		pML->Generate( pView, nNewSize );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CImageDraw
////////////////////////////////////////////////////////////////////////////////////////////////////
CImageDraw::CImageDraw( const SRect &_sWindow, NDb::CUITexture* _pTexture, const SRect &_sTexRect, const NGfx::SPixel8888 &_sColor ):
	sWindow( _sWindow ), pUITexture(_pTexture), sTextureRect( _sTexRect ), sColor( _sColor ), vScale( 1, 1 )
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const SRect& CImageDraw::GetWindow()
{
	return sWindow;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CImageDraw::SetWindow( const SRect &_sWindow )
{
	sWindow = _sWindow;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CImageDraw::SetScale( const CVec2 &_vScale )
{
	vScale = _vScale;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CImageDraw::SetColor( const NGfx::SPixel8888 &_sColor )
{
	sColor = _sColor;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CImageDraw::SetImage( NDb::CUITexture* _pTexture, const SRect &sTexRect )
{
	pUITexture = _pTexture;
	sTextureRect = sTexRect;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CImageDraw::DrawAtPixels(const STime& time, NGScene::I2DGameView* view, const CVec2& position) {
  vPixelPosition = position; bPixelPosition = true;
  Draw(0, time, view);
  bPixelPosition = false;
}
void CImageDraw::Draw( CWindow *pWindow, const STime &sTime, NGScene::I2DGameView *pView )
{
	CVec2 vScreenRect = pView->GetViewportSize();
	NGfx::SPixel8888 sDrawColor;
	MakeColor( &sDrawColor, sColor );

	SRect sScrWindow( sWindow );
	SPoint sScrPosition( sWindow.x1, sWindow.y1 );
	if ( pWindow )
	{
		if ( !pWindow->ClientToScreen( &sScrPosition, &sScrWindow, false ) )
			return;
		pWindow->VirtualToScreen( &sScrPosition, &sScrWindow );
	}
	else
	{
		float fXCoef = S2UI::Scale();
		float fYCoef = S2UI::Scale();
		sScrPosition.x *= fXCoef;
		sScrPosition.y *= fYCoef;
		sScrWindow.x1 *= fXCoef;
		sScrWindow.y1 *= fYCoef;
		sScrWindow.x2 *= fXCoef;
		sScrWindow.y2 *= fYCoef;
	}

	if (bPixelPosition && !pWindow) {
		const int dx = int(vPixelPosition.x)-sScrPosition.x, dy = int(vPixelPosition.y)-sScrPosition.y;
		sScrPosition.x += dx; sScrPosition.y += dy;
		sScrWindow.x1 += dx; sScrWindow.x2 += dx; sScrWindow.y1 += dy; sScrWindow.y2 += dy;
	}

	if ( IsValid( pUITexture ) )
	{
		NDb::EUIMode eMode = NDb::UIM_1024x768;
		if (!IsValid(pUITexture->pTextures[eMode])) {
			for (int i = 0; i < 4; ++i) if (IsValid(pUITexture->pTextures[i])) { eMode = (NDb::EUIMode)i; break; }
		}
		if (!IsValid(pUITexture->pTextures[eMode])) return;
		CTPoint<float> scale(S2UI::Scale() * pUITexture->nWidth / pUITexture->pTextures[eMode]->nWidth,
		                      S2UI::Scale() * pUITexture->nHeight / pUITexture->pTextures[eMode]->nHeight);
		NDb::CTexture *pTexture = pUITexture->pTextures[eMode];
		ASSERT( ( pTexture->nWidth != 0 ) && ( pTexture->nHeight != 0 ) );
		if ( ( pTexture->nWidth == 0 ) || ( pTexture->nHeight == 0 ) )
			return;

		CTRect<float> sTexRect( sTextureRect.x1, sTextureRect.y1, sTextureRect.x2, sTextureRect.y2 );
		if ( ( sTexRect.Width() == 0 ) && ( sTexRect.Height() == 0 ) )
		{
			sTexRect.x1 = 0;
			sTexRect.x2 = pTexture->nWidth;
			sTexRect.y1 = pTexture->nHeight;
			sTexRect.y2 = 0;
		}

		// retail 12-byte CRectLayout has no scale member -- the tile quad size (|texrect| * the
		// mode-scale * the user vScale) is baked into every AddRect (retail 6-arg AddRect @0x174620).
		const CVec2 vTileScale( scale.x * vScale.x, scale.y * vScale.y );
		const float fTileSizeX = fabsf( sTexRect.Width() ) * vTileScale.x;
		const float fTileSizeY = fabsf( sTexRect.Height() ) * vTileScale.y;
		CRectLayout sLayout;
		if(sAspectSource.x>0 && sAspectSource.y>0) {
			// Loading assets include power-of-two padding. Fit the authored visible
			// portion, not the padded texture or a repeated copy of the image.
			const int sw=Min(sAspectSource.x,pUITexture->nWidth), sh=Min(sAspectSource.y,pUITexture->nHeight);
			const auto fit=S2Display::Fit(sWindow.Width(),sWindow.Height(),sw,sh);
			CRectLayout bars;
			bars.AddRect(0,0,sWindow.Width()*S2UI::Scale(),sWindow.Height()*S2UI::Scale(),
			    CTRect<float>(0,0,1,1),NGfx::SPixel8888(0,0,0,255));
			pView->CreateDynamicRects((NDb::CTexture*)0,bars,sScrPosition,sScrWindow);
			sLayout.AddRect(fit.x*S2UI::Scale(),fit.y*S2UI::Scale(),fit.width*S2UI::Scale(),fit.height*S2UI::Scale(),
			    CTRect<float>(0,pTexture->nHeight,float(sw)*pTexture->nWidth/pUITexture->nWidth,
			        pTexture->nHeight-float(sh)*pTexture->nHeight/pUITexture->nHeight),sDrawColor);
		} else if(nHorizontalSplit<0 && sWindow.Width()>pUITexture->nWidth) {
            const auto slices=S2Display::StretchHorizontal(sWindow.Width(),pUITexture->nWidth,-nHorizontalSplit);
            const float tx=sTexRect.Width()/pUITexture->nWidth;
            for(const auto& slice:slices)
                sLayout.AddRect(slice.position*S2UI::Scale(),0,slice.width*S2UI::Scale(),fTileSizeY,
                    CTRect<float>(sTexRect.x1+slice.source*tx,sTexRect.y1,
                        sTexRect.x1+(slice.source+slice.sourceWidth)*tx,sTexRect.y2),sDrawColor);
		} else if (nHorizontalSplit>0 && nHorizontalSplit<pUITexture->nWidth && sWindow.Width()>pUITexture->nWidth) {
			const auto slices=S2Display::ExpandHorizontal(sWindow.Width(),pUITexture->nWidth,nHorizontalSplit);
			const float texPerLogical=sTexRect.Width()/pUITexture->nWidth;
			// The empty HUD plate supplies the gap, so rounded weapon-slot edges are not extruded.
			NDb::CTexture* fillTexture=IsValid(pHorizontalFill) ? pHorizontalFill->pTextures[eMode].GetPtr() : 0;
			if (!IsValid(fillTexture) && IsValid(pHorizontalFill))
				for(int i=0;i<4;++i) if(IsValid(pHorizontalFill->pTextures[i])) { fillTexture=pHorizontalFill->pTextures[i]; break; }
			if(IsValid(fillTexture)) {
				const auto& gap=slices[1];
				const auto material=S2Display::NeutralHorizontalTile(pHorizontalFill->nWidth,nHorizontalSplit);
				const int fillWidth=Max(1,material.width), sourceX=material.source;
				const int fillHeight=Max(1,pHorizontalFill->nHeight);
				const float tx=float(fillTexture->nWidth)/pHorizontalFill->nWidth;
				CRectLayout fill;
				// Repeat the complete neutral tile at its authored size: stretching a single
				// column turns its grain into conspicuous horizontal stripes.
				for(int y=0;y<sWindow.Height();y+=fillHeight)
					for(int x=0;x<gap.width;x+=fillWidth) {
						const int width=Min(fillWidth,gap.width-x);
						const int u1=x/fillWidth%2 ? sourceX+fillWidth : sourceX;
						const int u2=x/fillWidth%2 ? u1-width : u1+width;
						fill.AddRect((gap.position+x)*S2UI::Scale(),y*S2UI::Scale(),width*S2UI::Scale(),fillHeight*S2UI::Scale(),
						    CTRect<float>(u1*tx,fillTexture->nHeight,u2*tx,0),sDrawColor);
					}
				pView->CreateDynamicRects(fillTexture,fill,sScrPosition,sScrWindow);
			}
			for (int y=0; y<sWindow.Height(); y+=pUITexture->nHeight) for (const auto& slice:slices) {
				if(IsValid(fillTexture) && &slice==&slices[1]) continue;
				const CTRect<float> uv(sTexRect.x1+slice.source*texPerLogical,sTexRect.y1,
				    sTexRect.x1+(slice.source+slice.sourceWidth)*texPerLogical,sTexRect.y2);
				sLayout.AddRect(slice.position*S2UI::Scale()*vScale.x,y*vTileScale.y,
				    slice.width*S2UI::Scale()*vScale.x,fTileSizeY,uv,sDrawColor);
			}
		} else {
			for ( int nTempY = 0; nTempY < sWindow.Height(); nTempY += pUITexture->nHeight )
				for ( int nTempX = 0; nTempX < sWindow.Width(); nTempX += pUITexture->nWidth )
					sLayout.AddRect( nTempX * vTileScale.x, nTempY * vTileScale.y, fTileSizeX, fTileSizeY, sTexRect, sDrawColor );
		}

		pView->CreateDynamicRects( pTexture, sLayout, sScrPosition, sScrWindow );
	}
	else
	{
		// texture-less fill: the old scale (vp/1024, vp/768) times the |texrect| (= the window dims)
		CRectLayout sLayout;
		sLayout.AddRect( 0, 0,
			sWindow.Width() * S2UI::Scale(),
			sWindow.Height() * S2UI::Scale(),
			CTRect<float>( 0, 0, sWindow.Width(), sWindow.Height() ), sDrawColor );
		pView->CreateDynamicRects( (NDb::CTexture*)0, sLayout, sScrPosition, sScrWindow );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void DrawTiledPanelBackground(CWindow* window, NGScene::I2DGameView* view, NDb::CUITexture* plate)
{
	if(!IsValid(plate) || plate->nWidth<=0 || plate->nHeight<=0) return;
	NDb::CTexture* texture=plate->pTextures[NDb::UIM_1024x768];
	if(!IsValid(texture)) for(int i=0;i<4;++i) if(IsValid(plate->pTextures[i])) { texture=plate->pTextures[i]; break; }
	if(!IsValid(texture)) return;
	SPoint position; SRect clip;
	if(!window->ClientToScreen(&position,&clip)) return;
	window->VirtualToScreen(&position,&clip);
	const int tw=Min(512,plate->nWidth/2), th=Min(128,plate->nHeight/2);
	if(th<=0) return;
	const float tx=float(texture->nWidth)/plate->nWidth, ty=float(texture->nHeight)/plate->nHeight;
	const int sx=Min(64,plate->nWidth-tw), sy=Min(40,plate->nHeight-th);
	const SPoint size=window->GetSize();
	CRectLayout tiles;
	for(int y=0;y<size.y;y+=th) for(int x=0;x<size.x;x+=tw) {
		const int w=Min(tw,size.x-x), h=Min(th,size.y-y);
		// Mirrored neighbors share their edge texels, avoiding visible rectangular seams.
		const int u1=x/tw%2 ? sx+tw : sx, u2=x/tw%2 ? sx+tw-w : sx+w;
		const int v1=y/th%2 ? plate->nHeight-sy-th : plate->nHeight-sy;
		const int v2=y/th%2 ? v1+h : v1-h;
		tiles.AddRect(x*S2UI::Scale(),y*S2UI::Scale(),w*S2UI::Scale(),h*S2UI::Scale(),
		    CTRect<float>(u1*tx,v1*ty,u2*tx,v2*ty),NGfx::SPixel8888(255,255,255,255));
	}
	view->CreateDynamicRects(texture,tiles,position,clip);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CModelWrap
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail @0x329710: Init + the fixed UI-item projection.
void MakeProjection( CTransformStack *pTS )
{
	pTS->Init();
	pTS->MakeProjective( CVec2( S2UI::Width(), S2UI::Height() ),
		S2Display::PreviewFOV(60, S2UI::Height(), 1, 1), 0.1f, 300 );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail @0x329830 (disasm-verified consts 512/384): conjugate the widget-center NDC shift through
// the projection, then camera, then the model matrix; backward = identity.
void MakeModelTransform( SFBTransform *pRes, const SPoint &sPosition, const SPoint &sSize, const SHMatrix &sCameraTransform, const SHMatrix &sModelTransform )
{
	CTransformStack tsCamera;
	tsCamera.Init();
	tsCamera.SetCamera( sCameraTransform );

	CTransformStack tsProjection;
	MakeProjection( &tsProjection );

	CVec2 vCenter( sPosition.x + sSize.x / 2, sPosition.y + sSize.y / 2 );

	SHMatrix sShift;
	Identity( &sShift );
	sShift._14 = ( vCenter.x - S2UI::Width()*0.5f ) / (S2UI::Width()*0.5f);
	sShift._24 = ( S2UI::Height()*0.5f - vCenter.y ) / (S2UI::Height()*0.5f);

	SHMatrix sA, sB;
	Multiply( &sA, sShift, tsProjection.Get().forward );
	Multiply( &sB, tsProjection.Get().backward, sA );
	Multiply( &sA, sB, tsCamera.Get().forward );
	Multiply( &pRes->forward, sA, sModelTransform );
	Identity( &pRes->backward );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail @0x32a920: identity wire matrices, then SetScene(0,true) -> own fast view, bParentScene=false.
CModelDraw::CModelDraw( const SRect &_sWindow, NDb::CModel* _pModel ):
	sWindow( _sWindow ), pModel( _pModel ), sColor( 0xFF, 0xFF, 0xFF, 0xFF )
{
	CTransformStack ts;
	ts.Init();
	pTransform = new NGScene::CCFBTransform;
	pTransform->Set( ts.Get() );

	Identity( &sModelTransform );
	Identity( &sCameraTransform );

	SetScene( 0, true );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const SRect& CModelDraw::GetWindow() const
{
	return sWindow;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelDraw::SetWindow( const SRect &_sWindow )
{
	sWindow = _sWindow;
	// retail @0x329570: SetWindow ALWAYS raises bUpdated -- CModel::Draw (@0x3126f0) calls it every
	// frame, so the placement fold (MakeModelTransform) tracks the window's current screen position
	bUpdated = true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const NGfx::SPixel8888& CModelDraw::GetColor() const
{
	return sColor;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelDraw::SetColor( const NGfx::SPixel8888 &_sColor )
{
	sColor = _sColor;
	CVec3 vAmbient( 0.5f * float( sColor.r ) / 0xFF, 0.5f * float( sColor.g ) / 0xFF, 0.5f * float( sColor.b ) / 0xFF );
	p3DView->SetAmbient( vAmbient, vAmbient );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
NDb::CModel* CModelDraw::GetModel() const
{
	return pModel;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelDraw::SetModel( NDb::CModel* _pModel )
{
	pModel = _pModel;
	pRender = 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail @0x32a390. (bFast=false is retail CreateNewInterfaceView @0x18b3e0 -- new CGameView +
// SetInterfaceMode(1); dev's scene has no interface-mode knob, CreateNewView is its standing stand-in.)
void CModelDraw::SetScene( NGScene::IGameView *pView, bool bFast )
{
	pRender = 0;
	if ( IsValid( pView ) )
	{
		p3DView = pView;
		bParentScene = true;
	}
	else
	{
		bParentScene = false;
		p3DView = bFast ? NGScene::CreateNewFastInterfaceView() : NGScene::CreateNewView();

		SRand rnd;
		NDb::CTAmbientLight *pLight = NDb::GetTAmbientLight( 7 );
		if ( IsValid( pLight ) )
			p3DView->SetAmbient( pLight->GetLight( &rnd ), NGScene::IGameView::LT_INVENTORY );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelDraw::SetModelTransform( const SHMatrix &sMatrix )
{
	bUpdated = true;
	sModelTransform = sMatrix;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelDraw::SetCameraTransform( const SHMatrix &sMatrix )
{
	bUpdated = true;
	sCameraTransform = sMatrix;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail @0x32a4d0. Flow: off-screen -> DROP the mesh; lazy (re)create from the serialized
// pModel+pTransform; bUpdated -> fold sCameraTransform/sModelTransform + placement into pTransform
// (CCFBTransform::Set bumps the version, the mesh's transform functor picks it up); a parent-scene
// mesh is drawn BY the parent -- only an own-view CModelDraw draws p3DView here.
void CModelDraw::Draw( CWindow *pWindow, const STime &sTime, NGScene::I2DGameView *pView )
{
	if ( !pModel )
		return;

	SRect sScrWindow( sWindow );
	SPoint sScrPosition( sWindow.x1, sWindow.y1 );
	if ( pWindow && !pWindow->ClientToScreen( &sScrPosition, &sScrWindow, false ) )
	{
		pRender = 0;
		return;
	}

	if ( !IsValid( pRender ) )
		pRender = p3DView->CreateMesh( pModel, pTransform );

	if ( bUpdated || S2Platform::Display().revision != nLayoutRevision )
	{
		bUpdated = false;
		nLayoutRevision = S2Platform::Display().revision;
		SFBTransform sResult;
		MakeModelTransform( &sResult, sScrPosition, SPoint( sWindow.Width(), sWindow.Height() ), sCameraTransform, sModelTransform );
		pTransform->Set( sResult );
	}

	if ( bParentScene )
		return;

	SRect s2DScrWindow( sScrWindow );
	SPoint s2DScrPosition( sScrPosition );
	SPoint sRealSize( sWindow.Width(), sWindow.Height() );
	if ( pWindow )
	{
		pWindow->VirtualToScreen( &s2DScrPosition, &s2DScrWindow );
		pWindow->VirtualToScreen( &sRealSize, 0 );
	}
	else
	{
		const CVec2 &vScreenRect = pView->GetViewportSize();

		float fXCoef = S2UI::Scale();
		float fYCoef = S2UI::Scale();
		sRealSize.x *= fXCoef;
		sRealSize.y *= fYCoef;
		s2DScrPosition.x *= fXCoef;
		s2DScrPosition.y *= fYCoef;
		s2DScrWindow.x1 *= fXCoef;
		s2DScrWindow.y1 *= fYCoef;
		s2DScrWindow.x2 *= fXCoef;
		s2DScrWindow.y2 *= fYCoef;
	}

	CRectLayout sLayout;
	sLayout.AddRect( 0, 0, sRealSize.x, sRealSize.y, CTRect<float>( 0, 0, 0, 0 ) );
	pView->CreateDynamicClearRects( sLayout, s2DScrPosition, s2DScrWindow, 1.0f );
	pView->Flush();

	CTransformStack ts;
	MakeProjection( &ts );

	NGScene::IGameView::SDrawInfo drawInfo;
	drawInfo.pTS = &ts;
	drawInfo.vOrigin = CVec2( sScrWindow.x1 / float(S2UI::Width()), sScrWindow.y1 / float(S2UI::Height()) );
	drawInfo.vSize = CVec2( sScrWindow.Width() / float(S2UI::Width()), sScrWindow.Height() / float(S2UI::Height()) );
	drawInfo.bOverlay = true;
	p3DView->Draw( drawInfo );

	pView->CreateDynamicClearRects( sLayout, s2DScrPosition, s2DScrWindow, 0.0f );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace
using namespace NUI;
REGISTER_SAVELOAD_CLASS( 0xB0814160, CTextDraw )
REGISTER_SAVELOAD_CLASS( 0xB0814161, CImageDraw )
REGISTER_SAVELOAD_CLASS( 0xB0814163, CModelDraw )
