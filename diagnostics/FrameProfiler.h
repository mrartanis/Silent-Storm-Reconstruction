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
  FaceGenerate, FaceSelect, FaceBlend, FaceCompose, GPUUpload, GeometryVB,
  GeometryIB, GeometryFlush, GeometryRename, IndexBounds, DrawFlush, RenderApply,
  SceneHSR, SceneList, SceneLights, SceneExecute, SceneTransparent, UIPortraits,
  SceneSort, SceneTriangles, CPUVertexCombine, CPUIndexCombine, CPUPositionTransform,
  CPUMeshCombine, CPULightmapCombine, CPUOtherCombine, Count };
inline const char* Name(int i) {
  static const char* names[] = {"app","events","interface","world","scene","ui",
    "present","submit","vertex_pack","head","morph","face_texture",
    "face_process","face_physics","face_sequence","face_generate","face_select","face_blend","face_compose","gpu_upload",
    "geometry_vb","geometry_ib","geometry_flush","geometry_rename","index_bounds","draw_flush","render_apply",
    "scene_hsr","scene_list","scene_lights","scene_execute","scene_transparent","ui_portraits",
    "scene_sort","scene_triangles","cpu_vertex_combine","cpu_index_combine","cpu_position_transform",
    "cpu_mesh_combine","cpu_lightmap_combine","cpu_other_combine"};
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
  unsigned vbRenames=0, ibRenames=0, vbBlocks=0, ibBlocks=0, scannedIndices=0;
  bool insideUI=false;
  double uiGeometryFlush=0,uiVertexPack=0;
  unsigned combinedFixedVertices=0,combinedMovingVertices=0;
  unsigned partialCopiedBytes=0,partialSkippedBytes=0;
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
  std::fprintf(s.file,",vb_renames,ib_renames,vb_blocks,ib_blocks,scanned_indices,ui_geometry_flush_ms,ui_vertex_pack_ms");
  std::fprintf(s.file,",combined_fixed_vertices,combined_moving_vertices,partial_copied_bytes,partial_skipped_bytes");
  std::fprintf(s.file,"\n"); return true;
}
inline void Begin() {
  State& s=Get(); if (!s.file) return;
  std::memset(s.times,0,sizeof(s.times)); std::memset(s.calls,0,sizeof(s.calls));
  s.framebufferCreates=0;s.drawMerges=0;s.gpuVBUpdates=s.gpuIBUpdates=s.gpuUploadBytes=s.gpuRenames=0; s.presents=0; s.packedVertices=s.packedBytes=0; s.gpu=s.render=s.waitRender=s.waitSubmit=-1;
  s.vbRenames=s.ibRenames=s.vbBlocks=s.ibBlocks=s.scannedIndices=0;
  s.insideUI=false;s.uiGeometryFlush=s.uiVertexPack=0;
  s.combinedFixedVertices=s.combinedMovingVertices=0;
  s.partialCopiedBytes=s.partialSkippedBytes=0;
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
  std::fprintf(s.file,",%u,%u,%u,%u,%u,%.6f,%.6f",s.vbRenames,s.ibRenames,s.vbBlocks,s.ibBlocks,s.scannedIndices,s.uiGeometryFlush,s.uiVertexPack);
  std::fprintf(s.file,",%u,%u,%u,%u",s.combinedFixedVertices,s.combinedMovingVertices,s.partialCopiedBytes,s.partialSkippedBytes);
  std::fprintf(s.file,"\n");
  if (--s.remaining==0) Stop();
}
struct Scope {
  Stage stage; bool active,previousUI; Clock::time_point start;
  explicit Scope(Stage value):stage(value),active(Get().file!=nullptr),previousUI(Get().insideUI) {
    if(active) {if(stage==UI)Get().insideUI=true;start=Clock::now();}
  }
  ~Scope() { if(active) {
    auto& state=Get();const double ms=Ms(start,Clock::now());
    state.times[stage]+=ms;++state.calls[stage];
    if(state.insideUI && stage==GeometryFlush)state.uiGeometryFlush+=ms;
    if(state.insideUI && stage==VertexPack)state.uiVertexPack+=ms;
    if(stage==UI)state.insideUI=previousUI;
  } }
  Scope(const Scope&)=delete; Scope& operator=(const Scope&)=delete;
};
}
