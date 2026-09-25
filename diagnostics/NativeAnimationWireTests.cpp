#include "../Main/StdAfx.h"
#include "../Main/GAnimFormat.h"
#include "../Main/GSkeleton.h"

#include <cstdint>
#include <cstring>

template<class T>
bool Check(const T& value) {
  using Codec = S2FileIO::StructureFieldCodec<T>;
  if (!Codec::kPortable || Codec::kWireSize != sizeof(T)) return false;
  std::uint8_t wire[64] = {};
  T loaded;
  if (!Codec::Encode(value, wire, sizeof(T)) ||
      std::memcmp(wire, &value, sizeof(T)) != 0 ||
      !Codec::Decode(wire, sizeof(T), &loaded) ||
      std::memcmp(&loaded, &value, sizeof(T)) != 0 ||
      Codec::Decode(wire, sizeof(T) - 1, &loaded) ||
      Codec::Encode(value, wire, sizeof(T) - 1)) return false;
  return true;
}

int main() {
  NAnimation::SBoneAnimKey bone;
  bone.rot = CQuat(0.5f, -1.0f, 2.0f, 0.25f);
  NAnimation::SRootAnimKey root;
  root.pos = CVec3(-3.0f, 4.0f, 1.5f);
  root.rot = bone.rot;
  NAnimation::SAddBoneAnimKey added;
  added.nParent = -17;
  added.pos = root.pos;
  added.rot = root.rot;
  NAnimation::SMSRAnimKey scaled;
  scaled.pos = root.pos;
  scaled.rot = root.rot;
  scaled.scale = CVec3(1.0f, 0.5f, -2.0f);
  NAnimation::SBonePose pose;
  pose.nParent = -17;
  pose.pos = root.pos;
  pose.rot = root.rot;
  pose.scale = scaled.scale;
  return Check(bone) && Check(root) && Check(added) &&
                 Check(scaled) && Check(pose) ? 0 : 1;
}
