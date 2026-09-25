#ifndef __GANIMFORMAT_H_
#define __GANIMFORMAT_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "GResource.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NAnimation
{
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SBone
{
	string szName; // bone name
	int nParent; // parent index in array of bones (default)
	CVec3 pos; // default pose position (pivot)
	CQuat rot; // default pose orientation
	CVec3 scale; // scale for this bone

	int operator&( CStructureSaver &f )
	{
		f.Add( 1, &szName );
		f.Add( 2, &nParent );
		f.Add( 3, &pos );
		f.Add( 4, &rot );
		f.Add( 5, &scale );
		return 0;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SAnimationHeader
{
	vector< int > indices; // bone indices in skeleton
	float fLength; // animation length in keys
	float fFrameRate; // frame rate
	int nRoots; // number of (pos,rot) bones
	int nBones; // number of (rot) bones
	int nAddBones; // number of (parent,pos,rot) bones;
	bool bScale; // if true, then (nRoots + nBones) is number of (pos,rot,scale) bones
	
	SAnimationHeader() { nRoots = nBones = nAddBones = 0; fLength = 0; fFrameRate = 30; bScale = false; }
	int operator&( CStructureSaver &f )
	{
		f.Add( 1, &indices );
		f.Add( 2, &fLength );
		f.Add( 3, &fFrameRate );
		f.Add( 4, &nRoots );
		f.Add( 5, &nBones );
		f.Add( 6, &nAddBones );
		f.Add( 7, &bScale );
		return 0;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SRootAnimKey
{
	CVec3 pos;
	CQuat rot;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SBoneAnimKey
{
	CQuat rot;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SAddBoneAnimKey
{
	int nParent;
	CVec3 pos;
	CQuat rot;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SMSRAnimKey
{
	CVec3 pos;
	CQuat rot;
	CVec3 scale;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CFileSkeletonInfo : public CObjectBase
{
	OBJECT_BASIC_METHODS(CFileSkeletonInfo);
public:
	vector< SBone > bones;
	bool bScale;

	CFileSkeletonInfo() {}
	const int GetBoneIndex( const char *pszName );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CFileAnimationInfo : public CObjectBase
{
	OBJECT_BASIC_METHODS(CFileAnimationInfo);
public:
	SAnimationHeader hdr;
	vector< SRootAnimKey > keysRoots;
	vector< SBoneAnimKey > keysBones;
	vector< SAddBoneAnimKey > keysAddBones;
	vector< SMSRAnimKey > keysMSR;

	CFileAnimationInfo() {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CFileSkeleton: public NGScene::CResourceLoader<int, CFileSkeletonInfo>
{
	OBJECT_BASIC_METHODS(CFileSkeleton);
protected:
	virtual void Recalc();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CFileAnimation: public NGScene::CResourceLoader<int, CFileAnimationInfo>
{
	OBJECT_BASIC_METHODS(CFileAnimation);
protected:
	virtual void Recalc();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CFileLocators: public NGScene::CResourceLoader<int, CFileSkeletonInfo>
{
	OBJECT_BASIC_METHODS(CFileLocators);
protected:
	virtual void Recalc();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../FileIO/PortableAnimationWire.h"

static_assert(sizeof(NAnimation::SBoneAnimKey) == 16, "bone key wire size");
static_assert(sizeof(NAnimation::SRootAnimKey) == 28, "root key wire size");
static_assert(sizeof(NAnimation::SAddBoneAnimKey) == 32, "added bone key wire size");
static_assert(sizeof(NAnimation::SMSRAnimKey) == 40, "scaled key wire size");

namespace S2FileIO {
template<>
struct StructureFieldCodec<NAnimation::SBoneAnimKey, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 16;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAnimation::SBoneAnimKey* value) {
    if (!value) return false;
    float f[4];
    if (!DecodeAnimationFloats(source, length, f, 4)) return false;
    value->rot = CQuat(f[0], f[1], f[2], f[3]);
    return true;
  }
  static bool Encode(const NAnimation::SBoneAnimKey& value,
                     std::uint8_t* destination, std::size_t length) {
    float f[4];
    value.rot.GetComponentsForWire(f);
    return EncodeAnimationFloats(f, 4, destination, length);
  }
};

template<>
struct StructureFieldCodec<NAnimation::SRootAnimKey, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 28;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAnimation::SRootAnimKey* value) {
    if (!value) return false;
    float f[7];
    if (!DecodeAnimationFloats(source, length, f, 7)) return false;
    value->pos = CVec3(f[0], f[1], f[2]);
    value->rot = CQuat(f[3], f[4], f[5], f[6]);
    return true;
  }
  static bool Encode(const NAnimation::SRootAnimKey& value,
                     std::uint8_t* destination, std::size_t length) {
    float f[7] = {value.pos.x, value.pos.y, value.pos.z};
    value.rot.GetComponentsForWire(f + 3);
    return EncodeAnimationFloats(f, 7, destination, length);
  }
};

template<>
struct StructureFieldCodec<NAnimation::SAddBoneAnimKey, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 32;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAnimation::SAddBoneAnimKey* value) {
    if (!value) return false;
    std::int32_t parent = 0;
    float f[7];
    if (!DecodeAnimationParentFloats(source, length, &parent, f, 7)) return false;
    value->nParent = parent;
    value->pos = CVec3(f[0], f[1], f[2]);
    value->rot = CQuat(f[3], f[4], f[5], f[6]);
    return true;
  }
  static bool Encode(const NAnimation::SAddBoneAnimKey& value,
                     std::uint8_t* destination, std::size_t length) {
    float f[7] = {value.pos.x, value.pos.y, value.pos.z};
    value.rot.GetComponentsForWire(f + 3);
    return EncodeAnimationParentFloats(value.nParent, f, 7,
                                        destination, length);
  }
};

template<>
struct StructureFieldCodec<NAnimation::SMSRAnimKey, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 40;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAnimation::SMSRAnimKey* value) {
    if (!value) return false;
    float f[10];
    if (!DecodeAnimationFloats(source, length, f, 10)) return false;
    value->pos = CVec3(f[0], f[1], f[2]);
    value->rot = CQuat(f[3], f[4], f[5], f[6]);
    value->scale = CVec3(f[7], f[8], f[9]);
    return true;
  }
  static bool Encode(const NAnimation::SMSRAnimKey& value,
                     std::uint8_t* destination, std::size_t length) {
    float f[10] = {value.pos.x, value.pos.y, value.pos.z};
    value.rot.GetComponentsForWire(f + 3);
    f[7] = value.scale.x; f[8] = value.scale.y; f[9] = value.scale.z;
    return EncodeAnimationFloats(f, 10, destination, length);
  }
};
} // namespace S2FileIO
#endif
