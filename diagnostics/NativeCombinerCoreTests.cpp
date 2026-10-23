#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/BasicChunk1.h"
#include "../Main/GCombiner.h"
#include "../Main/GCompactVectorTransform.h"
#if defined(_WIN32)
#include "../Main/GGeometry.h"
#endif

#include <cstdint>
#include <cstdio>

namespace {
#if defined(_WIN32)
class CTestMesh : public CPtrFuncBase<NGScene::CObjectInfo> {
  OBJECT_NOCOPY_METHODS(CTestMesh);
protected:
  bool NeedUpdate() override { return false; }
  void Recalc() override {}
public:
  CTestMesh() {}
  explicit CTestMesh(NGScene::CObjectInfo* mesh) { pValue = mesh; }
};
class CGeometryPart : public NGScene::IPart {
  OBJECT_NOCOPY_METHODS(CGeometryPart);
public:
  CGeometryPart() {}
  explicit CGeometryPart(CTestMesh* mesh) : IPart(mesh, nullptr, false) {}
};
class CTestParts : public CFuncBase<vector<CPtr<NGScene::IPart>>> {
  OBJECT_NOCOPY_METHODS(CTestParts);
protected:
  bool NeedUpdate() override { return false; }
public:
  void Add(NGScene::IPart* part) { value.push_back(part); }
};
bool CheckSeamedMeshIndexBoundary() {
  // UV seams keep distinct vertices while merging their geometric positions.
  // Two legal 16-bit meshes together need a second base-vertex batch.
  const int vertices = 39999;
  NGScene::CObjectInfo::SData data;
  data.verts.resize(vertices);
  vector<STriangle> triangles;
  for (int i = 0; i < vertices; ++i) {
    auto& v = data.verts[i];
    v.pos = CVec3(i % 3 == 1 ? 1.f : 0.f, i % 3 == 2 ? 1.f : 0.f, 0.f);
    v.tex = CVec2(float(i) / vertices, 0.f);
    if (i % 3 == 0) {
      STriangle triangle; triangle.i1 = i; triangle.i2 = i + 1; triangle.i3 = i + 2;
      triangles.push_back(triangle);
    }
  }
  data.geometry.SetTriangles(triangles);
  CObj<NGScene::CObjectInfo> mesh = new NGScene::CObjectInfo;
  mesh->Assign(data);
  if (mesh->GetVertices().size() != vertices || mesh->GetPositions().size() != 3) return false;
  CObj<CTestMesh> node = new CTestMesh(mesh);
  CObj<CGeometryPart> first = new CGeometryPart(node), second = new CGeometryPart(node);
  CObj<CTestParts> parts = new CTestParts;
  parts->Add(first); parts->Add(second);
  CObj<NGScene::CIBCombiner> indices = new NGScene::CIBCombiner(parts, NGScene::LT_NONE, NGScene::IBTT_VERTICES);
  ++nDGCurrentFrame;
  CDGPtr<NGScene::CIBCombiner> pointer(indices.GetPtr()); pointer.Refresh();
  const auto& lists = indices->GetValue();
  if (lists.size() != 2 || lists[1].nBaseIndex != vertices) {
    std::printf("seamed mesh base index=%d expected=%d\n", lists.size() == 2 ? lists[1].nBaseIndex : -1, vertices);
    return false;
  }
  for (int part = 0; part < 2; ++part) {
    for (int t = 0; t < lists[part].nTris; ++t) {
      const auto& triangle = lists[part].pTri[t];
      const int base = lists[part].nBaseIndex;
      for (int index : {int(triangle.i1), int(triangle.i2), int(triangle.i3)})
        if (base + index < part * vertices || base + index >= (part + 1) * vertices) return false;
    }
  }
  return true;
}
#endif
bool CheckPackedBasis() {
  // Explicit MMX fixtures: identity, 90-degree yaw, 90-degree pitch.
  const NGfx::SCompactTransformer transforms[] = {
    {{2048, 2048, 2048, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{2048, 0, 0, 0}, {0, 0, -2048, 0}, {0, 2048, 0, 0}},
    {{0, 2048, 0, 0}, {-2048, 0, 0, 0}, {0, 0, 2048, 0}}
  };
  const CVec3 bases[] = {CVec3(1,0,0), CVec3(0,1,0), CVec3(0,0,1)};
  const CVec3 expected[][3] = {
    {CVec3(1,0,0), CVec3(0,1,0), CVec3(0,0,1)},
    {CVec3(0,1,0), CVec3(-1,0,0), CVec3(0,0,1)},
    {CVec3(0,0,-1), CVec3(0,1,0), CVec3(1,0,0)}
  };
  for (int matrix = 0; matrix < 3; ++matrix) {
    for (int basis = 0; basis < 3; ++basis) {
      NGfx::SCompactVector packed{};
      NGfx::CalcCompactVector(&packed, bases[basis]);
      const CVec3 actual = NGScene::TransformCompactVector(&packed, &transforms[matrix]);
      if (fabs2(actual - expected[matrix][basis]) > 1e-8f) {
        std::printf("packed basis mismatch matrix=%d basis=%d got=(%.6f,%.6f,%.6f)\n",
          matrix, basis, actual.x, actual.y, actual.z);
        return false;
      }
    }
  }
  return true;
}

class CTestPart : public NGScene::IPart {
  OBJECT_NOCOPY_METHODS(CTestPart);
  std::uintptr_t sortValue;
public:
  CTestPart() : sortValue(0) {}
  explicit CTestPart(std::uintptr_t value, bool solid = false)
    : NGScene::IPart(nullptr, nullptr, solid), sortValue(value) {}
  std::uintptr_t GetSortValue() const override { return sortValue; }
};

std::uint64_t Digest(CMemoryStream& stream) {
  std::uint64_t hash = UINT64_C(14695981039346656037);
  for (int i = 0; i < stream.GetSize(); ++i) {
    hash ^= stream.GetBuffer()[i];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}
} // namespace

int main() {
#if defined(_WIN32)
  if (!CheckSeamedMeshIndexBoundary()) return 8;
#endif
  if (!CheckPackedBasis()) return 7;
  NGScene::SStaticTrackers trackers;
  CObj<NGScene::CPerMaterialCombiner> combiner =
    new NGScene::CPerMaterialCombiner(&trackers);
  CObj<NGScene::CAutomaticCombiner> automatic = new NGScene::CAutomaticCombiner;
  if (!pSSClasses ||
      pSSClasses->GetTypeID(static_cast<NGScene::CPerMaterialCombiner*>(nullptr)) != 0x02741133 ||
      pSSClasses->GetTypeID(static_cast<NGScene::CAutomaticCombiner*>(nullptr)) != 0x01091206)
    return 6;
  CObj<CTestPart> first = new CTestPart(7, true);
  CObj<CTestPart> second = new CTestPart(19);
  first->SetCombiner(combiner, false, false);
  second->SetCombiner(combiner, false, false);
  if (combiner->GetSize() != 2) return 1;

  CDGPtr<NGScene::CPerMaterialCombiner> pointer(combiner.GetPtr());
  pointer.Refresh();
  const auto& ordered = combiner->GetValue();
  if (ordered.size() != 2 || ordered[0] != second.GetPtr() ||
      ordered[1] != first.GetPtr()) return 2;
  first->SetCombiner(combiner, true, true);
  if (combiner->GetSize() != 2) return 3;
  first->SetCombiner(nullptr, false, false);
  second->SetCombiner(nullptr, false, false);
  if (combiner->GetSize() != 0) return 4;

  CMemoryStream stream;
  {
    CStructureSaver saver(stream, CStructureSaver::WRITE);
    saver.Add(1, &combiner);
    saver.Add(2, &automatic);
  }
  const std::uint64_t digest = Digest(stream);
  combiner = nullptr;
  automatic = nullptr;
  stream.SetRMode();
  stream.Seek(0);
  {
    CStructureSaver saver(stream, CStructureSaver::READ);
    saver.Add(1, &combiner);
    saver.Add(2, &automatic);
  }
  if (!combiner || combiner->GetSize() != 0 || !automatic) {
    std::printf("combiner load mismatch ptr=%p size=%d bytes=%d\n",
      static_cast<void*>(combiner.GetPtr()),
      combiner ? combiner->GetSize() : -1, stream.GetSize());
    return 5;
  }
  std::printf("combiner core bytes=%d fnv=%016llX\n", stream.GetSize(),
    static_cast<unsigned long long>(digest));
  return 0;
}
