#include "../Main/VectorFontRasterizer.h"
#include <algorithm>
#include <cstdio>
static int failures=0;
static void Check(bool ok,const char* name) { if (!ok) { std::fprintf(stderr,"FAIL: %s\n",name); ++failures; } }
int main(int argc,char** argv) {
  if (argc!=2) return 2;
  for (const auto family : {"System","Times New Roman","Courier"}) for (int size : {12,16,20,24,32,48}) {
    S2Fonts::Atlas atlas; std::string error;
    bool ok=S2Fonts::Rasterize(std::string(argv[1])+"/"+S2Fonts::FamilyFile(family),size,&atlas,&error);
    Check(ok,"rasterize packaged font"); if(!ok) { std::fprintf(stderr,"%s\n",error.c_str()); continue; }
    Check(atlas.lineHeight>=size && atlas.averageWidth>0,"font metrics");
    for (unsigned code : {65u,120u,63u,0x416u,0x451u}) Check(std::any_of(atlas.glyphs.begin(),atlas.glyphs.end(),[code](const S2Fonts::Glyph& g){return g.code==code;}),"Latin and Cyrillic coverage");
    Check(std::any_of(atlas.rgba.begin(),atlas.rgba.end(),[](unsigned char c){return c!=0;}),"nonempty raster");
    for(std::size_t i=0;i<atlas.rgba.size();i+=4) Check(atlas.rgba[i]==atlas.rgba[i+3],"premultiplied alpha");
    for(const auto& g:atlas.glyphs) Check(g.x>=0&&g.y>=0&&g.x+g.width<=atlas.width&&g.y+g.height<=atlas.height,"glyph bounds");
    if(std::string(family)=="Times New Roman"&&size==24)
      Check(std::any_of(atlas.kerns.begin(),atlas.kerns.end(),[](const S2Fonts::Kern& k){return k.previous=='A'&&k.current=='V'&&k.adjustment<0;}),"AV kerning");
  }
  S2Fonts::Atlas atlas; std::string error;
  Check(!S2Fonts::Rasterize("missing.ttf",16,&atlas,&error),"missing font diagnosed");
  Check(S2Fonts::FamilyFile("unknown").empty(),"unknown family left for bitmap fallback");
  std::printf("Vector fonts: %d failures\n",failures); return failures?1:0;
}
