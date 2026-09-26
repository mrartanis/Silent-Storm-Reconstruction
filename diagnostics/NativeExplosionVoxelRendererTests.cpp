#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiObject.h"
#include "../Main/aiInterval.h"
#include "../Main/aiVoxelRender.h"

#include <cstdio>

namespace {

class CTestSource : public CObjectBase {
  OBJECT_NOCOPY_METHODS(CTestSource);
 public:
  CTestSource() = default;
};

void TraceTriangle(NAI::CExplVoxelRenderer* renderer, CObjectBase* object,
                   int user_id, const CVec3& offset, bool terrain) {
  std::vector<CVec3> points = {
      offset + CVec3(0, 0, 0), offset + CVec3(2, 0, 0),
      offset + CVec3(0, 2, 0)};
  NAI::CEdgesInfo triangles;
  triangles.edges.emplace_back(0, 1);
  triangles.edges.emplace_back(1, 2);
  triangles.mesh.emplace_back(0, 1, 0);
  SFBTransform transform;
  Identity(&transform.forward);
  Identity(&transform.backward);
  NAI::SSourceInfo source(object, nullptr, 0, 0);
  NAI::SConvexHull hull(points, triangles, transform, source, user_id, {});
  renderer->TraceEntity(hull, terrain);
}

}  // namespace

int main() {
  CObj<CTestSource> source = new CTestSource;
  CObj<CTestSource> other_source = new CTestSource;
  NAI::CExplVoxelRenderer::CObjectsHash objects;
  int next_object_id = 0;
  NAI::CExplVoxelRenderer renderer;
  renderer.Init(CVec3(0, 0, 0), 8.0f, 16, &objects, &next_object_id);
  renderer.InitParallel(NAI::AXIS_Z);

  TraceTriangle(&renderer, source, 21, CVec3(-3, -3, 0), false);
  TraceTriangle(&renderer, source, 22, CVec3(1, 1, 0), false);
  TraceTriangle(&renderer, other_source, 21, CVec3(1, -3, 0), false);
  TraceTriangle(&renderer, source, 21, CVec3(-3, -3, 0), false);
  TraceTriangle(&renderer, nullptr, 0, CVec3(-3, 1, 0), true);

  if (next_object_id != 5 || objects.size() != 4) return 1;
  const auto first = objects.find(NAI::SVoxelObjectKey(source, 21));
  const auto second = objects.find(NAI::SVoxelObjectKey(source, 22));
  const auto third = objects.find(NAI::SVoxelObjectKey(other_source, 21));
  if (first == objects.end() || second == objects.end() || third == objects.end() ||
      first->second.nObjectID != 2 || second->second.nObjectID != 3 ||
      third->second.nObjectID != 4)
    return 2;

  int counts[5] = {};
  for (int z = 0; z < renderer.voxels.GetZSize(); ++z)
    for (int y = 0; y < renderer.voxels.GetYSize(); ++y)
      for (int x = 0; x < renderer.voxels.GetXSize(); ++x) {
        const unsigned id = renderer.voxels[z][y][x].nObject;
        if (id >= 5) return 3;
        ++counts[id];
      }
  if (counts[1] != 6 || counts[2] != 6 || counts[3] != 6 ||
      counts[4] != 6) return 4;
  std::printf("native explosion voxels: terrain=%d object21=%d object22=%d other21=%d\n",
              counts[1], counts[2], counts[3], counts[4]);
  return 0;
}
