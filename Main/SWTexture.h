#ifndef __GSWTEXTURE_H__
#define __GSWTEXTURE_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "DG.h"
#include "GResource.h"
#include "GPixelFormat.h"
#include "..\Misc\2DArray.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGfx
{
	class CTexture;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SBumpPixel
{
	float fDU, fDV;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CSWTextureData: public CObjectBase
{
	OBJECT_BASIC_METHODS(CSWTextureData);
public:
	vector<CArray2D<NGfx::SPixel8888> > mips;
	vector<CArray2D<SBumpPixel> > bumpMips;
	// Layout coordinates stay in database texels even when the loaded image is denser.
	float texelScaleX = 1, texelScaleY = 1;

	void PrepareBump();
	int GetXSize() const { return mips[0].GetXSize(); }
	int GetYSize() const { return mips[0].GetYSize(); }
	float GetLogicalXSize() const { return GetXSize() / texelScaleX; }
	float GetLogicalYSize() const { return GetYSize() / texelScaleY; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CSWTexture : public CResourceLoader<int, CSWTextureData>
{
	OBJECT_BASIC_METHODS(CSWTexture);
	CObj<CFileRequest> pRequest;
	bool bIsReady;
	int logicalWidth = 0, logicalHeight = 0;
	unsigned resourceRevision = 0;
	bool SyncResourceRevision();
	void LoadTexture();
protected:
	virtual void Recalc();
	bool NeedUpdate() override;
public:
	CSWTexture() : bIsReady(false) {}
	bool IsReady();
	void SetLogicalSize( int width, int height ) { logicalWidth = width; logicalHeight = height; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CBilinearTexture: public CPtrFuncBase<CSWTextureData>
{
	OBJECT_BASIC_METHODS(CBilinearTexture);
	ZDATA
	CArray2D<NGfx::SPixel8888> pic;
	int nXSize, nYSize;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pic); f.Add(3,&nXSize); f.Add(4,&nYSize); return 0; }
protected:
	virtual void Recalc();
public:
	CBilinearTexture() {}
	CBilinearTexture( const CArray2D<NGfx::SPixel8888> &_data, int _nXSize, int _nYSize )
		: pic(_data), nXSize(_nXSize), nYSize(_nYSize) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __GTEXTURE_H__
