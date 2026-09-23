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
  // Synthetic original-format stream: one muscle, one weighted and one implicit
  // vertex. No proprietary fixture is required by this unit test.
  constexpr std::size_t vertexBase = 28 + 172;
  constexpr std::size_t implicitBase = vertexBase + 28 + 12;
  std::vector<unsigned char> good(implicitBase + 24, 0);
  U32(&good, 0, 0xAD5A018Du);
  U32(&good, 4, 1); // muscles
  U32(&good, 16, 0); // bones in the still-undecoded trailing section
  U32(&good, 8, 2); // total vertices
  U32(&good, 12, 1); // one explicit, one implicit
  U32(&good, 24, 5); // fixed-size muscle record: type, name and anchors
  std::memcpy(good.data() + 28, "a_Test", 6);
  F32(&good, 24 + 68, 1.0f);
  F32(&good, 24 + 72, 2.0f);
  F32(&good, 24 + 76, 3.0f);
  F32(&good, 24 + 80, 4.0f);
  F32(&good, 24 + 84, 5.0f);
  F32(&good, 24 + 88, 6.0f);
  U32(&good, vertexBase, 1); // explicit vertex id
  F32(&good, vertexBase + 12, 1.25f);
  F32(&good, vertexBase + 16, -2.5f);
  F32(&good, vertexBase + 20, 3.75f);
  U32(&good, vertexBase + 24, 1); // one influence triple
  U32(&good, vertexBase + 28, 0); // muscle index
  F32(&good, vertexBase + 32, 0.5f);
  F32(&good, vertexBase + 36, 0.75f);
  U32(&good, implicitBase, 0); // implicit vertex id
  U32(&good, implicitBase + 4, 3); // stored-position variant
  F32(&good, implicitBase + 12, 4.0f);
  F32(&good, implicitBase + 16, 5.0f);
  F32(&good, implicitBase + 20, 6.0f);
  NativeLifeStudio::HeadData parsed;
  assert(NativeLifeStudio::DecodeHeadVertices(good.data(), good.size(), &parsed));
  assert(parsed.vertexCount == 2 && parsed.explicitVertexCount == 1);
  assert(parsed.vertices[0].index == 1 && parsed.vertices[0].influences.size() == 1);
  assert(parsed.vertices[0].influences[0].muscleIndex == 0);
  assert(parsed.vertices[0].sourcePosition[1] == -2.5f);
  assert(parsed.implicitVertices.size() == 1 && parsed.implicitVertices[0].index == 0);
  assert(parsed.implicitVertices[0].sourcePosition[2] == 6.0f);
  assert(parsed.muscleCount == 1 && parsed.boneCount == 0);
  assert(parsed.muscles.size() == 1 && parsed.muscles[0].name == "a_Test" &&
         parsed.muscles[0].type == 5 && parsed.muscles[0].pointA[1] == 2.0f &&
         parsed.muscles[0].pointB[2] == 6.0f);
  auto badBone = good;
  std::memset(badBone.data() + 28, 'A', 64); // missing name terminator
  assert(!NativeLifeStudio::DecodeHeadVertices(badBone.data(), badBone.size(), &parsed));
  badBone = good;
  U32(&badBone, 24 + 68, 0x7F800000u); // nonfinite anchor
  assert(!NativeLifeStudio::DecodeHeadVertices(badBone.data(), badBone.size(), &parsed));
  for (std::size_t length = 0; length < good.size(); ++length)
    assert(!NativeLifeStudio::DecodeHeadVertices(good.data(), length, &parsed));
  std::vector<unsigned char> bad = good;
  U32(&bad, 0, 0);
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = good;
  U32(&bad, 4, 0xFFFFFFFFu);
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = good;
  U32(&bad, vertexBase, 2); // out-of-range vertex id
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = good;
  U32(&bad, vertexBase + 24, 0xFFFFFFFFu); // impossible influence count
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = good;
  U32(&bad, vertexBase + 28, 1); // out-of-range muscle index
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  return 0;
}
