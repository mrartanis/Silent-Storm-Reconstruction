#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../FileIO/BasicChunk1.h"
#include "../Main/GSceneInternal.h"

#include <cstdint>
#include <cstdio>

int main() {
  if (!pSSClasses ||
      pSSClasses->GetTypeID(static_cast<NGScene::CNonePart*>(nullptr)) != 0x02662160)
    return 1;
  CObj<NGScene::CNonePart> part = new NGScene::CNonePart;
  CMemoryStream stream;
  {
    CStructureSaver saver(stream, CStructureSaver::WRITE);
    saver.Add(1, &part);
  }
  std::uint64_t digest = UINT64_C(14695981039346656037);
  for (int i = 0; i < stream.GetSize(); ++i) {
    digest ^= stream.GetBuffer()[i];
    digest *= UINT64_C(1099511628211);
  }
  part = nullptr;
  stream.SetRMode();
  stream.Seek(0);
  {
    CStructureSaver saver(stream, CStructureSaver::READ);
    saver.Add(1, &part);
  }
  if (!part) return 2;
  std::printf("scene part bytes=%d fnv=%016llX\n", stream.GetSize(),
    static_cast<unsigned long long>(digest));
  return 0;
}
