#ifndef __GSKELETON_H_
#define __GSKELETON_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../Misc/Geom.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
// typedefs required for animation etc
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGScene
{
typedef vector<SHMatrix> SSkeletonMatrices;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NAnimation
{	
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SBonePose;
typedef vector<SBonePose> SSkeletonPose;
struct SBonePose
{
	int nParent;
	CVec3 pos;
	CQuat rot;
	CVec3 scale;

	SBonePose() { nParent = -1; pos = VNULL3; rot = QNULL; scale = CVec3(1,1,1); }
	void UseParent( const SBonePose &pose );
	void MakeGlobal( const SSkeletonPose &pose );
	void MakeLocal( const SSkeletonPose &pose, int nNewParent );
	void Interpolate( const SSkeletonPose &pose, float val, const SBonePose &bone1, const SBonePose &bone2 );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../FileIO/PortableAnimationWire.h"

static_assert(sizeof(NAnimation::SBonePose) == 44, "bone pose wire size");
namespace S2FileIO {
template<>
struct StructureFieldCodec<NAnimation::SBonePose, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 44;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAnimation::SBonePose* value) {
    if (!value) return false;
    std::int32_t parent = 0;
    float f[10];
    if (!DecodeAnimationParentFloats(source, length, &parent, f, 10))
      return false;
    value->nParent = parent;
    value->pos = CVec3(f[0], f[1], f[2]);
    value->rot = CQuat(f[3], f[4], f[5], f[6]);
    value->scale = CVec3(f[7], f[8], f[9]);
    return true;
  }
  static bool Encode(const NAnimation::SBonePose& value,
                     std::uint8_t* destination, std::size_t length) {
    float f[10] = {value.pos.x, value.pos.y, value.pos.z};
    value.rot.GetComponentsForWire(f + 3);
    f[7] = value.scale.x; f[8] = value.scale.y; f[9] = value.scale.z;
    return EncodeAnimationParentFloats(value.nParent, f, 10,
                                        destination, length);
  }
};
} // namespace S2FileIO
#endif
