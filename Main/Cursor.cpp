#include "StdAfx.h"
#include "DisplayLayout.h"
#include "GResource.h"
#include "GPixelFormat.h"
#include "mmpFormat.h"
#include <set>
#include <tuple>
#include "../Game/Platform.h"
#include "Gfx.h"
#include "GSceneUtils.h"
#include "G2DView.h"
#include "..\Input\Bind.h"
#include "..\DBFormat\DataFormat.h"
#include "..\DBFormat\DataInterface.h"
#include "Interface.h"
#include "UIWrap.h"
#include "UIML.h"     // BUG 8: NUI::IML / CreateML -- retail draws the cursor caption through the CML engine (outline + pt-size)
#include "..\MiscDll\Commands.h"      // REGISTER_VAR_EX (ui_hwcursor)
#include "..\FileIO\BasicChunk1.h"    // START_REGISTER / FINISH_REGISTER
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGfx
{
	HWND GetHWND();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NUI
{
static bool bHWCursor = true;
void UpdateCursorMode() { S2Platform::UseNativeCursor(bHWCursor); }
// Use the same texture, orientation, dimensions and anchor as CImageDraw.
static bool SelectCursorImage(NDb::CUICursor* cursor)
{
	if (!IsValid(cursor) || !IsValid(cursor->pUITexture)) return false;
	NDb::CUITexture* ui = cursor->pUITexture;
	NDb::CTexture* texture = ui->pTextures[NDb::UIM_1024x768];
	if (!IsValid(texture)) for (int i=0;i<4;++i)
		if (IsValid(ui->pTextures[i])) { texture=ui->pTextures[i]; break; }
	if (!IsValid(texture) || texture->nWidth<=0 || texture->nHeight<=0) return false;
	const int width=Max(1,int(ui->nWidth*S2UI::Scale()+.5f));
	const int height=Max(1,int(ui->nHeight*S2UI::Scale()+.5f));
	const int hotX=Min(width-1,Max(0,int(width*cursor->nCenterX+.5f)));
	const int hotY=Min(height-1,Max(0,int(height*cursor->nCenterY+.5f)));
	const int id=cursor->GetRecordID();
	if (S2Platform::SelectNativeCursor(id,width,height,hotX,hotY)) return true;
	using Key=std::tuple<int,int,int,int,int>;
	static std::set<Key> failed;
	const Key key(id,width,height,hotX,hotY);
	if (failed.count(key)) return false;
	bool ok=false;
	try {
		// The decompressed alias also covers DXT UI assets when DXT rendering is enabled.
		const int textureID=texture->GetRecordID() | (texture->bIsDXT ? 0x01000000 : 0);
		CObj<NGScene::CFileRequest> file=new NGScene::CFileRequest("Textures",textureID);
		file->Read();
		CDataStream* stream=file->GetStream();
		stream->Seek(0);
		SMMPFileHeader header;
		stream->Read(&header,sizeof(header));
		if (header.dwSignature==MMP_SIGNATURE && header.format==NGfx::CF_A8R8G8B8 &&
		    header.nSizeX>=texture->nWidth && header.nSizeY>=texture->nHeight &&
		    header.nSizeX<=4096 && header.nSizeY<=4096 && width<=512 && height<=512 &&
		    stream->GetSize()>=int(sizeof(header))+header.nSizeX*header.nSizeY*int(sizeof(DWORD))) {
			std::vector<DWORD> stored(header.nSizeX*header.nSizeY);
			stream->Read(stored.data(),int(stored.size()*sizeof(DWORD)));
			std::vector<DWORD> pixels(texture->nWidth*texture->nHeight);
			for (int y=0;y<texture->nHeight;++y)
				memcpy(&pixels[y*texture->nWidth],&stored[(texture->nHeight-1-y)*header.nSizeX],texture->nWidth*sizeof(DWORD));
			ok=S2Platform::SelectNativeCursor(id,width,height,hotX,hotY,pixels.data(),texture->nWidth,texture->nHeight);
		}
	} catch (...) { }
	if (!ok) {
		failed.insert(key);
		fprintf(stderr,"Native cursor %d (%dx%d): using software fallback\n",id,width,height);
	}
	return ok;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const int N_TRANSITION_TIME	= 250;
////////////////////////////////////////////////////////////////////////////////////////////////////
static CVec2 vCursorPos = CVec2( 0, 0 );
////////////////////////////////////////////////////////////////////////////////////////////////////
// CCursor
////////////////////////////////////////////////////////////////////////////////////////////////////
class CCursor: public ICursor
{
	OBJECT_BASIC_METHODS(CCursor);
protected:
	NInput::CBind bindX, bindY;
	// transient (retail v1.2 deleted the Jan03 mouse-easing members; ctor re-seeds them)
	float fThreshold1, fThreshold2, fAcceleration;
	STime sLastUpdateTime;
	CTimeCounter sTimer;
	CDGPtr<CCTime> pTimer;
	// BUG 8: retail draws the cursor ToHit/AP caption through the CML markup engine (IML), NOT the legacy
	// GText CTextDraw -- so it renders the DB-string markup (Courier, 16pt, colour, 1px outline) like retail.
	// The shared CTextDraw stays GText (14 other consumers); only the cursor gets its own IML. Transient.
	CObj<IML> pTextML;
	ZDATA
	int nDisableCount; // retail Enable/Disable nesting counter (gates ProcessEvent @0xd60a0); serialized first
	bool bShow;
	STime sTransitionTime;
	SCursorInfo sInfo;
	SCursorInfo sOldInfo;
	CObj<CTextDraw> pText;
	CObj<CImageDraw> pImage;
	CObj<CImageDraw> pOldImage;
	// retail v1.2 wire @0xd7550: {2 nDisableCount, 3 bShow, 4 sTransitionTime, 5 sInfo, 6 sOldInfo,
	// 7 pText, 8 pImage, 9 pOldImage}. The old Jan03 table put bShow@2 -> it read retail's
	// nDisableCount low byte (0) -> cursor permanently hidden after loading a retail save.
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&nDisableCount); f.Add(3,&bShow); f.Add(4,&sTransitionTime); f.Add(5,&sInfo); f.Add(6,&sOldInfo); f.Add(7,&pText); f.Add(8,&pImage); f.Add(9,&pOldImage); return 0; }

protected:
	float AccelerateAxis( float fDelta, const STime &sDelta );

public:
	CCursor( bool bShow = true );

	const CVec2& GetPos() const;
	void SetPos( const CVec2 &vCursorPos );
	
	const SCursorInfo& GetCursor() const;
	void SetCursor( const SCursorInfo &sInfo );

	void SetCursorText( const wstring &wsText );

	void Update();

	void ProcessEvent( const NInput::SEvent &sEvent );
	void Draw( const STime &sTime, NGScene::I2DGameView *pView );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
ICursor* ICursor::Create( bool bShowCursor, CVec2 vBegPos )
{
	if ( ( vBegPos.x > 0 ) && ( vBegPos.y > 0 ) )
		vCursorPos = vBegPos;

	return new CCursor( bShowCursor );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CCursor
////////////////////////////////////////////////////////////////////////////////////////////////////
CCursor::CCursor( bool _bShow ):
	bindX( "cursor_x" ), bindY( "cursor_y" ), nDisableCount(0), bShow(_bShow)
{
	pText = new CTextDraw();
	pTextML = CreateML();   // BUG 8: the cursor's own CML markup text (renders the DB-string font/colour/outline)
	pImage = new CImageDraw();
	pOldImage = new CImageDraw();

	pTimer = sTimer.GetTime();
	sLastUpdateTime = 0;

	int pdwParams[3];
	S2Platform::MouseAcceleration( &pdwParams[0], &pdwParams[1], &pdwParams[2] );

	fThreshold1 = pdwParams[0];
	fThreshold2 = pdwParams[1];
	fAcceleration = pdwParams[2];

	if ( fAcceleration == 0 ) /// CRAP: WinME WTF ?
		fAcceleration = 1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const CVec2& CCursor::GetPos() const
{
	return vCursorPos;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CCursor::SetPos( const CVec2 &_vCursorPos )
{
	vCursorPos = _vCursorPos;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const SCursorInfo& CCursor::GetCursor() const
{
	return sInfo;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CCursor::SetCursor( const SCursorInfo &_sInfo )
{
	if ( sInfo.pCursor != _sInfo.pCursor )
	{
		sOldInfo = sInfo;
		sTransitionTime = 0;
	}

	sInfo = _sInfo;
	pText->SetText( _sInfo.wsText );
	if ( IsValid( pTextML ) )
		pTextML->SetText( _sInfo.wsText, 0 );   // 0 = process the <font>/<color> tags from the DB strings
}
////////////////////////////////////////////////////////////////////////////////////////////////////
float CCursor::AccelerateAxis( float fDelta, const STime &sDelta )
{
	float fSecondDelta = fabs( fDelta ) * 1000 / sDelta;

	float fInc = fDelta;
	if ( fSecondDelta > fThreshold1 )
		fInc *= fAcceleration;
	if ( fSecondDelta > fThreshold2 )
		fInc *= 2;

	return fInc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CCursor::Update()
{
	sTimer.Advance( true, S2Platform::Milliseconds() );
	pTimer.Refresh();
	STime sDelta = pTimer->GetValue() - sLastUpdateTime;
	sLastUpdateTime = pTimer->GetValue();
	if (S2Platform::NativeCursorEnabled()) {
		bindX.GetDelta(); bindY.GetDelta();
		if (S2Platform::Active()) S2Platform::CursorPosition(&vCursorPos.x,&vCursorPos.y);
		return;
	}
	if ( sDelta == 0 )
		return;

	const CVec2 &vSize = NGfx::GetScreenRect();
	vCursorPos.x += AccelerateAxis( bindX.GetDelta() * 250.0f, sDelta );
	vCursorPos.y += AccelerateAxis( bindY.GetDelta() * 250.0f, sDelta );

	vCursorPos.x = Max( vCursorPos.x, 0.0f );
	vCursorPos.x = Min( vCursorPos.x, vSize.x - 1 ); 
	vCursorPos.y = Max( vCursorPos.y, 0.0f );
	vCursorPos.y = Min( vCursorPos.y, vSize.y - 1 );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CCursor::Draw( const STime &sTime, NGScene::I2DGameView *pView )
{
	const bool nativeImage=S2Platform::NativeCursorEnabled() && bShow && SelectCursorImage(sInfo.pCursor);
	S2Platform::NativeCursorVisible(nativeImage);
	if ( sTransitionTime == 0 )
		sTransitionTime = sTime;

	if ( bShow )
	{
		float fCoeff = float( sTime - sTransitionTime ) / N_TRANSITION_TIME;
		if ( fCoeff > 1.0f )
		{
			fCoeff = 1.0f;
			sOldInfo.pCursor = 0;
		}

		CVec2 vVirtCursorPos( S2UI::FromPixel(vCursorPos.x), S2UI::FromPixel(vCursorPos.y) );

		// retail draws from the UICursors RECORD: texture = pCursor->pUITexture, anchor = retail
		// CalcCursorPos @0xd6200: sPos = (int)( vVirt - texSize * nCenter ), nCenterX/Y from the record.
		if ( IsValid( sInfo.pCursor ) && IsValid( sInfo.pCursor->pUITexture ) )
		{
			NDb::CUITexture *pTex = sInfo.pCursor->pUITexture;
			SPoint sPos( vVirtCursorPos.x - float( pTex->nWidth ) * float( sInfo.pCursor->nCenterX ), vVirtCursorPos.y - float( pTex->nHeight ) * float( sInfo.pCursor->nCenterY ) );
			pImage->SetWindow( SRect( sPos.x, sPos.y, sPos.x + pTex->nWidth, sPos.y + pTex->nHeight ) );
			pImage->SetImage( pTex );
			pImage->SetColor( NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF * fCoeff ) );
			if (!nativeImage) pImage->DrawAtPixels(sTime, pView, CVec2(vCursorPos.x-pTex->nWidth*sInfo.pCursor->nCenterX*S2UI::Scale(), vCursorPos.y-pTex->nHeight*sInfo.pCursor->nCenterY*S2UI::Scale()));

			// BUG 8: draw the caption through the cursor's OWN CML markup engine (retail cursor path), so the
			// DB-string markup renders as retail does -- Courier, 16pt, the DB colour, and the 1px black
			// outline (which GText cannot draw). Scale the virtual (1024x768) text anchor to screen, generate
			// the ML at full width (the short caption never wraps), and Render.
			if ( IsValid( pTextML ) )
			{
				CVec2 vScr = pView->GetViewportSize();
				SPoint sTextVirt( sPos.x + pTex->nWidth, sPos.y );
				SPoint sScrPos(vCursorPos.x+pTex->nWidth*(1-sInfo.pCursor->nCenterX)*S2UI::Scale(), vCursorPos.y-pTex->nHeight*sInfo.pCursor->nCenterY*S2UI::Scale());
				pTextML->Generate( pView, (int)vScr.x );
				SRect sScrWindow( sScrPos.x, sScrPos.y, (int)vScr.x, (int)vScr.y );
				pTextML->Render( pView, sScrPos, sScrWindow );
			}
		}
		if ( !nativeImage && IsValid( sOldInfo.pCursor ) && IsValid( sOldInfo.pCursor->pUITexture ) )
		{
			NDb::CUITexture *pTex = sOldInfo.pCursor->pUITexture;
			SPoint sPos( vVirtCursorPos.x - float( pTex->nWidth ) * float( sOldInfo.pCursor->nCenterX ), vVirtCursorPos.y - float( pTex->nHeight ) * float( sOldInfo.pCursor->nCenterY ) );
			pOldImage->SetWindow( SRect( sPos.x, sPos.y, sPos.x + pTex->nWidth, sPos.y + pTex->nHeight ) );
			pOldImage->SetImage( pTex );
			pOldImage->SetColor( NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF * ( 1.0f - fCoeff ) ) );
			pOldImage->DrawAtPixels(sTime, pView, CVec2(vCursorPos.x-pTex->nWidth*sOldInfo.pCursor->nCenterX*S2UI::Scale(), vCursorPos.y-pTex->nHeight*sOldInfo.pCursor->nCenterY*S2UI::Scale()));
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CCursor::ProcessEvent( const NInput::SEvent &sEvent )
{
	if (S2Platform::NativeCursorEnabled() && sEvent.mMessage.hasPointer)
		vCursorPos=CVec2(sEvent.mMessage.pointerX,sEvent.mMessage.pointerY);
	bindX.ProcessEvent( sEvent );
	bindY.ProcessEvent( sEvent );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class CEditorCursor: public CCursor
{
	OBJECT_BASIC_METHODS(CEditorCursor);
public:
	void ProcessEvent( const NInput::SEvent &eEvent );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
void CEditorCursor::ProcessEvent( const NInput::SEvent &eEvent )
{
	if (S2Platform::NativeCursorEnabled()) { CCursor::ProcessEvent(eEvent); return; }
	POINT sPoint;
	float x = 0, y = 0;
	S2Platform::CursorPosition( &x, &y );
	sPoint.x = static_cast<LONG>(x); sPoint.y = static_cast<LONG>(y);

	CVec2 scrSize = NGfx::GetScreenRect();
	vCursorPos.x = sPoint.x;
	vCursorPos.y = sPoint.y;
	vCursorPos.x = Max( vCursorPos.x, 0.0f );
	vCursorPos.x = Min( vCursorPos.x, scrSize.x ); 
	vCursorPos.y = Max( vCursorPos.y, 0.0f );
	vCursorPos.y = Min( vCursorPos.y, scrSize.y );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
ICursor* ICursor::CreateEditorCursor()
{
	return new CEditorCursor;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// The existing graphics checkbox and saved config select native or software cursors.
START_REGISTER(Cursor)
	REGISTER_VAR_EX( "ui_hwcursor", NGlobal::VarBoolHandler, &bHWCursor, 1, true )
FINISH_REGISTER
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace NUI;
REGISTER_SAVELOAD_CLASS( 0x00821183, CCursor );
REGISTER_SAVELOAD_CLASS( 0xA2812160, CEditorCursor );
////////////////////////////////////////////////////////////////////////////////////////////////////
