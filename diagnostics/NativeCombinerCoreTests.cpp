#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/BasicChunk1.h"
#include "../Main/GCombiner.h"

#include <cstdint>
#include <cstdio>

namespace {
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
