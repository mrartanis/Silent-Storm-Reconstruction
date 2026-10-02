#ifndef __GLOCALE_H__
#define __GLOCALE_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "DG.h"
#include "FontFormat.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGfx
{
	class CTexture;
};
namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
//! Типы шрифтов
struct SFont
{
	ZDATA
	int nSize;
	string szName;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&nSize); f.Add(3,&szName); return 0; }

	SFont() {}
	SFont( int _nSize, const string &_szName ): nSize( _nSize ), szName( _szName ) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
//! Описание шрифта
class CFontInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CFontInfo);
private:
	ZDATA
	SFont sFont;
	CObj< CPtrFuncBase<NGfx::CTexture> > pTexture;
	CDGPtr< CPtrFuncBase<CFontFormatInfo> > pInfo;
	ZEND int operator&( CStructureSaver &f );
	bool bRuntimeVector = false;
	void RestoreRuntime();
		
public:
	CFontInfo() {}
	CFontInfo( const SFont &_sFont, CPtrFuncBase<NGfx::CTexture> *_pTexture, CPtrFuncBase<CFontFormatInfo> *_pInfo ):
		sFont( _sFont ), pTexture( _pTexture ), pInfo( _pInfo ) {}

	const SFont& GetType() const { return sFont; }
	void SetRuntimeVector(const SFont& request) { bRuntimeVector = true; sFont = request; }
	bool IsVector() const { return bRuntimeVector; }
	CPtrFuncBase<NGfx::CTexture>* GetTexture() { RestoreRuntime(); return pTexture; }
	CPtrFuncBase<CFontFormatInfo>* GetFormatInfo() { RestoreRuntime(); return pInfo; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
//! Локаль
class CTextLocaleInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CTextLocaleInfo);
private:
	ZDATA
	CVec2 vScreenRect;
	vector< CObj<CFontInfo> > fonts;
	vector< CPtr<CFontInfo> > currentFonts;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&vScreenRect); f.Add(3,&fonts); f.Add(4,&currentFonts); return 0; }

protected:
	CFontInfo* SearchFont( const SFont &sFont );

public:
	CTextLocaleInfo();

	void Setup( const CVec2 &vScreenRect );
	CFontInfo* GetFont( const SFont &sFont );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}; // namespace 
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
