#pragma once
// Index bounds shared by the renderer and its parity benchmark.
// Fixed-size memcpy supports unaligned legacy data without aliasing violations.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace S2Geometry {
template<class Index>
inline void FastIndexBounds(const unsigned char* data, unsigned count, int base,
                            std::vector<uint32_t>& source, uint32_t& low, uint32_t& high) {
  uint32_t rawLow=UINT32_MAX,rawHigh=0;
  for(unsigned i=0;i<count;++i) {
    Index index;
    std::memcpy(&index,data+size_t(i)*sizeof(Index),sizeof(Index));
    rawLow=(std::min)(rawLow,uint32_t(index));
    rawHigh=(std::max)(rawHigh,uint32_t(index));
  }
  const int64_t first=int64_t(rawLow)+base,last=int64_t(rawHigh)+base;
  if(first<0 || last>UINT32_MAX)throw std::runtime_error("Invalid game base vertex");
  low=uint32_t(first);high=uint32_t(last);
  if(!source.empty())for(unsigned i=0;i<count;++i) {
    Index index;
    std::memcpy(&index,data+size_t(i)*sizeof(Index),sizeof(Index));
    source[i]=uint32_t(int64_t(index)+base);
  }
}
}
