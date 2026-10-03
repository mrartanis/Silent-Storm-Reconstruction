#include "../Main/StdAfx.h"
#undef for
#include "../Game/Platform.h"
#include "../Main/Gfx.h"
#include "../Main/GfxBuffers.h"
#include "../Main/GfxRender.h"
#include "../Main/GfxShaders.h"
#include "../Misc/2DArray.h"
#include <cstdio>
#include "../Main/VectorFonts.h"
#include "../Main/GfxUtils.h"
#include "../Main/SWTexture.h"
#include "../Main/Interface.h"
#include "../Main/UICommCtrls.h"
#include "../Main/GInit.h"
#include "../MiscDll/Commands.h"
#include "../Main/DisplayLayout.h"
#include "../FileIO/Streams.h"
#include <SDL3/SDL.h>
#include "FrameProfiler.h"

namespace {
CObj<NGfx::CGeometry> Quad(float left,float top,float right,float bottom,float z);
void Draw(NGfx::CRenderContext& context,NGfx::CGeometry* geometry);
int RunGraphicsOptions() {
  // Existing profiles must not retain the removed low-quality presets.
  NGlobal::SetVar("gfx_resolution",wstring(L"1024x768"));
  NGlobal::SetVar("gfx_fullscreen",0);
  NGlobal::SetVar("gfx_texture_mip",3);
  NGlobal::SetVar("gfx_texture_usedxt",1);
  NGlobal::SetVar("gfx_terrain_565",1);
  NGlobal::SetVar("gfx_shadows",0);
  NGlobal::SetVar("gfx_vsync",0);
  if(!NGScene::SetModeFromConfig(false))return 50;
  for(const char* name:{"gfx_texture_mip","gfx_texture_usedxt","gfx_terrain_565","gfx_16bit_textures","gfx_fastest"})
    if(NGlobal::GetVar(name,-1).GetInt()!=0)return 51;
  if(NGlobal::GetVar("gfx_shadows",0).GetInt()!=1 ||
     NGlobal::GetVar("gfx_depth_tex_resolution",0).GetInt()!=2048 ||
     NGlobal::GetVar("gfx_cl_cube_resolution",0).GetInt()!=256)return 52;
  NGfx::SRenderTargetsInfo targets;
  if(!NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 53;
  NGlobal::SetVar("gfx_anisotropic_filter",1);
  NGfx::Flip();
  if(NGfx::pDevice->PresentationResetFlags()&(BGFX_RESET_VSYNC|BGFX_RESET_MAXANISOTROPY))return 54;
  NGlobal::SetVar("gfx_vsync",1);NGlobal::SetVar("gfx_anisotropic_filter",16);
  NGfx::Flip();
  if((NGfx::pDevice->PresentationResetFlags()&(BGFX_RESET_VSYNC|BGFX_RESET_MAXANISOTROPY))!=
     (BGFX_RESET_VSYNC|BGFX_RESET_MAXANISOTROPY))return 55;
  NGlobal::SetVar("gfx_vsync",0);NGlobal::SetVar("gfx_anisotropic_filter",1);
  NGfx::Flip();
  if(NGfx::pDevice->PresentationResetFlags()&(BGFX_RESET_VSYNC|BGFX_RESET_MAXANISOTROPY))return 56;
  if(!NGfx::pDevice->Healthy())return 57;
  // GPU image test: filtering a stair-step edge must create intermediate
  // coverage while preserving both uniform regions and opaque alpha.
  CObj<NGfx::CTexture> pattern=NGfx::MakeTexture(128,128,1,NGfx::SPixel8888::ID,NGfx::REGULAR,NGfx::CLAMP);
  {NGfx::CTextureLock<NGfx::SPixel8888> lock(pattern,0,NGfx::INPLACE);
    for(int y=0;y<128;++y)for(int x=0;x<128;++x)
      lock[y][x]=x>y/2+32?NGfx::SPixel8888(255,255,255,255):NGfx::SPixel8888(0,0,0,255);}
  auto geometry=Quad(-1-1.0f/128,1+1.0f/128,1-1.0f/128,-1+1.0f/128,0.5f);
  NGfx::CRenderContext context;context.SetCulling(NGfx::CULL_NONE);context.SetDepth(NGfx::DEPTH_NONE);
  context.SetVertexShader(vsTexture);context.SetPixelShader(psTextureCopyAlpha);context.SetTexture(0,pattern);
  context.ClearBuffers(0xff000000);Draw(context,geometry);
  CArray2D<NGfx::SPixel8888> original,filtered;
  NGlobal::SetVar("gfx_antialiasing",0);NGfx::MakeScreenShot(&original,false);
  NGlobal::SetVar("gfx_antialiasing",1);NGfx::ApplySceneAntialiasing();NGfx::MakeScreenShot(&filtered,false);
  int smooth=0;
  for(int y=8;y<120;++y)for(int x=8;x<120;++x) {
    if(original[y][x].r!=0 && original[y][x].r!=255)return 58;
    if(filtered[y][x].r>0 && filtered[y][x].r<255)++smooth;
    if(filtered[y][x].a!=255)return 59;
  }
  if(smooth<50 || filtered[64][16].r!=0 || filtered[64][112].r!=255)return 60;
  // Fine UI strokes are composed after FXAA. Require exact texels, including
  // after presentation and with AA toggled: neither path may filter the UI.
  CObj<NGfx::CTexture> ui=NGfx::MakeTexture(32,32,1,NGfx::SPixel8888::ID,NGfx::REGULAR,NGfx::CLAMP);
  {NGfx::CTextureLock<NGfx::SPixel8888> lock(ui,0,NGfx::INPLACE);
    for(int y=0;y<32;++y)for(int x=0;x<32;++x)
      lock[y][x]=(x+y)%2?NGfx::SPixel8888(255,255,255,255):NGfx::SPixel8888(0,0,0,255);}
  context.ClearBuffers(0xff000000);Draw(context,geometry);
  NGfx::ApplySceneAntialiasing();
  context.SetTexture(0,ui,true);
  auto uiGeometry=Quad(0.5f-1.0f/128,1+1.0f/128,1-1.0f/128,0.5f+1.0f/128,0.5f);
  Draw(context,uiGeometry);
  CArray2D<NGfx::SPixel8888> composed;
  NGfx::MakeScreenShot(&composed,false);
  for(int y=0;y<128;++y)for(int x=0;x<128;++x) {
    if(x>=96 && y<32) {
      const int value=((x-96+y)%2)?255:0;
      if(composed[y][x].r!=value || composed[y][x].g!=value || composed[y][x].b!=value || composed[y][x].a!=255)return 62;
    } else if(composed[y][x].color!=filtered[y][x].color)return 63;
  }
  NGfx::Flip();NGfx::MakeScreenShot(&filtered,false);
  for(int y=0;y<128;++y)for(int x=0;x<128;++x)
    if(filtered[y][x].color!=composed[y][x].color)return 64;
  NGlobal::SetVar("gfx_antialiasing",0);NGfx::MakeScreenShot(&filtered,false);
  for(int y=0;y<128;++y)for(int x=0;x<128;++x)
    if(filtered[y][x].color!=composed[y][x].color)return 65;
  // Legacy transient geometry expires when Flip advances its buffer frame.
  geometry=Quad(-1-1.0f/128,1+1.0f/128,1-1.0f/128,-1+1.0f/128,0.5f);
  context.SetTexture(0,pattern);context.ClearBuffers(0xff000000);Draw(context,geometry);
  NGfx::ApplySceneAntialiasing();NGfx::MakeScreenShot(&filtered,false);
  for(int y=0;y<128;++y)for(int x=0;x<128;++x)
    if(filtered[y][x].color!=original[y][x].color)return 61;
  std::printf("Graphics: maximum quality restored; live v-sync/anisotropy on/off; scene FXAA edge pixels=%d; UI texels exact before/after presentation; exact AA-off restoration passed\n",smooth);
  return 0;
}
CObj<NGfx::CGeometry> Quad(float left,float top,float right,float bottom,float z) {
  CObj<NGfx::CGeometry> result;
  NGfx::CBufferLock<NGfx::SGeomVecFull> lock(&result,4);
  const CVec3 positions[]={CVec3(left,top,z),CVec3(right,top,z),CVec3(right,bottom,z),CVec3(left,bottom,z)};
  for(int i=0;i<4;++i) {
    lock[i]={};lock[i].pos=positions[i];
    NGfx::CalcTexCoords(&lock[i].tex,i==1 || i==2 ? 1.0f:0.0f,i>=2?1.0f:0.0f);
  }
  return result;
}
void Draw(NGfx::CRenderContext& context,NGfx::CGeometry* geometry) {
  STriangle triangles[]={STriangle(0,1,2),STriangle(0,2,3)};
  context.DrawPrimitive(geometry,NGfx::STriangleList(triangles,2,0));
}
bool Color(const CArray2D<NGfx::SPixel8888>& image,int x,int y,int r,int g,int b) {
  const auto& pixel=image[y][x];
  if(abs(int(pixel.r)-r)<=3 && abs(int(pixel.g)-g)<=3 && abs(int(pixel.b)-b)<=3)return true;
  fprintf(stderr,"Pixel %d,%d expected %d,%d,%d got %d,%d,%d\n",x,y,r,g,b,pixel.r,pixel.g,pixel.b);return false;
}
int RunShadowPcf() {
  NGfx::SRenderTargetsInfo targets;
  if(!NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 140;
  NGlobal::SetVar("gfx_antialiasing",0);
  CObj<NGfx::CTexture> map=NGfx::MakeTexture(32,32,1,NGfx::SPixel8888::ID,NGfx::REGULAR,NGfx::CLAMP);
  {NGfx::CTextureLock<NGfx::SPixel8888> lock(map,0,NGfx::INPLACE);
    for(int y=0;y<32;++y)for(int x=0;x<32;++x)
      lock[y][x]=x>y/2+8?NGfx::SPixel8888(255,0,0,255):NGfx::SPixel8888(0,0,0,255);}
  auto geometry=Quad(-1-1.0f/128,1+1.0f/128,1-1.0f/128,-1+1.0f/128,.5f);
  NGfx::CRenderContext context;context.SetCulling(NGfx::CULL_NONE);context.SetDepth(NGfx::DEPTH_NONE);
  context.SetVertexShader(vsDirectionalTestSmoothed);context.SetPixelShader(psShadowTestSmoothed);
  context.SetVSConst(4,CVec4(0,.5f,0,0));context.SetVSConst(16,CVec4(0,0,0,.5f));
  context.SetVSConst(19,CVec4(0,1,1,0));
  context.SetVSConst(25,CVec4(.5f,0,0,.5f));context.SetVSConst(26,CVec4(0,-.5f,0,.5f));
  context.SetVSConst(28,CVec4(0,0,0,1));
  context.SetPSConst(0,CVec4(1,0,0,0));context.SetPSConst(1,CVec4(.2f,.2f,.2f,124/256.f));
  context.SetTexture(0,map,true);
  CArray2D<NGfx::SPixel8888> hard,soft,restored;
  context.SetPSConst(2,CVec4(0,0,0,0));context.ClearBuffers(0);Draw(context,geometry);NGfx::MakeScreenShot(&hard,false);
  context.SetPSConst(2,CVec4(0,0,1,0));context.ClearBuffers(0);Draw(context,geometry);NGfx::MakeScreenShot(&soft,false);
  int fractional=0;
  for(int y=8;y<120;++y)for(int x=8;x<120;++x) {
    if(hard[y][x].r!=51 && hard[y][x].r!=255)return 141;
    if(soft[y][x].r>51 && soft[y][x].r<255)++fractional;
    if(soft[y][x].a!=hard[y][x].a)return 142;
  }
  if(fractional<100 || !Color(soft,8,64,255,255,255) || !Color(soft,120,64,51,51,51))return 143;
  context.SetPSConst(2,CVec4(0,0,0,0));context.ClearBuffers(0);Draw(context,geometry);NGfx::MakeScreenShot(&restored,false);
  for(int y=0;y<128;++y)for(int x=0;x<128;++x)if(restored[y][x].color!=hard[y][x].color)return 144;
  // A steep receiver plane stored in the map must remain lit. Testing every tap
  // against the center depth would incorrectly self-shadow half of this plane.
  {NGfx::CTextureLock<NGfx::SPixel8888> lock(map,0,NGfx::INPLACE);
    for(int y=0;y<32;++y)for(int x=0;x<32;++x)
      lock[y][x]=NGfx::SPixel8888(int((.3f+.4f*(x+.5f)/32)*255+.5f),0,0,255);}
  context.SetVSConst(16,CVec4(.2f,0,0,.5f));context.SetPSConst(2,CVec4(0,0,1,0));
  context.ClearBuffers(0);Draw(context,geometry);NGfx::MakeScreenShot(&soft,false);
  for(int y=8;y<120;++y)for(int x=8;x<120;++x)if(soft[y][x].r!=255)return 145;
  printf("Shadow PCF: %d fractional edge pixels, lit/shadow interiors and alpha preserved, exact off restoration, sloped receiver remains lit\n",fractional);
  return 0;
}
int RunIndexRing() {
  const bool originalBan=NGfx::bBan32BitIndices;
  struct RestorePath { bool original; ~RestorePath(){NGfx::bBan32BitIndices=original;} } restore{originalBan};
  for(bool wide:{false,true}) {
  NGfx::bBan32BitIndices=!wide;
  NGfx::SRenderTargetsInfo targets;
  if(!NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 120;
  NGfx::CRenderContext context;context.SetCulling(NGfx::CULL_NONE);context.SetDepth(NGfx::DEPTH_NONE);
  context.SetVertexShader(vsConstLight);context.SetPixelShader(psDiffuse);
  auto& perf=S2Perf::Get();perf.file=std::tmpfile();if(!perf.file)return 121;
  struct StopCapture { ~StopCapture(){S2Perf::Stop();} } stop;
  S2Perf::Begin();
  auto quad=[](float left,float right) {
    CObj<NGfx::CGeometry> geometry;
    NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> update(&geometry,4);
    NGfx::SGeomVecFull data[4]{};
    const CVec3 positions[]={CVec3(left,1,.5f),CVec3(right,1,.5f),CVec3(right,-1,.5f),CVec3(left,-1,.5f)};
    for(int i=0;i<4;++i)data[i].pos=positions[i];
    update.Write(0,4,data);return geometry;
  };
  auto left=quad(-1,0),right=quad(0,1);
  context.ClearBuffers(0xff000000);context.SetVSConst(16,CVec4(1,0,0,1));Draw(context,left);
  NGfx::NextFrameBuffes(true); // cache aging inside the same GPU frame
  // Degenerate triangles exercise real legacy submissions without adding
  // hundreds of thousands of fullscreen fragments to this regression.
  std::vector<STriangle> filler(65536,STriangle(0,0,0));
  context.DrawPrimitive(left,NGfx::STriangleList(filler.data(),65536,0));
  if(perf.ibRenames!=1)return 122; // 768 KB fits without replacing a used bank
  for(int pass=0;pass<20;++pass) {
    for(unsigned i=0;i<filler.size();++i) {
      const unsigned index=(i/5000+pass)%4;
      filler[i]=STriangle(index,index,index);
    }
    context.DrawPrimitive(right,NGfx::STriangleList(filler.data(),int(filler.size()),0));
  }
  if(perf.ibRenames<2)return 123; // force wrap and preserve the early red draw
  context.SetVSConst(16,CVec4(0,1,0,1));Draw(context,right);
  CArray2D<NGfx::SPixel8888> image;NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,255,0,0) || !Color(image,96,64,0,255,0))return 124;
  NGfx::Flip();S2Perf::Begin();
  left=quad(-1,0);right=quad(0,1);
  context.ClearBuffers(0xff000000);context.SetVSConst(16,CVec4(0,0,1,1));Draw(context,left);
  context.SetVSConst(16,CVec4(0,1,0,1));Draw(context,right);
  NGfx::MakeScreenShot(&image,false);
  if(perf.ibRenames || !Color(image,32,64,0,0,255) || !Color(image,96,64,0,255,0))return 125;
  std::printf("%d-bit index ring: cache aging does not rewind, 65536 triangles without rename, forced multi-MB wrap preserves early draw, next frame reuses the GPU bank; pixels passed\n",wide?32:16);
  if(!NGfx::pDevice->Healthy())return 126;
  }
  return 0;
}
int RunPartialGeometry() {
  NGfx::SRenderTargetsInfo targets;
  if(!NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 100;
  CObj<NGfx::CGeometry> geometry;
  NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> update(&geometry,256);
  std::vector<NGfx::SGeomVecFull> data(256);
  auto quad=[&](int first,float left,float right) {
    const CVec3 positions[]={CVec3(left,1,.5f),CVec3(right,1,.5f),CVec3(right,-1,.5f),CVec3(left,-1,.5f)};
    for(int i=0;i<4;++i)data[first+i].pos=positions[i];
  };
  quad(0,-1,0);quad(128,0,1);update.Write(0,256,data.data());
  NGfx::CRenderContext context;context.SetCulling(NGfx::CULL_NONE);context.SetDepth(NGfx::DEPTH_NONE);
  context.SetVertexShader(vsConstLight);context.SetPixelShader(psDiffuse);
  auto draw=[&](int first,const CVec4& color) {
    STriangle triangles[]={STriangle(first,first+1,first+2),STriangle(first,first+2,first+3)};
    context.SetVSConst(16,color);context.DrawPrimitive(geometry,NGfx::STriangleList(triangles,2,0));
  };
  CArray2D<NGfx::SPixel8888> image;
  context.ClearBuffers(0xff000000);draw(0,CVec4(1,0,0,1));draw(128,CVec4(0,1,0,1));
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,255,0,0) || !Color(image,96,64,0,255,0))return 101;
  auto& perf=S2Perf::Get();perf.file=std::tmpfile();if(!perf.file)return 102;
  struct StopCapture { ~StopCapture(){S2Perf::Stop();} } stop;
  S2Perf::Begin();
  // A retained mesh stays alive through pool rotation. Unchanged parts must
  // neither pack vertices nor send a vertex update to the GPU.
  NGfx::Flip();auto* identity=geometry.GetPtr();
  {NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> next(&geometry,132);
   if(identity!=geometry.GetPtr())return 103;
   next.Write(0,4,data.data());next.Write(128,4,data.data()+128);}
  context.ClearBuffers(0xff000000);draw(0,CVec4(1,0,0,1));draw(128,CVec4(0,1,0,1));
  NGfx::MakeScreenShot(&image,false);
  if(perf.gpuVBUpdates || perf.packedVertices || perf.partialCopiedBytes ||
     perf.partialSkippedBytes!=8*sizeof(data[0]))return 104;
  S2Perf::Begin();quad(128,.25f,.75f);update.Write(128,4,data.data()+128);
  context.ClearBuffers(0xff000000);draw(0,CVec4(1,0,0,1));draw(128,CVec4(0,1,0,1));
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,255,0,0) || !Color(image,96,64,0,255,0) || !Color(image,120,64,0,0,0) ||
     perf.gpuVBUpdates!=1 || !perf.packedVertices || perf.packedVertices>256 || perf.partialCopiedBytes!=4*sizeof(data[0]))return 105;
  // An overwrite after an earlier draw must retain that draw's snapshot.
  context.ClearBuffers(0xff000000);draw(0,CVec4(1,0,0,1));
  quad(0,0,1);update.Write(0,4,data.data());draw(0,CVec4(0,0,1,1));
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,255,0,0) || !Color(image,96,64,0,0,255))return 106;
  // Capacity growth and format changes replace the object; shrinking retains it.
  CObj<NGfx::CGeometry> grown=geometry;
  {NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> next(&grown,513);
   if(grown.GetPtr()==geometry.GetPtr())return 107;}
  {NGfx::CPartialGeometryUpdate<NGfx::SGeomVecNT1> next(&grown,4);
   if(grown->GetGeometryFormatID()!=NGfx::SGeomVecNT1::ID)return 108;}
  if(!NGfx::SetMode(NGfx::SVideoMode(256,128,32,NGfx::WINDOWED),targets))return 109;
  context.SetScreenRT();quad(0,-1,0);
  {NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> next(&geometry,132);next.Write(0,132,data.data());}
  context.ClearBuffers(0xff000000);draw(0,CVec4(1,0,0,1));draw(128,CVec4(0,1,0,1));
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,64,64,255,0,0) || !Color(image,192,64,0,255,0))return 110;
  NGfx::Done3D();
  if(!NGfx::Init3D(static_cast<HWND>(S2Platform::NativeWindow())) ||
     !NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 111;
  context.SetScreenRT();quad(0,0,1);
  {NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> next(&geometry,132);next.Write(0,132,data.data());}
  context.ClearBuffers(0xff000000);draw(0,CVec4(0,0,1,1));
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,0,0,0) || !Color(image,96,64,0,0,255))return 112;
  bool rejected=false;try{NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> next(&geometry,132);next.Write(131,2,data.data());}catch(const std::exception&){rejected=true;}
  if(!rejected)return 113;
  // Spill into another bank instead of evicting a mesh already used this frame.
  CObj<NGfx::CGeometry> largeA,largeB;
  {NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> next(&largeA,90000);
   quad(0,-1,0);next.Write(0,4,data.data());}
  {NGfx::CPartialGeometryUpdate<NGfx::SGeomVecFull> next(&largeB,90000);
   quad(0,0,1);next.Write(0,4,data.data());}
  if(!IsValid(largeA) || !IsValid(largeB) || largeA->GetVertexStream()==largeB->GetVertexStream())return 115;
  context.ClearBuffers(0xff000000);context.SetVSConst(16,CVec4(1,0,0,1));Draw(context,largeA);
  context.SetVSConst(16,CVec4(0,1,0,1));Draw(context,largeB);
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,255,0,0) || !Color(image,96,64,0,255,0))return 116;
  std::printf("Partial geometry: exact pixels, zero unchanged uploads, changed range packing, pool rotation, same-frame snapshots, shrink/grow/format, resize/device epoch, invalid range, pool spill without live eviction passed\n");
  return NGfx::pDevice->Healthy()?0:114;
}
int RunGeometry() {
  NGfx::SRenderTargetsInfo targets;
  if(!NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 70;
  NGfx::CRenderContext context;context.SetCulling(NGfx::CULL_NONE);context.SetDepth(NGfx::DEPTH_NONE);
  context.SetVertexShader(vsConstLight);context.SetPixelShader(psDiffuse);
  auto initial=Quad(-1,1,1,-1,.5f);Draw(context,initial);NGfx::Flip(); // apply legacy context state
  const D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},D3DDECL_END()};
  NGfx::BgfxDeclaration declaration(elements);
  NGfx::BgfxBuffer vertices(256*sizeof(CVec3)),indices(512*sizeof(uint16_t)),wideIndices(512*sizeof(uint32_t),true);
  struct Unbind {
    ~Unbind() {
      if(NGfx::pDevice) {NGfx::pDevice->SetStreamSource(0,nullptr,0,0);NGfx::pDevice->SetVertexDeclaration(nullptr);NGfx::pDevice->SetIndices(nullptr);}S2Perf::Stop();
    }
  } unbind;
  void* data=nullptr;
  auto writeQuad=[&](float left,float right) {
    vertices.Lock(0,4*sizeof(CVec3),&data,0);
    const CVec3 quad[]={CVec3(left,1,0.5f),CVec3(right,1,0.5f),CVec3(right,-1,0.5f),CVec3(left,-1,0.5f)};
    memcpy(data,quad,sizeof(quad));vertices.Unlock();
  };
  writeQuad(-1,0);
  indices.Lock(0,0,&data,0);const uint16_t order[]={0,1,2,0,2,3};memcpy(data,order,sizeof(order));indices.Unlock();
  auto& perf=S2Perf::Get();perf.file=std::tmpfile();if(!perf.file)return 71;S2Perf::Begin();
  auto first=vertices.VertexGPU(declaration,0,sizeof(CVec3),0,4);
  vertices.VertexGPU(declaration,0,sizeof(CVec3),0,4);
  NGfx::Flip();
  vertices.Lock(0,0,&data,D3DLOCK_READONLY);vertices.Unlock();vertices.VertexGPU(declaration,0,sizeof(CVec3),0,4);
  NGfx::Flip();
  if(perf.gpuVBUpdates!=1)return 72; // unchanged and readonly maps do not upload
  vertices.VertexGPU(declaration,0,sizeof(CVec3),0,4);
  vertices.Lock(0,0,&data,0);vertices.MarkWritten(8*sizeof(CVec3),sizeof(CVec3));
  static_cast<CVec3*>(data)[8]=CVec3(0,0,0);vertices.Unlock();
  auto disjoint=vertices.VertexGPU(declaration,0,sizeof(CVec3),8,1);
  if(first.idx!=disjoint.idx || perf.gpuRenames!=1)return 73; // same packed block, separate allocation
  context.ClearBuffers(0xff000000);
  NGfx::pDevice->SetStreamSource(0,&vertices,0,sizeof(CVec3));NGfx::pDevice->SetVertexDeclaration(&declaration);NGfx::pDevice->SetIndices(&indices);
  context.SetVSConst(16,CVec4(1,0,0,1));NGfx::pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
  writeQuad(0,1);context.SetVSConst(16,CVec4(0,1,0,1));NGfx::pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
  if(perf.gpuRenames!=3)return 74; // one initial VB, one initial IB, one renamed VB
  CArray2D<NGfx::SPixel8888> image;NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,255,0,0) || !Color(image,96,64,0,255,0))return 75;
  // Index overwrite also preserves an earlier draw. Negative base uses the
  // normalized fallback; positive base and 32-bit indices bind the GPU bank.
  vertices.Lock(0,0,&data,0);
  const CVec3 quads[]={CVec3(-1,1,.5f),CVec3(0,1,.5f),CVec3(0,-1,.5f),CVec3(-1,-1,.5f),
                      CVec3(0,1,.5f),CVec3(1,1,.5f),CVec3(1,-1,.5f),CVec3(0,-1,.5f)};
  memcpy(data,quads,sizeof(quads));vertices.Unlock();
  context.ClearBuffers(0xff000000);context.SetVSConst(16,CVec4(1,0,0,1));
  NGfx::pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
  indices.Lock(0,sizeof(order),&data,0);for(unsigned i=0;i<6;++i)static_cast<uint16_t*>(data)[i]=order[i]+4;indices.Unlock();
  context.SetVSConst(16,CVec4(0,1,0,1));NGfx::pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,8,0,2);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,32,64,255,0,0) || !Color(image,96,64,0,255,0))return 76;
  context.ClearBuffers(0xff000000);context.SetVSConst(16,CVec4(0,0,1,1));
  NGfx::pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,-4,0,4,0,2);
  wideIndices.Lock(0,0,&data,0);for(unsigned i=0;i<6;++i)static_cast<uint32_t*>(data)[i]=order[i];wideIndices.Unlock();
  NGfx::pDevice->SetIndices(&wideIndices);NGfx::pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,4,0,4,0,2);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,32,64,0,0,255) || !Color(image,96,64,0,0,255))return 77;
  // Adjacent indexed draws merge only across identical state. Repeated writes
  // of the same constants must not introduce a false barrier.
  wideIndices.Lock(0,0,&data,0);for(unsigned i=0;i<6;++i){static_cast<uint32_t*>(data)[i]=order[i];static_cast<uint32_t*>(data)[6+i]=order[i]+4;}wideIndices.Unlock();
  context.ClearBuffers(0xff000000);unsigned merges=perf.drawMerges;
  NGfx::pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
  context.SetVSConst(16,CVec4(0,0,1,1));
  NGfx::pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,8,6,2);
  if(perf.drawMerges!=merges+1)return 79;
  NGfx::MakeScreenShot(&image,false);if(!Color(image,32,64,0,0,255) || !Color(image,96,64,0,0,255))return 80;
  vertices.Lock(0,0,&data,0);
  for(unsigned i=0;i<6;++i){static_cast<CVec3*>(data)[i]=quads[order[i]];static_cast<CVec3*>(data)[6+i]=quads[order[i]+4];}vertices.Unlock();
  context.ClearBuffers(0xff000000);context.SetVSConst(16,CVec4(1,0,0,1));merges=perf.drawMerges;
  NGfx::pDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,1);NGfx::pDevice->DrawPrimitive(D3DPT_TRIANGLELIST,3,1);
  context.SetVSConst(16,CVec4(0,1,0,1)); // flush before changing the color
  NGfx::pDevice->DrawPrimitive(D3DPT_TRIANGLELIST,6,1);NGfx::pDevice->DrawPrimitive(D3DPT_TRIANGLELIST,9,1);
  if(perf.drawMerges!=merges+2)return 81;
  NGfx::MakeScreenShot(&image,false);if(!Color(image,32,64,255,0,0) || !Color(image,96,64,0,255,0))return 82;
  context.ClearBuffers(0xff000000);context.SetVSConst(16,CVec4(1,1,0,1));
  NGfx::pDevice->SetStreamSource(0,&vertices,6*sizeof(CVec3),sizeof(CVec3));
  NGfx::pDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,2);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,32,64,0,0,0) || !Color(image,96,64,255,255,0))return 85;
  // Stack resources must be unbound before their final owner goes away.
  NGfx::pDevice->SetStreamSource(0,nullptr,0,0);NGfx::pDevice->SetVertexDeclaration(nullptr);NGfx::pDevice->SetIndices(nullptr);
  // Existing CPU buffers can outlive a renderer instance. Rebuild their GPU
  // banks against the new epoch, never destroy a recycled handle from the old one.
  NGfx::Done3D();
  if(!NGfx::Init3D(static_cast<HWND>(S2Platform::NativeWindow())) ||
     !NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 83;
  S2Perf::Begin();vertices.VertexGPU(declaration,0,sizeof(CVec3),0,12);wideIndices.IndexGPU(0,12);
  NGfx::Flip();if(perf.gpuVBUpdates!=1 || perf.gpuIBUpdates!=1)return 84;
  S2Perf::Stop();
  std::printf("Geometry: unchanged/readonly reuse, disjoint pooled writes, same-frame VB/IB overwrite, 16/32-bit indices, signed base vertex, indexed/nonindexed batching, constant barriers, stream offset and device epoch passed\n");
  return NGfx::pDevice->Healthy()?0:78;
}
int RunPassStress() {
  NGfx::SRenderTargetsInfo targets;targets.nRegisters=1;
  if(!NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 90;
  CObj<NGfx::CTexture> a=NGfx::MakeTexture(16,16,1,NGfx::SPixel8888::ID,NGfx::TARGET,NGfx::CLAMP);
  CObj<NGfx::CTexture> b=NGfx::MakeTexture(16,16,1,NGfx::SPixel8888::ID,NGfx::TARGET,NGfx::CLAMP);
  NGfx::CRenderContext context;
  auto& perf=S2Perf::Get();perf.file=std::tmpfile();if(!perf.file)return 91;
  struct StopCapture {~StopCapture(){S2Perf::Stop();}} stop;
  NGlobal::SetVar("gfx_antialiasing",1);
  for(unsigned frame=0;frame<8;++frame) {
    S2Perf::Begin();
    for(unsigned pass=0;pass<700;++pass) {
      context.SetTextureRT(pass&1?a.GetPtr():b.GetPtr());context.ClearBuffers(0xffff0000);
      context.SetScreenRT();context.ClearBuffers(0xff0000ff);
      if(!NGfx::pDevice->Healthy())return 92;
    }
    NGfx::ApplySceneAntialiasing();NGfx::Flip();
    if(!NGfx::pDevice->Healthy() || perf.framebufferCreates>(frame==0?5u:0u))return 93;
  }
  CArray2D<NGfx::SPixel8888> image;NGfx::MakeScreenShot(&image,false);
  if(!Color(image,64,64,0,0,255))return 94;
  // A failed renderer still allows the save path to build its thumbnail.
  DWORD unknownShader[]={0xffffffff,0};NGfx::BgfxShader* shader=nullptr;
  if(NGfx::pDevice->CreateVertexShader(unknownShader,&shader)!=E_FAIL || shader)return 95;
  NGfx::MakeScreenShot(&image,false);
  if(image.GetXSize()!=128 || image.GetYSize()!=128 || !Color(image,64,64,0,0,0))return 96;
  CDGPtr<NGScene::CBilinearTexture> thumbnail=new NGScene::CBilinearTexture(image,320,200);
  thumbnail.Refresh();auto* data=thumbnail->GetValue();
  if(!data || !Color(data->mips.front(),319,199,0,0,0))return 97;
  std::printf("Pass stress: 1402 ordered views x 8 frames use five persistent framebuffer attachment sets; final pixels exact; failed-renderer save thumbnail passed\n");
  return 0;
}
int RunDisplay() {
  NGfx::SRenderTargetsInfo targets; targets.nRegisters=1;
  const int sizes[][2]={{1024,768},{1280,720},{1920,1080},{3440,1440},{3840,2160}};
  for(const auto& size:sizes) {
    if(!NGfx::SetMode(NGfx::SVideoMode(size[0],size[1],32,NGfx::WINDOWED),targets)) return 20;
    const auto& m=S2Platform::Display();
    int pw=0,ph=0; SDL_GetWindowSizeInPixels(S2Platform::Window(),&pw,&ph);
    if(pw!=m.pixelWidth || ph!=m.pixelHeight || m.CanvasWidth()<1023.99f || m.CanvasHeight()<767.99f) return 21;
    CObj<NUI::CWindow> root=new NUI::CWindow(NUI::SWindowInfo(0,NUI::SPoint(0,0),NUI::SPoint(1024,768),"desktop"));
    CObj<NUI::CWindow> edge=new NUI::CWindow(NUI::SWindowInfo(root,NUI::SPoint(900,700),NUI::SPoint(100,50)));
    edge->EnableAdaptiveLayout();
    const auto first=edge->GetPosition(); const auto second=edge->GetPosition();
    if(first.x!=900+root->GetSize().x-1024 || first.y!=700+root->GetSize().y-768 || first!=second ||
       !edge->HitTest(first.x+50,first.y+25) || edge->HitTest(first.x-1,first.y+25)) return 22;
    edge->SetPosition(first); edge->SetPosition(edge->GetPosition());
    if(edge->GetPosition()!=first) return 31;
    CObj<NUI::CWindow> panel=new NUI::CWindow(NUI::SWindowInfo(root,NUI::SPoint(512,32),NUI::SPoint(512,564)));
    panel->SetLayoutAnchors(S2Display::Far,S2Display::Stretch);
    if(panel->GetPosition()!=NUI::SPoint(root->GetSize().x-512,32) ||
        panel->GetSize()!=NUI::SPoint(512,root->GetSize().y-204)) return 33;
    CObj<NUI::CWindow> form=new NUI::CWindow(NUI::SWindowInfo(root,NUI::SPoint(0,0),NUI::SPoint(1024,768),"chargenUI"));
    CObj<NUI::CWindow> paper=new NUI::CWindow(NUI::SWindowInfo(form,NUI::SPoint(0,0),NUI::SPoint(1024,768),"background"));
    CObj<NUI::CWindow> field=new NUI::CWindow(NUI::SWindowInfo(form,NUI::SPoint(244,128),NUI::SPoint(200,32),"field"));
    field->EnableAdaptiveLayout();
    if(paper->GetSize()!=NUI::SPoint(1024,768) ||
        field->GetPosition()!=NUI::SPoint(paper->GetPosition().x+244,paper->GetPosition().y+128)) return 34;
    field->SetPosition(field->AuthoredToLayout(NUI::SPoint(260,130)));
    if(field->GetAuthoredPosition()!=NUI::SPoint(260,130)) return 35;
    field->SetPosition(field->AuthoredToLayout(NUI::SPoint(260,130)));
    if(field->GetAuthoredPosition()!=NUI::SPoint(260,130)) return 36;
    // A moved graphics field must open its popup below the new position,
    // aligned to the right edge, retaining its artwork and active hit area.
    CObj<NUI::CComboBox> combo=new NUI::CComboBox(NUI::SWindowInfo(form,NUI::SPoint(672,200),NUI::SPoint(256,18),"combo"));
    combo->EnableAdaptiveLayout();
    combo->SetSize(NUI::SPoint(153,18));
    combo->SetPosition(combo->AuthoredToLayout(NUI::SPoint(775,330)));
    auto popup=form->GetChildByID("list");
    popup->SetSize(NUI::SPoint(256,40));
    popup->SetStyle(NUI::STYLE_VISIBLE,true);
    combo->Update(0,nullptr);
    const auto popupPosition=combo->GetPosition()+NUI::SPoint(153-256,18);
    if(popup->GetPosition()!=popupPosition || popup->GetSize().x!=256 ||
        !popup->GetStyle(NUI::STYLE_ENABLED) || !popup->HitTest(popupPosition.x+75,popupPosition.y+20)) return 44;
    combo->SetStyle(NUI::STYLE_ENABLED,false);combo->Update(0,nullptr);
    if(popup->GetStyle(NUI::STYLE_ENABLED)) return 45;
    combo->SetStyle(NUI::STYLE_ENABLED,true);combo->Update(0,nullptr);
    if(!popup->GetStyle(NUI::STYLE_ENABLED)) return 46;
    CObj<NUI::CWindow> map=new NUI::CWindow(NUI::SWindowInfo(root,NUI::SPoint(0,0),NUI::SPoint(1024,768),"globalmapUI"));
    CObj<NUI::CWindow> mapImage=new NUI::CWindow(NUI::SWindowInfo(map,NUI::SPoint(0,0),NUI::SPoint(1024,768),"background"));
    CObj<NUI::CWindow> zone=new NUI::CWindow(NUI::SWindowInfo(map,NUI::SPoint(217,190),NUI::SPoint(66,90),"northbritain"));
    zone->EnableAdaptiveLayout();
    if(mapImage->GetSize()!=NUI::SPoint(1024,768) ||
       zone->GetPosition()!=NUI::SPoint(mapImage->GetPosition().x+217,mapImage->GetPosition().y+190)) return 37;
    // Chapter gameplay coordinates must meet the same centered markers as input.
    CObj<NUI::CWindow> chapter=new NUI::CWindow(NUI::SWindowInfo(root,NUI::SPoint(0,0),NUI::SPoint(1024,768),"chaptermapUI"));
    CObj<NUI::CWindow> view=new NUI::CWindow(NUI::SWindowInfo(chapter,NUI::SPoint(173,65),NUI::SPoint(678,610),"view"));
    view->EnableAdaptiveLayout();
    NUI::SPoint local;
    view->ScreenToClient(NUI::SPoint(410+S2UI::MapOffsetX(),300+S2UI::MapOffsetY()),&local);
    CObj<NUI::CWindow> mission=new NUI::CWindow(NUI::SWindowInfo(view,local,NUI::SPoint(69,69)));
    CObj<NUI::CWindow> encounter=new NUI::CWindow(NUI::SWindowInfo(view,NUI::SPoint(local.x+100,local.y),NUI::SPoint(49,49)));
    if(!mission->HitTest(420+S2UI::MapOffsetX(),310+S2UI::MapOffsetY()) ||
       !encounter->HitTest(520+S2UI::MapOffsetX(),310+S2UI::MapOffsetY())) return 42;
    CMemoryStream chapterWire;
    {CStructureSaver save(chapterWire,CStructureSaver::WRITE);save.Add(1,&chapter);}
    chapter=0;chapterWire.SetRMode();chapterWire.Seek(0);
    {CStructureSaver load(chapterWire,CStructureSaver::READ);load.Add(1,&chapter);}
    auto restoredView=chapter->GetChildByID("view");
    if(restoredView->GetPosition()!=NUI::SPoint(173+S2UI::MapOffsetX(),65+S2UI::MapOffsetY()) ||
       restoredView->GetSize()!=NUI::SPoint(678,610)) return 43;
    CObj<NUI::CWindow> hud=new NUI::CWindow(NUI::SWindowInfo(root,NUI::SPoint(0,596),NUI::SPoint(1024,172),"unitpanel"));
    hud->EnableAdaptiveLayout();
    CObj<NUI::CWindow> info=new NUI::CWindow(NUI::SWindowInfo(hud,NUI::SPoint(0,36),NUI::SPoint(746,136),"infopanel_singleunit"));
    CObj<NUI::CWindow> tabs=new NUI::CWindow(NUI::SWindowInfo(hud,NUI::SPoint(0,0),NUI::SPoint(746,40),"unitstabbar"));
    CObj<NUI::CWindow> multi=new NUI::CWindow(NUI::SWindowInfo(hud,NUI::SPoint(0,36),NUI::SPoint(746,136),"infopanel_multipleunits"));
    CObj<NUI::CWindow> face=new NUI::CWindow(NUI::SWindowInfo(info,NUI::SPoint(16,16),NUI::SPoint(82,106),"face"));
    CObj<NUI::CWindow> commands=new NUI::CWindow(NUI::SWindowInfo(hud,NUI::SPoint(806,10),NUI::SPoint(214,102),"uniticonbar"));
    for(auto* child:{info.GetPtr(),tabs.GetPtr(),multi.GetPtr(),face.GetPtr(),commands.GetPtr()}) child->EnableAdaptiveLayout();
    if(hud->GetPosition()!=NUI::SPoint(0,root->GetSize().y-172) || hud->GetSize().x!=root->GetSize().x ||
        info->GetPosition()!=NUI::SPoint(0,36) || tabs->GetPosition()!=NUI::SPoint(0,0) ||
        multi->GetPosition()!=NUI::SPoint(0,36) || face->GetPosition()!=NUI::SPoint(16,16) ||
        commands->GetPosition().x!=root->GetSize().x-218) return 38;
    CMemoryStream hudWire;
    {CStructureSaver save(hudWire,CStructureSaver::WRITE);save.Add(1,&hud);}
    hud=0;hudWire.SetRMode();hudWire.Seek(0);
    {CStructureSaver load(hudWire,CStructureSaver::READ);load.Add(1,&hud);}
    if(!hud || hud->GetChildByID("infopanel_singleunit")->GetPosition()!=NUI::SPoint(0,36) ||
        hud->GetChildByID("unitstabbar")->GetPosition()!=NUI::SPoint(0,0) ||
        hud->GetChildByID("uniticonbar")->GetPosition().x!=root->GetSize().x-218) return 39;
    // Side-panel frames, text and doll viewports must move as one authored group.
    // The old independent anchors stretched the background and split the doll in two.
    for(const char* id:{"inventorypanel","characterpanel"}) {
      CObj<NUI::CWindow> panel=new NUI::CWindow(NUI::SWindowInfo(root,NUI::SPoint(512,32),NUI::SPoint(512,564),id));
      panel->SetLayoutAnchors(S2Display::Far,S2Display::Stretch);
      CObj<NUI::CWindow> paper=new NUI::CWindow(NUI::SWindowInfo(panel,NUI::SPoint(0,0),NUI::SPoint(512,512)));
      CObj<NUI::CWindow> footer=new NUI::CWindow(NUI::SWindowInfo(panel,NUI::SPoint(0,512),NUI::SPoint(512,52)));
      CObj<NUI::CWindow> doll=new NUI::CWindow(NUI::SWindowInfo(panel,NUI::SPoint(296,62),NUI::SPoint(193,420),"unitview"));
      for(auto* child:{paper.GetPtr(),footer.GetPtr(),doll.GetPtr()}) child->EnableAdaptiveLayout();
      const int offset=(panel->GetSize().y-564)/2;
      if(paper->GetPosition()!=NUI::SPoint(0,offset) || paper->GetSize()!=NUI::SPoint(512,512) ||
          footer->GetPosition()!=NUI::SPoint(0,512+offset) || footer->GetSize()!=NUI::SPoint(512,52) ||
          doll->GetPosition()!=NUI::SPoint(296,62+offset) || doll->GetSize()!=NUI::SPoint(193,420)) return 40;
      CMemoryStream panelWire;
      {CStructureSaver save(panelWire,CStructureSaver::WRITE);save.Add(1,&panel);}
      panel=0;panelWire.SetRMode();panelWire.Seek(0);
      {CStructureSaver load(panelWire,CStructureSaver::READ);load.Add(1,&panel);}
      panel->SetLayoutAnchors(S2Display::Far,S2Display::Stretch);
      if(panel->GetChildByID("unitview")->GetPosition()!=NUI::SPoint(296,62+offset) ||
          panel->GetChildByID("unitview")->GetSize()!=NUI::SPoint(193,420)) return 41;
    }
    CObj<NUI::CWindow> fill=new NUI::CWindow(NUI::SWindowInfo(root,NUI::SPoint(0,0),NUI::SPoint(1024,768)));
    const auto fillSize=fill->GetSize(); fill->SetSize(fillSize); fill->SetSize(fill->GetSize());
    if(fill->GetSize()!=root->GetSize()) return 32;
    NGfx::CRenderContext rc; rc.ClearBuffers(0xff000000);
    CArray2D<NGfx::SPixel8888> image; NGfx::MakeScreenShot(&image,false);
    if(image.GetXSize()!=pw || image.GetYSize()!=ph) return 23;
    std::printf("surface %dx%d window %dx%d display=%.3f UI=%.3f layout/hit/readback passed\n",pw,ph,m.windowWidth,m.windowHeight,m.displayScale,m.uiScale);
  }
  if(!NGfx::SetMode(NGfx::SVideoMode(1024,768,32,NGfx::WINDOWED),targets)) return 24;
  CObj<NGScene::CFontInfo> font=NGScene::GetVectorFont(NGScene::SFont(24,"Arial"));
  if(!font || !font->IsVector()) return 25;
  CDGPtr<CPtrFuncBase<CFontFormatInfo>> format(font->GetFormatInfo()); format.Refresh();
  const int advance=format->GetValue()->GetChar(0x416).nWidth;
  CMemoryStream wire;
  { CStructureSaver save(wire,CStructureSaver::WRITE); save.Add(1,&font); }
  if(wire.GetSize()>4096) return 26;
  font=0; wire.SetRMode(); wire.Seek(0);
  { CStructureSaver load(wire,CStructureSaver::READ); load.Add(1,&font); }
  if(!font) return 27;
  format=font->GetFormatInfo(); format.Refresh();
  if(!font->IsVector() || format->GetValue()->GetChar(0x416).nWidth!=advance) return 28;
  CDGPtr<CPtrFuncBase<NGfx::CTexture>> texture(font->GetTexture()); texture.Refresh();
  NGfx::CRenderContext rc; rc.ClearBuffers(0xff000000); rc.SetAlphaCombine(NGfx::COMBINE_SMART_ALPHA);
  {
    NGfx::C2DQuadsRenderer quads(rc,NGfx::GetScreenRect(),NGfx::QRM_OVERWRITE);
    float x=16;
    for(const wchar_t c:std::wstring(L"AV English \u0420\u0443\u0441\u0441\u043a\u0438\u0439")) {
      const auto& g=format->GetValue()->GetChar(c);
      quads.AddRect(CTRect<float>(x+g.nA,16,x+g.nA+g.x2-g.x1,16+g.y2-g.y1),texture->GetValue(),CTRect<float>(g.x1,g.y1,g.x2,g.y2));
      x+=g.nA+g.nBC;
    }
    quads.Flush();
  }
  CArray2D<NGfx::SPixel8888> image; NGfx::MakeScreenShot(&image,false);
  int lit=0; for(int y=16;y<60;++y)for(int x=16;x<400;++x)if(image[y][x].r>100)++lit;
  if(lit<100) return 29;
  NGScene::ResetVectorFontDevice(); texture.Refresh();
  if(!texture->GetValue()) return 30;
  std::printf("English/Cyrillic vector atlas upload, descriptor serialization (%d bytes), metrics restoration and device generation passed, pixels=%d\n",wire.GetSize(),lit);
  NGScene::ClearVectorFonts(); return 0;
}
int Run() {
  NGfx::SRenderTargetsInfo targets;targets.nRegisters=1;targets.AddTex(16,1);
  if(!NGfx::SetMode(NGfx::SVideoMode(128,128,32,NGfx::WINDOWED),targets))return 3;
  NGfx::CRenderContext context;context.SetCulling(NGfx::CULL_NONE);context.SetDepth(NGfx::DEPTH_NORMAL);
  context.SetVertexShader(vsConstLight);context.SetPixelShader(psDiffuse);
  context.ClearBuffers(0xff000000);
  auto red=Quad(-1,1,1,-1,0.5f);context.SetVSConst(16,CVec4(1,0,0,1));Draw(context,red);
  // A farther primitive must fail depth; a nearer half must replace red.
  auto farther=Quad(-1,1,1,-1,0.8f);context.SetVSConst(16,CVec4(0,1,0,1));Draw(context,farther);
  auto nearer=Quad(0,1,1,-1,0.2f);context.SetVSConst(16,CVec4(0,0,1,1));Draw(context,nearer);
  CArray2D<NGfx::SPixel8888> image;NGfx::MakeScreenShot(&image,false);
  if(image.GetXSize()!=128 || image.GetYSize()!=128 || !Color(image,32,64,255,0,0) || !Color(image,96,64,0,0,255))return 4;
  // A clear after submitted geometry must stay after it, and texture targets
  // must be usable by a later screen pass in the same frame.
  auto target=NGfx::MakeTexture(16,16,1,NGfx::SPixel8888::ID,NGfx::TARGET,NGfx::CLAMP);
  CObj<NGfx::CTexture> targetOwner=target;
  context.SetTextureRT(target);context.ClearBuffers(0xff00ff00);
  context.SetScreenRT();context.ClearBuffers(0xff000000);
  context.SetVertexShader(vsTexture);context.SetPixelShader(psTextureCopyAlpha);context.SetTexture(0,target);
  Draw(context,red);NGfx::MakeScreenShot(&image,false);
  if(!Color(image,64,64,0,255,0))return 5;
  // Regular texture subregions are copied from the CPU staging surface.
  CObj<NGfx::CTexture> texture=NGfx::MakeTexture(4,4,1,NGfx::SPixel8888::ID,NGfx::REGULAR,NGfx::CLAMP);
  { NGfx::CTextureLock<NGfx::SPixel8888> lock(texture,0,NGfx::INPLACE);
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)lock[y][x]=NGfx::SPixel8888(255,255,0,255); }
  context.ClearBuffers(0xff000000);context.SetTexture(0,texture);Draw(context,red);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,64,64,255,255,0))return 6;
  // Shadow passes share stencil bits; writing bit 0x40 must preserve 0x80.
  context.SetVertexShader(vsConstLight);context.SetPixelShader(psDiffuse);
  context.SetDepth(NGfx::DEPTH_NONE);context.ClearBuffers(0xff000000);
  auto leftHalf=Quad(-1,1,0,-1,0.5f);
  context.SetColorWrite(NGfx::COLORWRITE_NONE);context.SetStencil(NGfx::STENCIL_WRITE,0x80,0x80);
  Draw(context,leftHalf);
  context.SetStencil(NGfx::STENCIL_WRITE,0x40,0x40);Draw(context,red);
  context.SetStencil(NGfx::STENCIL_TEST,0x80,0x80);context.SetColorWrite(NGfx::COLORWRITE_ALL);
  context.SetVSConst(16,CVec4(0,0,1,1));Draw(context,red);
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,0,0,255) || !Color(image,96,64,0,0,0))return 11;
  context.SetStencil(NGfx::STENCIL_NONE);context.ClearBuffers(0xffff0000);
  context.SetAlphaCombine(NGfx::COMBINE_ALPHA);context.SetVSConst(16,CVec4(0,1,0,0.5f));Draw(context,red);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,64,64,128,128,0))return 12;
  context.SetAlphaCombine(NGfx::COMBINE_NONE);context.ClearBuffers(0xff000000);
  NGfx::pDevice->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
  NGfx::pDevice->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);
  NGfx::pDevice->SetRenderState(D3DRS_ALPHAREF,128);
  context.SetVSConst(16,CVec4(1,0,0,0.25f));Draw(context,red);
  context.SetVSConst(16,CVec4(0,1,0,0.75f));Draw(context,nearer);
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,64,0,0,0) || !Color(image,96,64,0,255,0))return 13;
  NGfx::pDevice->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
  // The same pixel shader must select its cube-sampler variant at runtime.
  CObj<NGfx::CCubeTexture> cube=NGfx::MakeCubeTexture(4,1,NGfx::SPixel8888::ID,NGfx::REGULAR);
  for(int face=0;face<6;++face) {
    NGfx::CTextureLock<NGfx::SPixel8888> lock(cube,NGfx::EFace(face),0,NGfx::INPLACE);
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)lock[y][x]=NGfx::SPixel8888(0,255,255,255);
  }
  context.ClearBuffers(0xff000000);context.SetVertexShader(vsRenderCubeMap);
  context.SetPixelShader(psTextureCopyAlpha);context.SetTexture(0,cube);Draw(context,red);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,64,64,0,255,255))return 14;
  // Existing UI transforms subtract half a pixel. Linear filtering must still
  // map a 128x128 texture to 128x128 pixels without mixing adjacent texels.
  CObj<NGfx::CTexture> checker=NGfx::MakeTexture(128,128,1,NGfx::SPixel8888::ID,NGfx::REGULAR,NGfx::CLAMP);
  {NGfx::CTextureLock<NGfx::SPixel8888> lock(checker,0,NGfx::INPLACE);
    for(int y=0;y<128;++y)for(int x=0;x<128;++x)lock[y][x]=(x+y)&1 ? NGfx::SPixel8888(0,0,255,255) : NGfx::SPixel8888(255,0,0,255);}
  auto pixelsQuad=Quad(-1-1.0f/128,1+1.0f/128,1-1.0f/128,-1+1.0f/128,0.5f);
  context.ClearBuffers(0xff000000);context.SetVertexShader(vsTexture);context.SetTexture(0,checker);Draw(context,pixelsQuad);
  NGfx::MakeScreenShot(&image,false);
  for(int y=32;y<36;++y)for(int x=32;x<36;++x)
    if(!Color(image,x,y,(x+y)&1?0:255,0,(x+y)&1?255:0))return 15;
  NGfx::Flip();
  NGfx::BgfxTexture* invalid=nullptr;
  if(NGfx::pDevice->CreateTexture(0,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&invalid,nullptr)!=E_INVALIDARG || invalid)return 7;
  if(!NGfx::SetMode(NGfx::SVideoMode(256,128,32,NGfx::WINDOWED),targets))return 8;
  context.SetScreenRT();context.ClearBuffers(0xffff00ff);NGfx::MakeScreenShot(&image,false);
  if(image.GetXSize()!=256 || image.GetYSize()!=128 || !Color(image,128,64,255,0,255))return 9;
  return NGfx::pDevice->Healthy()?0:10;
}
}
int main(int argc,char** argv) {
  S2Platform::SetErrorDialogs(false);
  if(!S2Platform::Init("Silent Storm bgfx regression",128,128,true))return 1;
  if(!NGfx::Init3D(static_cast<HWND>(S2Platform::NativeWindow()))) {S2Platform::Done();return 2;}
  int result=argc>1 && std::string(argv[1])=="--display" ? RunDisplay() :
             argc>1 && std::string(argv[1])=="--geometry" ? RunGeometry() :
             argc>1 && std::string(argv[1])=="--partial" ? RunPartialGeometry() :
             argc>1 && std::string(argv[1])=="--index-ring" ? RunIndexRing() :
             argc>1 && std::string(argv[1])=="--passes" ? RunPassStress() :
             argc>1 && std::string(argv[1])=="--graphics" ? RunGraphicsOptions() :
             argc>1 && std::string(argv[1])=="--shadow-pcf" ? RunShadowPcf() : Run();
  NGfx::Done3D();S2Platform::Done();
  if(result)fprintf(stderr,"bgfx regression failure code=%d\n",result);
  if(!result)fprintf(stdout,"bgfx shaders, depth, pass order, render targets, texture upload, stencil masks, blending, alpha test, cube sampling, readback, resize and shutdown passed\n");
  return result;
}
