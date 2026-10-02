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
#include "../Main/Interface.h"
#include "../Main/DisplayLayout.h"
#include "../FileIO/Streams.h"
#include <SDL3/SDL.h>

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
  int result=argc>1 && std::string(argv[1])=="--display" ? RunDisplay() : Run();NGfx::Done3D();S2Platform::Done();
  if(result)fprintf(stderr,"bgfx regression failure code=%d\n",result);
  if(!result)fprintf(stdout,"bgfx shaders, depth, pass order, render targets, texture upload, stencil masks, blending, alpha test, cube sampling, readback, resize and shutdown passed\n");
  return result;
}
