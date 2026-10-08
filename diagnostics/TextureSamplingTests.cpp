#include "../Main/TextureSampling.h"
#include <cstdio>
#include <cstdint>
#include "../Misc/PortableFloat2Int.h"

using S2TextureSampling::Rect;
using S2TextureSampling::SampleRect;
using S2TextureSampling::TexelScale;
static int failures = 0, checks = 0;
static void Check(bool ok, const char* name) {
  ++checks;
  if (!ok) { std::fprintf(stderr, "FAIL: %s\n", name); ++failures; }
}
static bool Close(float a, float b) { return std::fabs(a-b) < 0.00002f; }
static bool Equal(const Rect& a, const Rect& b) {
  return Close(a.x1,b.x1) && Close(a.y1,b.y1) && Close(a.x2,b.x2) && Close(a.y2,b.y2);
}
// Actual portable Float2Int rounding + signed SHORT2 storage + backend int16
// expansion + Render2D c16 multiplication. This tests the loss point that a
// floating-point-only SampleRect test cannot detect.
static float GPUCoordinate(float physical, float steps, int holder, bool np2=false) {
  const int packed=S2Math::Float2IntWithCurrentRounding(physical*steps);
  Check(packed>=-32768 && packed<=32767,"packed vertex coordinate fits signed SHORT2");
  const std::int16_t vertex=static_cast<std::int16_t>(packed);
  return float(vertex)*S2TextureSampling::PackedUVScale(steps,holder,np2);
}
static Rect GPUWhole(const Rect& source,int width,int height) {
  const float steps=S2TextureSampling::UVPackingSteps(source,width,height);
  return {GPUCoordinate(source.x1,steps,width),GPUCoordinate(source.y1,steps,height),
          GPUCoordinate(source.x2,steps,width),GPUCoordinate(source.y2,steps,height)};
}
int main() {
  Check(TexelScale(4096,0)==1 && TexelScale(128,-1)==1, "manual/font default density1");
  Check(TexelScale(128,32)==4, "accepted POT6094 density from actual selected dimensions");
  Check(TexelScale(2060,515)==4 && TexelScale(520,130)==4, "NPOT3455 exact dimensions density4");
  Check(TexelScale(64,256)==0.25f, "LR reload derives density below1");
  Check(Close(TexelScale(1,107),1.0f/107), "average-color fallback fills one physical texel");

  const Rect target{0,0,515,130}, source{0,130,515,0};
  Check(Equal(SampleRect(target,source,1,1,0,0),source), "density1 unchanged logical endpoints");
  const auto full=SampleRect(target,source,4,4,0,0);
  Check(Equal(full,{0,520,2060,0}), "asymmetric full NPOT4x covers entire sheet, reversed Y");
  Check(Close((full.x2-full.x1)/2060,1) && Close((full.y1-full.y2)/520,1), "full normalized extent1 instead of quartercrop");
  const auto atlas=SampleRect(target,source,4,4,101,79);
  Check(Equal(atlas,{101,599,2161,79}), "nonzero physical atlas placement not multiplied by density");
  Check(Close(atlas.x1/4096,101.0f/4096) && Close(atlas.x2/4096,2161.0f/4096), "holder normalization preserves exact placed edge-to-edge span");
  Check(Equal(SampleRect({20,30,50,60},{7,58,37,28},4,4,101,79),{129,311,249,191}), "clipped partial source scales sampling only");
  Check(Equal(SampleRect({50,60,20,30},{37,28,7,58},4,4,101,79),{249,191,129,311}), "reversed target/source preserve orientation");
  Check(Equal(SampleRect({0,0,15,20},{5,30,20,10},4,2,10,7),{30,67,90,27}), "independent X/Y physical-to-logical ratios");

  // Frozen retail golden values: enlargement correction occurs in logical units.
  const auto corrected=SampleRect({0,0,8,6},{2,12,6,8},1,1,0,0);
  Check(Equal(corrected,{16.0f/7,11.8f,40.0f/7,8.2f}), "old halftexel enlargement correction exact golden endpoints");
  Check(Equal(SampleRect({0,0,8,6},{2,12,6,8},4,4,16,40),
              {16+64.0f/7,87.2f,16+160.0f/7,72.8f}), "density applied after logical halftexel correction, then placement");
  Check(Equal(SampleRect({0,0,2,2},{2,12,6,8},1,1,0,0),{2,12,6,8}), "retail downscale endpoints unchanged");
  Check(Equal(SampleRect({0,0,8,6},{6,8,2,12},1,1,0,0),{40.0f/7,8.2f,16.0f/7,11.8f}), "old reversed-source enlargement semantics");
  Check(Equal(SampleRect({0,0,1,1},{0,0,1,1},4,4,0,0),{0,0,4,4}), "one-logical-pixel exact full span");
  Check(Equal(SampleRect({0,0,8,6},{2,3,2,3},4,4,16,40),{24,52,24,52}), "zero source extent stays finite");
  Check(Equal(SampleRect({0,0,256,256},{0,256,256,0},0.25f,0.25f,0,0),{0,64,64,0}), "LR whole source endpoints use selected64 physical pixels");
  Check(Equal(SampleRect({0,0,107,39},{0,39,107,0},TexelScale(1,107),TexelScale(1,39),0,0),{0,1,1,0}), "fake fallback fills complete average texel");

  // Physical API callers deliberately pass density1 even for a file carrying HD metadata.
  Check(Equal(SampleRect({0,0,2060,520},{0,520,2060,0},1,1,0,0),{0,520,2060,0}), "ShowTexture/CopyTexture physical full-sheet API not scaled again");
  Check(Equal(target,{0,0,515,130}) && Equal(source,{0,130,515,0}), "sample helper leaves geometry and caller source immutable");

  Check(S2Math::Float2IntWithCurrentRounding(4096.0f*8)==32768 &&
        static_cast<std::int16_t>(32768)==-32768,"reproduce old4096 signed SHORT2 overflow");
  Check(S2TextureSampling::UVPackingSteps({0,0,1024,1024},1024,1024)==8,
        "retail small holder keeps eight steps exactly");
  Check(S2TextureSampling::UVPackingSteps({0,0,4095,4095},4095,4095)==8,
        "last integer below overflow keeps retail precision");
  Check(S2TextureSampling::UVPackingSteps({0,0,4096,4096},4096,4096)==4,
        "4096 holder uses four steps without coordinate clamp");
  const auto hdWhole=SampleRect({0,0,300,300},{0,0,1024,1024},4,4,0,0);
  Check(Equal(GPUWhole(hdWhole,4096,4096),{0,0,1,1}),"5425 whole normalizes exact full4096 span");
  Check(Equal(GPUWhole(SampleRect({0,0,300,300},{1024,1024,0,0},4,4,0,0),4096,4096),{1,1,0,0}),
        "5425 reversed full retains positive endpoints and orientation");
  Check(Equal(GPUWhole(SampleRect({0,0,300,300},{256,256,768,768},4,4,0,0),4096,4096),{.25f,.25f,.75f,.75f}),
        "5425 middle50percent retains quarter to three-quarter sampling");
  Check(Equal(GPUWhole(SampleRect({60,20,240,140},{204.8f,128,819.2f,896},4,4,0,0),4096,4096),{.20001220703125f,.125f,.79998779296875f,.875f}),
        "5425 clipped partial endpoints keep subtexel precision without bands");
  Check(Equal(GPUWhole({0,0,8204,2084},8204,2084),{0,0,1,1}),
        "large asymmetric NPOT whole covers full physical extent");
  Check(S2TextureSampling::UVPackingSteps({0,0,8204,2084},8204,2084)==2,
        "large NPOT selects safe multiplier from longest physical side");
  Check(Equal(GPUWhole({8204,2084,0,0},8204,2084),{1,1,0,0}),
        "large NPOT reversed whole has no signed wrap");
  Check(Equal(GPUWhole({101,79,2161,599},8192,4096),{101.0f/8192,79.0f/4096,2161.0f/8192,599.0f/4096}),
        "nonzero atlas physical placement packed before holder normalization");
  Check(S2TextureSampling::UVPackingSteps({0,0,16384,16384},16384,16384)==1 &&
        Equal(GPUWhole({0,0,16384,16384},16384,16384),{0,0,1,1}),
        "backend maximum16384 full span does not overflow");
  Check(S2TextureSampling::UVPackingSteps({-4096,-4096,4096,4096},4096,4096)==4 &&
        Equal(GPUWhole({-4096,-4096,4096,4096},4096,4096),{-1,-1,1,1}),
        "negative physical coordinates remain unclamped");
  Check(S2TextureSampling::UVPackingSteps({0,0,32768,1024},4096,4096)==.5f &&
        Close(GPUCoordinate(32768,.5f,4096),8),
        "coordinates beyond holder bounds use lower precision without crop");
  Check(Close(GPUCoordinate(2060,8,2060,true),2060),
        "legacy unnormalizedNP2 decode retains physical coordinate");
  Check(GPUCoordinate(17.375f,8,1024)==17.375f/1024,
        "small textures preserve exact old eighth-texel value");
  Check(S2TextureSampling::TexelUVScale(4096,false)==1.0f/4096 &&
        S2TextureSampling::PackedUVScale(4,4096,false)*4==1.0f/4096,
        "large texture blur offset remains one physical texel independently of packing4");
  Check(S2TextureSampling::TexelUVScale(8204,false)==1.0f/8204 &&
        S2TextureSampling::TexelUVScale(2084,false)==1.0f/2084,
        "asymmetric NPOT blur preserves independent physical texel offsets");
  Check(S2TextureSampling::TexelUVScale(2060,true)==1 &&
        S2TextureSampling::TexelUVScale(0,false)==1,
        "legacyNP2 and fallback blur scales retain prior convention");
  const auto logicalCorrected=SampleRect({0,0,8,6},{2,12,6,8},4,4,4096,4096);
  const float precision=S2TextureSampling::UVPackingSteps(logicalCorrected,8192,8192);
  Check(std::fabs(GPUCoordinate(logicalCorrected.x1,precision,8192)*8192-(4096+64.0f/7))<=.5f/precision,
        "logical halftexel correction survives atlas packing within declared quantization");
  std::printf("TextureSamplingTests: %d checks, %d failures\n",checks,failures);
  return failures ? 1 : 0;
}
