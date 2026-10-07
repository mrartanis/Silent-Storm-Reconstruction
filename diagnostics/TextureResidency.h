#pragma once
// Opt-in main-thread observations; no resource ownership or save-file state.
#pragma push_macro("for")
#undef for
#include <cstdlib>
#include <map>
#include <cstdint>
#pragma pop_macro("for")
namespace S2TextureDiag {
inline bool Enabled() {
  static const bool enabled = [] { const char* p=std::getenv("S2_TEXTURE_DIAGNOSTICS"); return p && *p=='1'; }();
  return enabled;
}
struct Entry { int frame=0,width=0; bool bump=false,placeholder=false; };
struct State {
  std::map<const void*,Entry> terrain,files;
  std::uint64_t evictions=0,generated128=0,generated256=0;
  std::uint64_t generatedHD=0;
  std::uint64_t stressFallbacks=0,pendingFallbacks=0,budgetFallbacks=0;
  std::uint64_t fullFileLoads=0,lowFileLoads=0,gpuBytes=0;
  unsigned gpuTextures=0; int presentedFrame=0;
};
inline State& Get() { static State state; return state; }
inline void Observe(bool terrain,const void* node,int frame,int width,bool bump,bool placeholder) {
  if(Enabled()) (terrain ? Get().terrain : Get().files)[node]={Get().presentedFrame,width,bump,placeholder};
}
}
