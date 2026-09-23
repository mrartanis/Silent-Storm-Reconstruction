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
  for (int i = 0; i < 4; ++i)
    F32(&good, 24 + 96 + i * 4, float(i + 1));
  for (int i = 0; i < 5; ++i)
    F32(&good, 24 + 132 + i * 4, 1.0f - i * 0.25f);
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
  std::vector<unsigned char> withBone(good.size() + 168, 0);
  std::memcpy(withBone.data(), good.data(), implicitBase);
  std::memcpy(withBone.data() + implicitBase + 168, good.data() + implicitBase, 24);
  U32(&withBone, 16, 1);
  std::memcpy(withBone.data() + implicitBase, "b_Test", 6);
  U32(&withBone, implicitBase + 64, 1);
  F32(&withBone, implicitBase + 68, 1.0f);
  F32(&withBone, implicitBase + 116, 2.0f);
  U32(&withBone, implicitBase + 164, 0);
  assert(NativeLifeStudio::DecodeHeadVertices(withBone.data(), withBone.size(), &parsed));
  assert(parsed.boneCount == 1 && parsed.bones.size() == 1 &&
         parsed.bones[0].name == "b_Test" && parsed.bones[0].matrixA[0] == 1.0f &&
         parsed.bones[0].matrixB[0] == 2.0f &&
         parsed.bones[0].muscleIndices.size() == 1 &&
         parsed.bones[0].muscleIndices[0] == 0);
  auto badBoneSection = withBone;
  std::memset(badBoneSection.data() + implicitBase, 'A', 64);
  assert(!NativeLifeStudio::DecodeHeadVertices(badBoneSection.data(), badBoneSection.size(), &parsed));
  badBoneSection = withBone;
  U32(&badBoneSection, implicitBase + 64, 2); // more attachments than muscles
  assert(!NativeLifeStudio::DecodeHeadVertices(badBoneSection.data(), badBoneSection.size(), &parsed));
  badBoneSection = withBone;
  U32(&badBoneSection, implicitBase + 164, 1); // out-of-range muscle index
  assert(!NativeLifeStudio::DecodeHeadVertices(badBoneSection.data(), badBoneSection.size(), &parsed));
  badBoneSection = withBone;
  U32(&badBoneSection, implicitBase + 68, 0x7F800000u); // nonfinite matrix
  assert(!NativeLifeStudio::DecodeHeadVertices(badBoneSection.data(), badBoneSection.size(), &parsed));
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

  // Synthetic 0x37D30DC0 IAnimator::Save stream: variable-length names,
  // one coefficient per influence, compact bones and implicit positions.
  constexpr std::size_t savedMusclePayload = 32 + 4 + 1 + 6;
  constexpr std::size_t savedVertexSection = savedMusclePayload + 108;
  constexpr std::size_t savedVertex = savedVertexSection + 4;
  constexpr std::size_t savedBone = savedVertex + 20 + 8;
  constexpr std::size_t savedBonePayload = savedBone + 1 + 6;
  constexpr std::size_t savedImplicit = savedBonePayload + 108 + 4;
  std::vector<unsigned char> saved(savedImplicit + 16, 0);
  U32(&saved, 0, 0x37D30DC0u);
  U32(&saved, 4, 1); // one muscle
  U32(&saved, 8, 2); // total vertices
  U32(&saved, 12, 1); // explicit vertices
  U32(&saved, 16, 1); // bones
  U32(&saved, 32, 5);
  saved[36] = 6;
  std::memcpy(saved.data() + 37, "a_Test", 6);
  F32(&saved, savedMusclePayload, 1.0f);
  F32(&saved, savedMusclePayload + 12, 2.0f);
  for (int i = 0; i < 4; ++i)
    F32(&saved, savedMusclePayload + 32 + i * 4, float(i + 1));
  for (int i = 0; i < 5; ++i)
    F32(&saved, savedMusclePayload + 68 + i * 4, 1.0f - i * 0.25f);
  U32(&saved, savedVertexSection, 5);
  U32(&saved, savedVertex, 1);
  F32(&saved, savedVertex + 4, 1.75f);
  F32(&saved, savedVertex + 8, 1.0f);
  F32(&saved, savedVertex + 12, 0.0f);
  U32(&saved, savedVertex + 16, 1);
  U32(&saved, savedVertex + 20, 0);
  F32(&saved, savedVertex + 24, 0.75f);
  saved[savedBone] = 6;
  std::memcpy(saved.data() + savedBone + 1, "b_Test", 6);
  U32(&saved, savedBonePayload, 1);
  F32(&saved, savedBonePayload + 8, 1.0f);
  F32(&saved, savedBonePayload + 56, 2.0f);
  U32(&saved, savedBonePayload + 108, 0);
  U32(&saved, savedImplicit, 0);
  F32(&saved, savedImplicit + 4, 6.0f);
  F32(&saved, savedImplicit + 8, 7.0f);
  F32(&saved, savedImplicit + 12, 8.0f);
  assert(NativeLifeStudio::DecodeHeadVertices(saved.data(), saved.size(), &parsed));
  assert(parsed.muscleCount == 1 && parsed.boneCount == 1 && parsed.vertexCount == 2);
  assert(parsed.muscles[0].name == "a_Test" && parsed.muscles[0].pointB[0] == 2.0f);
  assert(parsed.vertices[0].index == 1 && parsed.vertices[0].sourcePosition[1] == 1.0f);
  assert(parsed.vertices[0].influences[0].componentA == 0.75f &&
         parsed.vertices[0].influences[0].componentB == 0.75f);
  assert(parsed.bones[0].name == "b_Test" && parsed.bones[0].matrixB[0] == 2.0f &&
         parsed.bones[0].muscleIndices[0] == 0);
  assert(parsed.implicitVertices[0].index == 0 &&
         parsed.implicitVertices[0].sourcePosition[2] == 8.0f);
  for (std::size_t length = 0; length < saved.size(); ++length)
    assert(!NativeLifeStudio::DecodeHeadVertices(saved.data(), length, &parsed));
  bad = saved;
  U32(&bad, savedVertex + 16, 0xFFFFFFFFu);
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = saved;
  U32(&bad, savedBonePayload, 2);
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = saved;
  F32(&bad, savedMusclePayload + 36, 0.5f); // non-increasing falloff positions
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  bad = saved;
  U32(&bad, savedMusclePayload + 68, 0x7F800000u); // nonfinite falloff value
  assert(!NativeLifeStudio::DecodeHeadVertices(bad.data(), bad.size(), &parsed));
  return 0;
}
