#include "../Main/IndexBounds.h"
#include <chrono>
#include <cstdio>
#include <random>

// Keep the reference scanner's runtime-sized memcpy and per-index validation.
void Reference(const unsigned char* data,unsigned count,unsigned size,int base,
               std::vector<uint32_t>& source,uint32_t& low,uint32_t& high) {
  low=UINT32_MAX;high=0;
  for(unsigned i=0;i<count;++i) {
    uint32_t index=0;std::memcpy(&index,data+size_t(i)*size,size);
    int64_t effective=int64_t(index)+base;
    if(effective<0 || effective>UINT32_MAX)throw std::runtime_error("Invalid game base vertex");
    if(!source.empty())source[i]=uint32_t(effective);
    low=std::min(low,uint32_t(effective));high=std::max(high,uint32_t(effective));
  }
}
int main() {
  std::mt19937 random(123);
  unsigned cases=0;
  for(unsigned size:{2u,4u})for(unsigned count:{1u,3u,15u,256u,16384u})
    for(unsigned offset:{0u,1u,3u})for(int base:{-70000,-1,0,17,INT32_MAX})
      for(bool copy:{false,true}) {
        std::vector<unsigned char> data(offset+size_t(count)*size);
        for(unsigned i=0;i<count;++i) {
          uint32_t value=random();
          if(i==0)value=0;if(i==count-1)value=size==2?65535:UINT32_MAX;
          std::memcpy(data.data()+offset+size_t(i)*size,&value,size);
        }
        std::vector<uint32_t> a(copy?count:0),b=a;
        uint32_t al=0,ah=0,bl=0,bh=0;bool ae=false,be=false;
        try{Reference(data.data()+offset,count,size,base,a,al,ah);}catch(const std::exception&){ae=true;}
        try{
          if(size==2)S2Geometry::FastIndexBounds<uint16_t>(data.data()+offset,count,base,b,bl,bh);
          else S2Geometry::FastIndexBounds<uint32_t>(data.data()+offset,count,base,b,bl,bh);
        }catch(const std::exception&){be=true;}
        if(ae!=be || (!ae && (al!=bl || ah!=bh || a!=b)))return 1;
        ++cases;
      }
  std::printf("Parity: %u cases, 16/32-bit, unaligned inputs, signed base, overflow, normalized output passed\n",cases);
  for(unsigned size:{2u,4u}) {
    constexpr unsigned count=16384,repeats=1000;
    std::vector<unsigned char> data(size_t(count)*size);
    for(auto& byte:data)byte=static_cast<unsigned char>(random());
    std::vector<uint32_t> empty;uint32_t low=0,high=0;volatile uint32_t checksum=0;
    auto start=std::chrono::steady_clock::now();
    for(unsigned n=0;n<repeats;++n){Reference(data.data(),count,size,0,empty,low,high);checksum=checksum^low^high;}
    const double oldMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    start=std::chrono::steady_clock::now();
    for(unsigned n=0;n<repeats;++n){
      if(size==2)S2Geometry::FastIndexBounds<uint16_t>(data.data(),count,0,empty,low,high);
      else S2Geometry::FastIndexBounds<uint32_t>(data.data(),count,0,empty,low,high);
      checksum=checksum^low^high;
    }
    const double newMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::printf("%u-bit: reference %.3f ms, probe %.3f ms, %.2fx (checksum %u)\n",size*8,oldMs,newMs,oldMs/newMs,checksum);
  }
}
