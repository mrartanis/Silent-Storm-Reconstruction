#pragma once
// Harness-only, main-thread state. Not serialized; no resource or artwork substitution.
#include "TextureResidency.h"
namespace S2UITextureProbe {
struct State {
  int requestedID=0, observedID=0, drawFrames=0, lastFrame=-1;
  int logicalWidth=0, logicalHeight=0, physicalWidth=0, physicalHeight=0, mips=0;
  int holderWidth=0, holderHeight=0, placementX=0, placementY=0;
  float densityX=1, densityY=1;
  bool pending=true, placeholder=true, drawn=false;
};
inline State& Get() { static State state; return state; }
inline void Select(int textureID) { Get()=State{}; Get().requestedID=textureID; }
} // namespace S2UITextureProbe
namespace NGScene { void ReleaseUITextureProbeResource(); }
