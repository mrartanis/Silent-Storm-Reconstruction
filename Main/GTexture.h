#ifndef __GTEXTURE_H__
#define __GTEXTURE_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "DG.h"
#include "GResource.h"
#include "GTextureLoader.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGfx
{
	class CTexture;
	class CCubeTexture;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
struct STextureKey
{
	enum EEFlags
	{
		TK_WRAP = 1,
		TK_TRANSPARENT = 2
	};
	int nID;
	int nFlags;

	STextureKey() {}
	STextureKey( int _nID, int _nFlags = 0 ): nID(_nID), nFlags(_nFlags) {}

	bool operator==( const STextureKey &a ) const { return nID == a.nID && nFlags == a.nFlags; }
};
struct STextureKeyHash
{
	int operator()( const STextureKey &k ) const { return k.nID; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// texture loader from disk
class CFileTexture : public CResourceLoader<STextureKey, NGfx::CTexture>
{
	OBJECT_BASIC_METHODS(CFileTexture);
	typedef CResourceLoader<STextureKey, NGfx::CTexture> TParent;
	bool bIsFakeTexture;
	CObj<CFileRequest> pRequest;
	unsigned resourceRevision = 0;
protected:
	virtual void Recalc();
	virtual bool NeedUpdate();
public:
	CFileTexture() : bIsFakeTexture(false) {}
	void CreateChecker();
	bool IsPendingForDiagnostics() const { return IsValid(pRequest) && !pRequest->IsReady(); }
	bool IsPlaceholderForDiagnostics() const { return bIsFakeTexture; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CFileCubeTexture : public CResourceLoader<int, NGfx::CCubeTexture>
{
	OBJECT_BASIC_METHODS(CFileCubeTexture);
protected:
	virtual void Recalc();
public:
	void CreateChecker();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CColorTexture : public CPtrFuncBase<NGfx::CTexture>
{
	OBJECT_BASIC_METHODS(CColorTexture);
	ZDATA
	CVec4 vColor;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&vColor); return 0; }
protected:
	virtual void Recalc();
public:
	CColorTexture() {}
	CColorTexture( const CVec4 &_v ) : vColor(_v) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../FileIO/PortableResourceKeyWire.h"

static_assert(sizeof(NGScene::STextureKey) == 8, "texture key wire size");

namespace S2FileIO {
template<>
struct StructureFieldCodec<NGScene::STextureKey, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 8;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NGScene::STextureKey* value) {
    if (!value) return false;
    ResourceKeyFields fields{};
    if (!DecodeResourceKey(source, length, &fields)) return false;
    value->nID = fields.id;
    value->nFlags = fields.option;
    return true;
  }
  static bool Encode(const NGScene::STextureKey& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeResourceKey(ResourceKeyFields{value.nID, value.nFlags},
                             destination, length);
  }
};
} // namespace S2FileIO

////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __GTEXTURE_H__
