#ifndef __BuildingClip_H_
#define __BuildingClip_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../DBFormat/DataConst.h"
#include "DiscretePos.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NDb
{
	class CModel;
	class CGeometry;
	class CAIGeometry;
}
namespace NBuilding
{
	class CMixedMaterial;
////////////////////////////////////////////////////////////////////////////////////////////////////
const int UNBROKEN_BLOCK32 = 0xffffffff;
////////////////////////////////////////////////////////////////////////////////////////////////////
enum ESide
{
	FRONT = 0,
	BACK,
	LEFT,
	RIGHT,
	TOP,
	BOTTOM,
	INTERNAL,
	ESIZE,
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// РєР»РёРїРёРЅС„Рѕ РґР»СЏ СЃС‚РµРЅ Рё РїРµСЂРµРєСЂС‹С‚РёР№; ObjectInfo С€Р°СЂРёС‚СЃСЏ РїРѕ СЌС‚РѕР№ РёРЅС„Рµ
// СЃРѕРґРµСЂР¶РёС‚СЃСЏ РёРЅС„РѕСЂРјР°С†РёСЏ Рѕ РІРёРґРёРјС‹С… РєСѓСЃРѕС‡РєР°С… Рё РєР°РєРёРјРё РїР»РѕСЃРєРѕСЃС‚СЏРјРё РєР»РёРїР°С‚СЊ РєСЂР°СЏ
enum EClipType
{
	CLIP_WALL,
	CLIP_FLOOR,	
};
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
inline int GetPieceHashID( int x, int y, int z )
{
	return z << 4 | y << 2 | x;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void GetPieceCoords( int nHashID, int *px, int *py, int *pz )
{
	*px = nHashID & 0x3;
	*py = (nHashID >> 2) & 0x3;
	*pz = (nHashID >> 4) & 0x7;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline int GetPartHashID( int partX, int partY, int partZ )
{
	return partZ << 13 | partY << 10 | partX << 7;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void GetPartCoords( int nHashID, int *pPartX, int *pPartY, int *pPartZ )
{
	*pPartX = (nHashID >> 7) & 0x7;
	*pPartY = (nHashID >> 10) & 0x7;
	*pPartZ = (nHashID >> 13) & 0x7;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool IsUnbrokenPart( int nHashID )
{
	return (nHashID & 0x7f) == 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SClipInfo
{
	CPtr<NDb::CGeometry> pGeometry;
	int nSubBlockID;
	CVec3 ptPos;
	int   nRotationID;
	CPtr<CMixedMaterial> pMaterials[NDb::N_MODEL_MATERIALS];
	//short nRooms[2];
	union
	{
		int nClip;				// РґР»СЏ СЃС‚РµРЅС‹: СЃС‚Р°СЂС€РёРµ 2 Р±Р°Р№С‚Р° - РґР»РёРЅР°, С‚РѕР»С‰РёРЅР°; РјР»Р°РґС€РёРµ - РєР»РёРїР°СЋС‰Р°СЏ РїР»РѕСЃРєРѕСЃС‚СЊ
		int nNeighbors;   // РґР»СЏ СЃРїР»РѕС€РЅРѕРіРѕ РѕР±СЉРµРєС‚Р° (РїРµСЂРµРєСЂС‹С‚РёР№, РѕРєРѕРЅ Рё С‚.Рґ.)
	};
	DWORD dwParts;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// z РѕРїСЂРµРґРµР»СЏРµС‚ РѕРґРёРЅ РёР· 5 РІРѕР·РјРѕР¶РЅС‹С… СЃР»РѕРµРІ
// С‚Р°Рє Р¶Рµ СѓС‡РёС‚С‹РІР°РµС‚СЃСЏ СЂР°Р·РЅРѕРµ СЂР°СЃРїРѕР»РѕР¶РµРЅРёРµ СѓР·Р»РѕРІ РїСЂРё С‡РµС‚РЅС‹С…\РЅРµС‡РµС‚РЅС‹С… z
// x=(0..2) y=(0..2) z=(0..4)
inline DWORD GetPartBit( int x, int y, int z )
{
	if ( ( ( x + y + z ) & 1 ) == 0 )
		return 0;
	return 1 << ( x + y * 3 + (z/2) * 9 );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UnpackParts( vector<int> *pParts, DWORD dwParts, int nSubPartID );
void SplitOptimized( vector<int> *pPartsHash );
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SAIClipInfo
{
	CPtr<NDb::CAIGeometry> pAIGeometry;
	int nSubBlock;
	CVec3 ptPos;
	int   nRotation;
	DWORD dwParts;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}
#endif
