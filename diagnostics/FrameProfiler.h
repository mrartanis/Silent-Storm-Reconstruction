#pragma once
// Opt-in, main-thread CPU scopes. Nested measurements are inclusive, not additive.
#pragma push_macro("for")
#undef for
#include <chrono>
#include <cstdio>
#include <cstring>
#pragma pop_macro("for")

namespace S2Perf {
enum Stage { App, Events, Interface, World, Scene, UI, Present, Submit,
  VertexPack, Head, Morph, Texture, FaceProcess, FacePhysics, FaceSequence,
  FaceGenerate, FaceSelect, FaceBlend, FaceCompose, GPUUpload, Count };
inline const char* Name(int i) {
  static const char* names[] = {"app","events","interface","world","scene","ui",
    "present","submit","vertex_pack","head","morph","face_texture",
    "face_process","face_physics","face_sequence","face_generate","face_select","face_blend","face_compose","gpu_upload"};
  return names[i];
}
using Clock = std::chrono::steady_clock;
inline double Ms(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b-a).count();
}
struct State {
  FILE* file = nullptr;
  int remaining = 0, frame = 0, presents = 0;
  bool freezeFaces = false;
  double times[Count] = {};
  unsigned calls[Count] = {};
  double interval = -1, gpu = -1, render = -1, waitRender = -1, waitSubmit = -1;
  unsigned packedVertices = 0, packedBytes = 0;
  unsigned gpuVBUpdates=0,gpuIBUpdates=0,gpuUploadBytes=0,gpuRenames=0;
  unsigned drawMerges=0;
  unsigned framebufferCreates=0;
  unsigned draws = 0, views = 0, width = 0, height = 0, gpuFrame = 0;
  Clock::time_point start;
  bool hasPrevious = false;
  Clock::time_point previous;
};
inline State& Get() { static thread_local State state; return state; }
inline void Stop() {
  State& s = Get(); if (s.file) std::fclose(s.file);
  s.file = nullptr; s.remaining = 0;
}
inline bool Start(const char* tag, int frames) {
  State& s = Get();
  if (s.file || frames < 1 || frames > 10000 || !*tag || std::strlen(tag)>80) return false;
  for (const char* p=tag; *p; ++p)
    if (!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='_'||*p=='-')) return false;
  char path[128]; std::snprintf(path,sizeof(path),"_perf_%s.csv",tag);
  // Preserve an earlier capture, including failed/interrupted runs.
  FILE* existing = std::fopen(path,"rb");
  if (existing) { std::fclose(existing); return false; }
  s.file = std::fopen(path,"wb"); if (!s.file) return false;
  s.remaining=frames; s.frame=0; s.hasPrevious=false;
  std::fprintf(s.file,"frame,frame_ms,presents,freeze_faces,width,height,gpu_ms,render_ms,wait_render_ms,wait_submit_ms,draws,views,gpu_frame,packed_vertices,packed_bytes,frame_interval_ms,gpu_vb_updates,gpu_ib_updates,gpu_upload_bytes,gpu_renames,draw_merges,framebuffer_creates");
  for(int i=0;i<Count;++i) std::fprintf(s.file,",%s_ms,%s_calls",Name(i),Name(i));
  std::fprintf(s.file,"\n"); return true;
}
inline void Begin() {
  State& s=Get(); if (!s.file) return;
  std::memset(s.times,0,sizeof(s.times)); std::memset(s.calls,0,sizeof(s.calls));
  s.framebufferCreates=0;s.drawMerges=0;s.gpuVBUpdates=s.gpuIBUpdates=s.gpuUploadBytes=s.gpuRenames=0; s.presents=0; s.packedVertices=s.packedBytes=0; s.gpu=s.render=s.waitRender=s.waitSubmit=-1;
  s.draws=s.views=s.width=s.height=s.gpuFrame=0;
  s.start=Clock::now();
  s.interval=s.hasPrevious ? Ms(s.previous,s.start) : -1;
  s.previous=s.start; s.hasPrevious=true;
}
inline void End() {
  State& s=Get(); if (!s.file) return;
  const double elapsed=Ms(s.start,Clock::now());
  std::fprintf(s.file,"%d,%.6f,%d,%d,%u,%u,%.6f,%.6f,%.6f,%.6f,%u,%u,%u,%u,%u,%.6f,%u,%u,%u,%u,%u,%u",
    s.frame++,elapsed,s.presents,s.freezeFaces?1:0,s.width,s.height,
    s.gpu,s.render,s.waitRender,s.waitSubmit,s.draws,s.views,s.gpuFrame,s.packedVertices,s.packedBytes,s.interval,s.gpuVBUpdates,s.gpuIBUpdates,s.gpuUploadBytes,s.gpuRenames,s.drawMerges,s.framebufferCreates);
  for(int i=0;i<Count;++i) std::fprintf(s.file,",%.6f,%u",s.times[i],s.calls[i]);
  std::fprintf(s.file,"\n");
  if (--s.remaining==0) Stop();
}
struct Scope {
  Stage stage; bool active; Clock::time_point start;
  explicit Scope(Stage value):stage(value),active(Get().file!=nullptr) { if(active) start=Clock::now(); }
  ~Scope() { if(active) { Get().times[stage]+=Ms(start,Clock::now()); ++Get().calls[stage]; } }
  Scope(const Scope&)=delete; Scope& operator=(const Scope&)=delete;
};
}
