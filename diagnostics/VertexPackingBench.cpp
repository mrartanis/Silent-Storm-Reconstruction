// Byte-parity and throughput check against the original generic converter.
// Checks byte identity against the live backend before measuring throughput.
#include "../Main/StdAfx.h"
#undef for
#include "../Main/BgfxBackend.h"
#include "FrameProfiler.h"
#include <vector>
#include <cstdio>
#include <cstring>

static void Reference(void* output,const void* input,unsigned count,const NGfx::BgfxDeclaration& d) {
  memset(output,0,count*d.gpuLayout.getStride());
  for(unsigned vertex=0;vertex<count;++vertex)for(unsigned a=0;a<bgfx::Attrib::Count;++a) {
    auto attr=bgfx::Attrib::Enum(a);if(!d.layout.has(attr))continue;
    float value[4]={0,0,0,1};bgfx::vertexUnpack(value,attr,d.layout,input,vertex);
    uint8_t n;bgfx::AttribType::Enum type;bool normalized,asInt;d.layout.decode(attr,n,type,normalized,asInt);
    if(type==bgfx::AttribType::Int16)for(unsigned c=0;c<n;++c) {
      int16_t v;memcpy(&v,static_cast<const unsigned char*>(input)+vertex*d.layout.getStride()+d.layout.getOffset(attr)+c*2,2);value[c]=float(v);
    }
    if(type==bgfx::AttribType::Uint8)std::swap(value[0],value[2]);
    bgfx::vertexPack(value,false,attr,d.gpuLayout,output,vertex);
  }
}
int main() {
  const D3DVERTEXELEMENT9 full[]={
    {0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},
    {0,12,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_NORMAL,0},
    {0,16,D3DDECLTYPE_SHORT2,0,D3DDECLUSAGE_TEXCOORD,0},
    {0,20,D3DDECLTYPE_SHORT2,0,D3DDECLUSAGE_TEXCOORD,1},
    {0,24,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_TANGENT,0},
    {0,28,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_TANGENT,1},D3DDECL_END()};
  const D3DVERTEXELEMENT9 tc[]={
    {0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},
    {0,12,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_COLOR,0},
    {0,16,D3DDECLTYPE_FLOAT2,0,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
  const D3DVERTEXELEMENT9 nt[]={
    {0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},
    {0,12,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_NORMAL,0},
    {0,24,D3DDECLTYPE_FLOAT2,0,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
  const D3DVERTEXELEMENT9 padded[]={
    {0,4,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},
    {0,20,D3DDECLTYPE_SHORT2,0,D3DDECLUSAGE_TEXCOORD,0},
    {0,28,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_COLOR,1},D3DDECL_END()};
  const D3DVERTEXELEMENT9* formats[]={full,tc,nt,padded};
  const char* names[]={"full","tc","nt","padded"};
  std::puts("format,vertices,repeats,reference_ms,live_ms,speedup,byte_equal");
  for(unsigned f=0;f<4;++f) {
    NGfx::BgfxDeclaration d(formats[f]); const auto& attrs=d.attributes;
    const unsigned count=65536,repeats=40;
    std::vector<unsigned char> input(count*d.layout.getStride()), baseline(count*d.gpuLayout.getStride()), prototype(baseline.size());
    // Cover every color byte and signed-short pattern, with finite position/UV floats.
    for(unsigned i=0;i<input.size();++i) input[i]=uint8_t(i*37+13);
    for(unsigned v=0;v<count;++v) for(const auto& a:attrs) {
      auto* data=input.data()+v*d.layout.getStride()+a.input;
      if(a.type==bgfx::AttribType::Float) {
        float values[]={float(int(v%97)-48)/3.0f, float(v%31)/7.0f, -0.0f};
        std::memcpy(data,values,a.components*4);
      } else if(a.type==bgfx::AttribType::Int16) {
        for(unsigned c=0;c<a.components;++c) {uint16_t value=uint16_t(v+c*32471);std::memcpy(data+c*2,&value,2);}
      } else for(unsigned c=0;c<a.components;++c)data[c]=uint8_t(v*37+c*61);
    }
    Reference(baseline.data(),input.data(),count,d);
    d.CopyVertices(prototype.data(),input.data(),count);
    if(baseline!=prototype) { std::fprintf(stderr,"Parity failed: %s\n",names[f]); return 1; }
    auto start=S2Perf::Clock::now();
    for(unsigned n=0;n<repeats;++n) Reference(baseline.data(),input.data(),count,d);
    double oldMs=S2Perf::Ms(start,S2Perf::Clock::now());
    start=S2Perf::Clock::now();
    for(unsigned n=0;n<repeats;++n) d.CopyVertices(prototype.data(),input.data(),count);
    double newMs=S2Perf::Ms(start,S2Perf::Clock::now());
    if(baseline!=prototype) return 2;
    std::printf("%s,%u,%u,%.3f,%.3f,%.3f,1\n",names[f],count,repeats,oldMs,newMs,oldMs/newMs);
  }
  return 0;
}
