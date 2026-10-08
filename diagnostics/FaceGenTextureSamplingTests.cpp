#include "../Main/FaceGenTextureSampling.h"
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>

using S2FaceGenTextureSampling::LogicalSourceMip;
static int failures = 0, checks = 0;
static void Check(bool ok, const char* message) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}
struct Pixel { unsigned char r,g,b,a; };
struct Image {
  int width, height;
  std::vector<Pixel> pixels;
  int GetXSize() const { return width; }
  int GetYSize() const { return height; }
  Pixel* operator[](int y) { return pixels.data()+y*width; }
  const Pixel* operator[](int y) const { return pixels.data()+y*width; }
};
// Independent fixture: each logical RGBA texel occupies a constant 4x4 block
// in the offline image. Real mip averaging must recover the entire fixture,
// including both eyebrows, the opposite corner and arbitrary alpha values.
static void WholeLayer(int width, int height, int atlasX, int atlasY) {
  Image source{width,height,{}};
  source.pixels.resize(width*height);
  for (int y=0;y<height;++y)
    for (int x=0;x<width;++x)
      source.pixels[y*width+x]={static_cast<unsigned char>(x),static_cast<unsigned char>(y),static_cast<unsigned char>((x+y)&255),static_cast<unsigned char>((x*13+y*7)&255)};
  std::vector<Image> mips;
  for (int density=4;density>=1;density/=2) {
    Image image{width*density,height*density,{}};
    image.pixels.resize(image.width*image.height);
    for (int y=0;y<image.height;++y)
      for (int x=0;x<image.width;++x)
        image.pixels[y*image.width+x]=source.pixels[(y/density)*width+x/density];
    mips.push_back(image);
  }
  const Image& selected=mips[LogicalSourceMip(width*4,height*4,4,4,3)];
  Check(selected.width==width && selected.height==height,"HD source occupies logical rect");
  const Pixel fill={83,71,53,19};
  Image atlas{256,256,std::vector<Pixel>(256*256,fill)};
  S2FaceGenTextureSampling::BlendLayer(atlas,selected,atlasX,atlasY,atlasX+width,atlasY+height,1.0f);
  bool whole=true, outside=true;
  // Independent integer reference, denominator255: no floats, no copied
  // implementation expression, no RGB premultiply before the source-over step.
  auto reference=[](int src,int dst,int alpha) { return (src*alpha+dst*(255-alpha)+127)/255; };
  for (int y=0;y<256;++y)
    for (int x=0;x<256;++x) {
      const bool inside=x>=atlasX && x<atlasX+width && y>=atlasY && y<atlasY+height;
      const Pixel& actual=atlas[y][x];
      if (inside) {
        const Pixel& expected=source[height-1-(y-atlasY)][x-atlasX];
        whole&=actual.r==reference(expected.r,fill.r,expected.a) &&
          actual.g==reference(expected.g,fill.g,expected.a) &&
          actual.b==reference(expected.b,fill.b,expected.a) && actual.a==255;
      } else outside&=actual.r==fill.r && actual.g==fill.g && actual.b==fill.b && actual.a==fill.a;
    }
  Check(whole,"whole logical field uses straight RGB, full source alpha and retail V flip");
  Check(outside,"adjacent FaceGen atlas regions unchanged");
  const Pixel& oldCorner=mips[0][height-1][width-1];
  Check(oldCorner.r!=source.pixels.back().r || oldCorner.g!=source.pixels.back().g,"fixture detects old upper-left HD crop");
}
int main() {
  WholeLayer(128,128,0,128); // face, hair, eyebrows
  WholeLayer(64,64,64,64);   // eyes
  WholeLayer(64,64,0,0);     // eyelashes/teeth
  Image transparent{1,1,{{255,128,64,0}}}, dst{1,1,{{83,71,53,19}}};
  S2FaceGenTextureSampling::BlendLayer(dst,transparent,0,0,1,1,1.0f);
  Check(dst[0][0].r==83 && dst[0][0].g==71 && dst[0][0].b==53 && dst[0][0].a==255,"alpha zero keeps backdrop RGB, authored fill is opaque after blend");
  Image opaque{1,1,{{255,0,83,255}}};
  S2FaceGenTextureSampling::BlendLayer(dst,opaque,0,0,1,1,0.5f);
  Check(dst[0][0].r==169 && dst[0][0].g==36 && dst[0][0].b==68 && dst[0][0].a==255,"actual half-weight source-over rounds correctly without double premultiply");
  Check(LogicalSourceMip(128,128,1,1,8)==0,"retail source stays byte-identical level zero");
  Check(LogicalSourceMip(512,512,4,4,10)==2,"actual HD6715 full logical128 source");
  Check(LogicalSourceMip(256,256,4,4,9)==2,"HD eye/eyelash full logical64 source");
  Check(LogicalSourceMip(260,132,4,4,7)==2,"exact NPOT logical65x33 mip");
  Check(LogicalSourceMip(512,256,2,2,9)==1,"density two keeps original logical region");
  Check(LogicalSourceMip(512,512,1,1,10)==0,"large original retains retail cropping");
  Check(LogicalSourceMip(512,512,4,4,2)==0,"incomplete unknown chain keeps original behavior");
  Check(LogicalSourceMip(512,512,0,4,10)==0,"invalid density guarded");
  Check(LogicalSourceMip(512,512,std::numeric_limits<float>::infinity(),4,10)==0,"nonfinite metadata guarded");
  std::printf("FaceGenTextureSamplingTests: %d checks, %d failures\n",checks,failures);
  return failures?1:0;
}
