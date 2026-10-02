#include "../Game/DisplayGeometry.h"
#include <cstdio>
#include <limits>
static int failures = 0;
static void Check(bool ok, const char* name) { if (!ok) { std::fprintf(stderr,"FAIL: %s\n",name); ++failures; } }
static bool Close(float a, float b) { return std::fabs(a-b) < 0.001f; }
int main() {
  for (const auto size : {std::pair<int,int>{1024,768}, {1280,720}, {1920,1080}, {3440,1440}, {3840,2160}, {7680,4320}})
    for (const float dpi : {1.0f,1.25f,1.5f,2.0f}) for (const float percent : {0.0f,75.0f,100.0f,125.0f,150.0f,200.0f}) {
      S2Display::Metrics m; m.pixelWidth=size.first; m.pixelHeight=size.second;
      m.uiScale=S2Display::UIScale(m.pixelWidth,m.pixelHeight,dpi,percent);
      Check(m.CanvasWidth() >= 1023.999f && m.CanvasHeight() >= 767.999f,"authored UI fits");
      for (float v : {0.0f,10.25f,512.0f,1000.0f}) Check(Close(m.PixelToUI(m.UIToPixel(v)),v),"UI roundtrip");
      auto fit=S2Display::Fit(m.pixelWidth,m.pixelHeight,640,480);
      Check(Close(fit.width/fit.height,4.0f/3),"video aspect");
      Check(fit.x >= -0.001f && fit.y >= -0.001f,"video contained");
      float fov=S2Display::HorizontalFOV(60,m.pixelWidth,m.pixelHeight);
      Check(Close(std::tan(fov*3.14159265f/360)*m.pixelHeight/m.pixelWidth,std::tan(60*3.14159265f/360)*0.75f),"vertical FOV invariant");
    }
  S2Display::Metrics m; m.windowWidth=960; m.windowHeight=540; m.pixelWidth=1920; m.pixelHeight=1080;
  Check(Close(m.WindowToPixelX(100),200)&&Close(m.WindowToPixelY(50),100),"relative and absolute input pixel density");
  auto changed=S2Display::Update(m,960,540,1200,675,1.25f,2,false,100);
  Check(changed.revision==m.revision+1 && changed.displayID==2,"fractional DPI monitor change invalidates layout");
  Check(Close(changed.WindowToPixelX(100),125),"fractional DPI input");
  auto minimized=S2Display::Update(changed,0,0,0,0,1.25f,2,true,100);
  Check(!minimized.drawable && minimized.pixelWidth==1200 && minimized.windowHeight==540,"minimize retains usable geometry and suspends drawing");
  auto restored=S2Display::Update(minimized,960,540,1200,675,1.25f,2,false,100);
  Check(restored.drawable && restored.revision==minimized.revision+1,"restore resumes drawing and invalidates layout");
  auto monitor=S2Display::Update(restored,960,540,1200,675,1.25f,3,false,100);
  Check(monitor.revision==restored.revision+1 && Close(monitor.uiScale,restored.uiScale),"same DPI monitor change refreshes modes without rescaling");
  Check(S2Display::Position(900,1024,1920,S2Display::Far)==1796,"right anchor");
  Check(S2Display::Position(412,1024,1920,S2Display::Center)==860,"center anchor");
  const float preview = S2Display::PreviewFOV(60,1080,256,256);
  Check(Close(std::tan(preview*3.14159265f/360)/std::tan(60*3.14159265f/360),1080.0f/768),"fixed preview pixel density");
  Check(Close(S2Display::PreviewFOV(60,1080,256,360),60),"preview viewport height accounted for");
  for(float scale:{0.75f,1.0f,1.25f,1.5f,2.0f}) for(int bound=1;bound<1024;++bound) {
    const int pixel=S2Display::Boundary(bound,scale);
    Check(int(pixel/scale)>=bound,"physical left edge maps inside logical interval");
    Check(int((pixel-1)/scale)<bound,"physical pixel before left edge stays outside");
  }
  Check(S2Display::PageForSource({{0,0},{0,100},{0,200},{1,0},{1,50}},1,70)==4,"dialogue source survives earlier page count changes");
  Check(S2Display::PageForSource({{0,0},{1,0},{1,100}},1,70)==1,"dialogue remains on same phrase after reflow");
  Check(S2Display::PageForSource({{0,0},{1,0},{1,30},{1,60}},1,70)==3,"dialogue character offset selects continuation page");
  Check(S2Display::PageForSource({{0,0}},1,0)==-1,"missing phrase uses legacy stage fallback");
  for(int width:{1024,1366,1920,3440,7680}) {
    const auto slices=S2Display::ExpandHorizontal(width,1024,746);
    int end=0;
    for(const auto& slice:slices) {
      Check(slice.position==end && slice.width>0,"HUD backdrop has no gaps or overlaps");
      Check(slice.source>=0 && slice.source+slice.sourceWidth<=1024,"HUD samples stay in authored texture");
      end+=slice.width;
    }
    Check(end==width,"HUD backdrop covers full width");
    if(width>1024) {
      Check(slices.front().width==746 && slices.back().width==278,"HUD portrait/command art keeps its width");
      Check(slices.back().position-slices.back().source==width-1024,"HUD command artwork follows right controls");
      Check(slices[1].sourceWidth==1,"HUD filler never repeats frames");
    } else Check(slices.size()==1 && slices[0].sourceWidth==1024,"original HUD artwork unchanged");
  }
  Check(S2Display::ExpandHorizontal(0,1024,746).empty(),"zero-width HUD has no geometry");
  for(int tw:{64,512,1024,4096}) for(int edge:{16,746,1024}) {
    const auto tile=S2Display::NeutralHorizontalTile(tw,edge);
    Check(tile.width>0 && tile.width==tile.sourceWidth,"HUD grain keeps authored width");
    Check(tile.source>=0 && tile.source+tile.sourceWidth<=(std::min)(tw,edge),"neutral HUD tile excludes command frames");
  }
  Check(S2Display::NeutralHorizontalTile(0,746).width==0,"missing neutral texture safe");
  auto loading=S2Display::Fit(3440,1440,1024,768);
  Check(Close(loading.x,760) && Close(loading.width,1920) && Close(loading.height,1440),"loading image fits visible 4:3 crop, excluding square texture padding");
  for(int width:{1024,1366,1920,3440,7680}) {
    const auto strips=S2Display::StretchHorizontal(width,1024,280);
    int end=0;
    for(const auto& strip:strips) {
      Check(strip.position==end && strip.width>0,"top strip has no gaps");
      Check(strip.source>=0 && strip.source+strip.sourceWidth<=1024,"top strip stays inside original texture");
      end+=strip.width;
    }
    Check(end==width,"top strip covers actual width");
    if(width>1024) Check(strips.size()==3 && strips[1].source==280 && strips[1].sourceWidth==464,
        "one continuous turn strip with unchanged end caps");
  }
  int w=0,h=0;
  for (const auto text : {L"1024",L"\"1920x1080\"",L"1280.000000",L"3440x1440",L"7680x4320"}) Check(S2Display::ParseResolution(text,&w,&h),"legacy and large resolution");
  Check(S2Display::ParseResolution(L"1280",&w,&h)&&h==960,"legacy 1280 is 960 tall");
  for (const auto text : {L"",L"nan",L"0x0",L"-1920x1080",L"1920x1080junk",L"999999x1080",L"1280foo",L"9999999999999999999999999x1080",L"1e100"}) Check(!S2Display::ParseResolution(text,&w,&h),"invalid resolution rejected");
  Check(Close(S2Display::UIScale(0,0,2,100),1),"zero surface safe");
  Check(S2Display::Fit(100,100,0,480).width==0,"invalid video safe");
  Check(Close(S2Display::UIScale(1920,1080,std::numeric_limits<float>::quiet_NaN(),0),1),"invalid DPI safe");
  std::printf("Display geometry: %d failures\n",failures); return failures?1:0;
}
