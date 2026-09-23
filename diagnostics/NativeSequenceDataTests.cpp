#include "NativeSequenceData.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <vector>

static void U32(std::vector<unsigned char> *bytes, std::size_t offset, std::uint32_t value)
{
  for (int i = 0; i < 4; ++i)
    (*bytes)[offset + i] = static_cast<unsigned char>(value >> (i * 8));
}

int main()
{
  std::vector<unsigned char> good(40, 0);
  U32(&good, 0, 0x46534D4Du); // MMSF
  U32(&good, 4, 1);
  U32(&good, 8, 8);
  U32(&good, 12, 30000);
  U32(&good, 16, 2);
  U32(&good, 20, 100);
  NativeLifeStudio::SequenceHeader header;
  assert(NativeLifeStudio::DecodeSequenceHeader(good.data(), good.size(), &header));
  assert(header.payloadSize == 8 && header.duration == 30000 &&
         header.trackCount == 2 && header.headerField20 == 100);
  for (std::size_t n = 0; n < good.size(); ++n)
    assert(!NativeLifeStudio::DecodeSequenceHeader(good.data(), n, &header));
  auto bad = good;
  U32(&bad, 0, 0);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 4, 2);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 8, 0xFFFFFFFFu);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 12, 0);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 16, 0);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 16, 3);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  assert(!NativeLifeStudio::DecodeSequenceHeader(nullptr, good.size(), &header));
  assert(!NativeLifeStudio::DecodeSequenceHeader(good.data(), good.size(), nullptr));
  return 0;
}
