#include "NativeHeadData.h"
#ifdef NDEBUG
#undef NDEBUG // Test assertions must run in RelWithDebInfo as well.
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

static void U32(std::vector<unsigned char> *bytes, std::size_t offset, std::uint32_t value)
{
  for (int i = 0; i < 4; ++i)
    (*bytes)[offset + i] = static_cast<unsigned char>(value >> (i * 8));
}

static void F32(std::vector<unsigned char> *bytes, std::size_t offset, float value)
{
  std::uint32_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  U32(bytes, offset, bits);
}

int main()
{
  // Synthetic minimal original-format stream: no proprietary fixture needed.
  std::vector<unsigned char> good(28 + 28 + 12 + 24, 0);
  U32(&good, 0, 0xAD5A018Du);
  U32(&good, 4, 0); // bones
  U32(&good, 8, 2); // total vertices
  U32(&good, 12, 1); // one explicit, one implicit
  U32(&good, 28, 1); // explicit vertex id
  F32(&good, 40, 1.25f);
  F32(&good, 44, -2.5f);
  F32(&good, 48, 3.75f);
  U32(&good, 52, 1); // one influence triple
  U32(&good, 56, 9);
  F32(&good, 60, 0.5f);
  F32(&good, 64, 0.75f);
  U32(&good, 68, 0); // implicit vertex id
  U32(&good, 72, 3); // stored-position variant
  F32(&good, 80, 4.0f);
  F32(&good, 84, 5.0f);
  F32(&good, 88, 6.0f);
  NativeLifeStudio::HeadData parsed;
  assert(NativeLifeStudio::DecodeHeadVertices(good.data(), good.size(), &parsed));
  assert(parsed.vertexCount == 2 && parsed.explicitVertexCount == 1);
  assert(parsed.vertices[0].index == 1 && parsed.vertices[0].influences.size() == 1);
  assert(parsed.vertices[0].sourcePosition[1] == -2.5f);
  assert(parsed.implicitVertices.size() == 1 && parsed.implicitVertices[0].index == 0);
  assert(parsed.implicitVertices[0].sourcePosition[2] == 6.0f);
  for (std::size_t length = 0; length < good.size(); ++length)
    assert(!NativeLifeStudio::DecodeHeadVertices(good.data(), length, &parsed));
  std::vector<unsigned char> bad = good;
  U32(&bad, 0, 0);
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = good;
  U32(&bad, 4, 0xFFFFFFFFu);
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = good;
  U32(&bad, 28, 2); // out-of-range vertex id
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = good;
  U32(&bad, 52, 0xFFFFFFFFu); // impossible influence count
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  return 0;
}
