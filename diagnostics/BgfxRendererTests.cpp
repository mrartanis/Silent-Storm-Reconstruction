#include "../Main/StdAfx.h"
#undef for
#include "../Game/Platform.h"
#include "../Main/Gfx.h"
#include "../Main/GfxBuffers.h"
#include "../Main/GfxRender.h"
#include "../Main/GfxShaders.h"
#include "../Misc/2DArray.h"
#include <cstdio>

namespace {
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
int Run() {
  NGfx::SRenderTargetsInfo targets;targets.nRegisters=1;targets.AddTex(16,1);
  if(!NGfx::SetMode(NGfx::SVideoMode(64,64,32,NGfx::WINDOWED),targets))return 3;
  NGfx::CRenderContext context;context.SetCulling(NGfx::CULL_NONE);context.SetDepth(NGfx::DEPTH_NORMAL);
  context.SetVertexShader(vsConstLight);context.SetPixelShader(psDiffuse);
  context.ClearBuffers(0xff000000);
  auto red=Quad(-1,1,1,-1,0.5f);context.SetVSConst(16,CVec4(1,0,0,1));Draw(context,red);
  // A farther primitive must fail depth; a nearer half must replace red.
  auto farther=Quad(-1,1,1,-1,0.8f);context.SetVSConst(16,CVec4(0,1,0,1));Draw(context,farther);
  auto nearer=Quad(0,1,1,-1,0.2f);context.SetVSConst(16,CVec4(0,0,1,1));Draw(context,nearer);
  CArray2D<NGfx::SPixel8888> image;NGfx::MakeScreenShot(&image,false);
  if(image.GetXSize()!=64 || image.GetYSize()!=64 || !Color(image,16,32,255,0,0) || !Color(image,48,32,0,0,255))return 4;
  // A clear after submitted geometry must stay after it, and texture targets
  // must be usable by a later screen pass in the same frame.
  auto target=NGfx::MakeTexture(16,16,1,NGfx::SPixel8888::ID,NGfx::TARGET,NGfx::CLAMP);
  CObj<NGfx::CTexture> targetOwner=target;
  context.SetTextureRT(target);context.ClearBuffers(0xff00ff00);
  context.SetScreenRT();context.ClearBuffers(0xff000000);
  context.SetVertexShader(vsTexture);context.SetPixelShader(psTextureCopyAlpha);context.SetTexture(0,target);
  Draw(context,red);NGfx::MakeScreenShot(&image,false);
  if(!Color(image,32,32,0,255,0))return 5;
  // Regular texture subregions are copied from the CPU staging surface.
  CObj<NGfx::CTexture> texture=NGfx::MakeTexture(4,4,1,NGfx::SPixel8888::ID,NGfx::REGULAR,NGfx::CLAMP);
  { NGfx::CTextureLock<NGfx::SPixel8888> lock(texture,0,NGfx::INPLACE);
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)lock[y][x]=NGfx::SPixel8888(255,255,0,255); }
  context.ClearBuffers(0xff000000);context.SetTexture(0,texture);Draw(context,red);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,32,32,255,255,0))return 6;
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
  if(!Color(image,16,32,0,0,255) || !Color(image,48,32,0,0,0))return 11;
  context.SetStencil(NGfx::STENCIL_NONE);context.ClearBuffers(0xffff0000);
  context.SetAlphaCombine(NGfx::COMBINE_ALPHA);context.SetVSConst(16,CVec4(0,1,0,0.5f));Draw(context,red);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,32,32,128,128,0))return 12;
  context.SetAlphaCombine(NGfx::COMBINE_NONE);context.ClearBuffers(0xff000000);
  NGfx::pDevice->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
  NGfx::pDevice->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);
  NGfx::pDevice->SetRenderState(D3DRS_ALPHAREF,128);
  context.SetVSConst(16,CVec4(1,0,0,0.25f));Draw(context,red);
  context.SetVSConst(16,CVec4(0,1,0,0.75f));Draw(context,nearer);
  NGfx::MakeScreenShot(&image,false);
  if(!Color(image,16,32,0,0,0) || !Color(image,48,32,0,255,0))return 13;
  NGfx::pDevice->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
  // The same pixel shader must select its cube-sampler variant at runtime.
  CObj<NGfx::CCubeTexture> cube=NGfx::MakeCubeTexture(4,1,NGfx::SPixel8888::ID,NGfx::REGULAR);
  for(int face=0;face<6;++face) {
    NGfx::CTextureLock<NGfx::SPixel8888> lock(cube,NGfx::EFace(face),0,NGfx::INPLACE);
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)lock[y][x]=NGfx::SPixel8888(0,255,255,255);
  }
  context.ClearBuffers(0xff000000);context.SetVertexShader(vsRenderCubeMap);
  context.SetPixelShader(psTextureCopyAlpha);context.SetTexture(0,cube);Draw(context,red);
  NGfx::MakeScreenShot(&image,false);if(!Color(image,32,32,0,255,255))return 14;
  // Existing UI transforms subtract half a pixel. Linear filtering must still
  // map a 64x64 texture to 64x64 pixels without mixing adjacent texels.
  CObj<NGfx::CTexture> checker=NGfx::MakeTexture(64,64,1,NGfx::SPixel8888::ID,NGfx::REGULAR,NGfx::CLAMP);
  {NGfx::CTextureLock<NGfx::SPixel8888> lock(checker,0,NGfx::INPLACE);
    for(int y=0;y<64;++y)for(int x=0;x<64;++x)lock[y][x]=(x+y)&1 ? NGfx::SPixel8888(0,0,255,255) : NGfx::SPixel8888(255,0,0,255);}
  auto pixelsQuad=Quad(-1-1.0f/64,1+1.0f/64,1-1.0f/64,-1+1.0f/64,0.5f);
  context.ClearBuffers(0xff000000);context.SetVertexShader(vsTexture);context.SetTexture(0,checker);Draw(context,pixelsQuad);
  NGfx::MakeScreenShot(&image,false);
  for(int y=16;y<20;++y)for(int x=16;x<20;++x)
    if(!Color(image,x,y,(x+y)&1?0:255,0,(x+y)&1?255:0))return 15;
  NGfx::Flip();
  NGfx::BgfxTexture* invalid=nullptr;
  if(NGfx::pDevice->CreateTexture(0,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&invalid,nullptr)!=E_INVALIDARG || invalid)return 7;
  if(!NGfx::SetMode(NGfx::SVideoMode(80,48,32,NGfx::WINDOWED),targets))return 8;
  context.SetScreenRT();context.ClearBuffers(0xffff00ff);NGfx::MakeScreenShot(&image,false);
  if(image.GetXSize()!=80 || image.GetYSize()!=48 || !Color(image,40,24,255,0,255))return 9;
  return NGfx::pDevice->Healthy()?0:10;
}
}
int main() {
  S2Platform::SetErrorDialogs(false);
  if(!S2Platform::Init("Silent Storm bgfx regression",64,64,true))return 1;
  if(!NGfx::Init3D(static_cast<HWND>(S2Platform::NativeWindow()))) {S2Platform::Done();return 2;}
  int result=Run();NGfx::Done3D();S2Platform::Done();
  if(!result)fprintf(stdout,"bgfx shaders, depth, pass order, render targets, texture upload, stencil masks, blending, alpha test, cube sampling, readback, resize and shutdown passed\n");
  return result;
}

