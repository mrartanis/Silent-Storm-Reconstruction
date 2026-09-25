#ifndef __GRenderModes_H_
#define __GRenderModes_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../FileIO/PortableRenderStateWire.h"

namespace NGfx
{
	class CTexture;
}
namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// ON THE WIRE (CGameView tag 25, CGScene tag 11). Values MUST match the retail PDB enum:
// SHOW_PL_OVERDRAW=4, LIGHTMAPPED=5, BEST=6 -- dev previously lacked slot 4, so a retail save's
// renderMode=6 (SRM_BEST) deserialized as SRM_LAST.
enum ESceneRenderMode
{
	SRM_FASTEST,
	SRM_SHOWOCCLUDERS,
	SRM_SHOWLIGHTMAP,
	SRM_SHOWSKYMAP,
	SRM_SHOW_PL_OVERDRAW,
	SRM_LIGHTMAPPED,
	SRM_BEST,
	SRM_LAST
};
enum ERenderPath
{
	RP_TNL,
	RP_FASTEST,
	RP_SHOWOCCLUDERS,
	RP_GF2,
	RP_UPDATE_CL,
	RP_SHOWLIGHTMAPPED,
	RP_GF2_CL,
	RP_GF3_CL
};
inline bool IsUingRegisters( ERenderPath rp ) { return rp >= RP_GF2; }
inline bool IsUsingCacheLighting( ERenderPath rp ) { return rp >= RP_UPDATE_CL; }
inline bool IsUsingShadows( ERenderPath rp ) { return rp >= RP_GF2; }
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EFogMode
{
	FOG_NONE,
	FOG_PERVERTEX,
	FOG_DYNAMIC,
	FOG_LAST
};
enum EHSRMode
{
	HSR_NONE,
	HSR_FAST,
	HSR_DYNAMIC,   // retail: keeps occlusion culling during camera movement (gfx_hsr 2 = default)
	HSR_LAST
};
enum ETransparentMode
{
	TRM_NONE,
	TRM_NORMAL,
	TRM_ONLY,
	TRM_LAST
};
enum EScenePartsSet
{
	SPS_STATIC = 1,
	SPS_DYNAMIC = 2,
	SPS_ALL = 3
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SFogParams
{
	float fDist;
	CVec3 vFogColor;
	float fHeight, fDensity;
	CVec3 vWaterColor;
	float fCameraHeight;
	float fVapourNoiseParam, fVapourSpeed, fVapourSwitchTime;
	float fTime;
	float fDistStart;
	float fVapourHeightStart;
	
	void SetDefaults()
	{
		memset( this, 0, sizeof(SFogParams) );
		fDist = 10000; fDistStart = 100;
	}
	bool IsSameParams( const SFogParams &a ) const 
	{ 
		return 
			fDist == a.fDist && 
			vFogColor == a.vFogColor && 
			fHeight == a.fHeight && 
			fDensity == a.fDensity && 
			vWaterColor == a.vWaterColor &&
			fabs( fCameraHeight - a.fCameraHeight ) < 0.1f &&
			fDistStart == a.fDistStart &&
			fVapourHeightStart == a.fVapourHeightStart;
	}
	bool operator==( const SFogParams &a ) const { return memcmp( &a, this, sizeof(SFogParams) ) == 0; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}

static_assert(sizeof(NGScene::SFogParams) == 64, "fog parameters wire size");
namespace S2FileIO {
template<>
struct StructureFieldCodec<NGScene::SFogParams, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 64;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NGScene::SFogParams* value) {
    if (!value) return false;
    float f[16];
    if (!DecodeRenderFloatFields<16>(source, length, f)) return false;
    value->fDist = f[0];
    value->vFogColor = CVec3(f[1], f[2], f[3]);
    value->fHeight = f[4]; value->fDensity = f[5];
    value->vWaterColor = CVec3(f[6], f[7], f[8]);
    value->fCameraHeight = f[9];
    value->fVapourNoiseParam = f[10]; value->fVapourSpeed = f[11];
    value->fVapourSwitchTime = f[12]; value->fTime = f[13];
    value->fDistStart = f[14]; value->fVapourHeightStart = f[15];
    return true;
  }
  static bool Encode(const NGScene::SFogParams& value,
                     std::uint8_t* destination, std::size_t length) {
    const float f[16] = {
        value.fDist, value.vFogColor.x, value.vFogColor.y, value.vFogColor.z,
        value.fHeight, value.fDensity,
        value.vWaterColor.x, value.vWaterColor.y, value.vWaterColor.z,
        value.fCameraHeight, value.fVapourNoiseParam, value.fVapourSpeed,
        value.fVapourSwitchTime, value.fTime, value.fDistStart,
        value.fVapourHeightStart};
    return EncodeRenderFloatFields<16>(f, destination, length);
  }
};
} // namespace S2FileIO
#endif
