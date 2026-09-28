#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

static bool CheckBoolVectorWire() {
  CMemoryStream stream;
  std::vector<bool> flags = {true, false, true, true, false, false, true, false};
  {
    CStructureSaver saver(stream, CStructureSaver::WRITE);
    saver.Add(1, &flags);
  }
  std::uint64_t hash = UINT64_C(14695981039346656037);
  const auto* bytes = static_cast<const std::uint8_t*>(stream.GetBuffer());
  for (int i = 0; i < stream.GetSize(); ++i)
    hash = (hash ^ bytes[i]) * UINT64_C(1099511628211);
  std::printf("bool-vector-wire bytes=%d fnv64=%016llx\n", stream.GetSize(),
              static_cast<unsigned long long>(hash));
  if (stream.GetSize() != 30 || hash != UINT64_C(0x671b29bcf33452a7))
    return false;
  stream.Seek(0);
  std::vector<bool> restored;
  {
    CStructureSaver saver(stream, CStructureSaver::READ);
    saver.Add(1, &restored);
  }
  return restored == flags;
}

class CWireProbeNode: public CObjectBase
{
  OBJECT_BASIC_METHODS(CWireProbeNode);
public:
  int value;
  CVec3 position;
  CQuat rotation;
  SPlane plane;
  SHMatrix matrix;
  CWireProbeNode(): value(0), position(0, 0, 0), rotation(0, 0, 0, 0),
      plane(CVec3(0, 0, 0), 0) {
    const float zeros[16] = {};
    S2FileIO::AssignMatrixFields(&matrix, zeros);
  }
  virtual int operator&(CStructureSaver& f) {
    f.Add(2, &value);
    f.Add(3, &position);
    f.Add(4, &rotation);
    f.Add(5, &plane);
    f.Add(6, &matrix);
    return 0;
  }
};

REGISTER_SAVELOAD_CLASS(0xA5923171, CWireProbeNode);

int main(int argc, char** argv)
{
  if (argc != 3) return 2;
  const std::string mode(argv[1]);
  if (mode != "write" && mode != "read") return 2;
  CObj<CWireProbeNode> first;
  CObj<CWireProbeNode> second;
  CPtr<CWireProbeNode> alias;
  try
  {
    if (!CheckBoolVectorWire()) return 5;
    CFileStream file;
    if (mode == "write")
    {
      first = new CWireProbeNode;
      second = new CWireProbeNode;
      first->value = 42;
      second->value = 77;
      first->position = CVec3(1.5f, -2.0f, 3.0f);
      second->position = CVec3(4.0f, 5.0f, 6.0f);
      first->rotation = CQuat(0.25f, -0.5f, 0.75f, 1.0f);
      second->rotation = CQuat(-1.0f, 0.0f, 0.5f, 0.25f);
      first->plane = SPlane(CVec3(2.0f, 3.0f, 4.0f), -5.0f);
      second->plane = SPlane(CVec3(-2.0f, -3.0f, -4.0f), 5.0f);
      first->matrix._11 = 11.0f; first->matrix._44 = 44.0f;
      second->matrix._11 = -11.0f; second->matrix._44 = -44.0f;
      alias = second;
      file.OpenWrite(argv[2]);
    }
    else file.OpenRead(argv[2]);
    {
      CStructureSaver saver(file, mode == "write" ? CStructureSaver::WRITE : CStructureSaver::READ);
      saver.Add(1, &first);
      saver.Add(2, &second);
      saver.Add(3, &alias);
    }
  }
  catch (...)
  {
    std::fprintf(stderr, "structure wire I/O failed\n");
    return 3;
  }
  float firstRotation[4] = {}, secondRotation[4] = {};
  if (first) first->rotation.GetComponentsForWire(firstRotation);
  if (second) second->rotation.GetComponentsForWire(secondRotation);
  if (!first || !second || !alias || first->value != 42 ||
      second->value != 77 || alias.GetBarePtr() != second.GetBarePtr() ||
      first->position.x != 1.5f || first->position.y != -2.0f ||
      second->position.z != 6.0f || firstRotation[0] != 0.25f ||
      secondRotation[3] != 0.25f || first->plane.n.y != 3.0f ||
      second->plane.d != 5.0f || first->matrix._11 != 11.0f ||
      second->matrix._44 != -44.0f)
  {
    std::fprintf(stderr, "structure wire contents mismatch\n");
    return 4;
  }
  std::printf("first=%d second=%d alias=1\n", first->value, second->value);
  return 0;
}
