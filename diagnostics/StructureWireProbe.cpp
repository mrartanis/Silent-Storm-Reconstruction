#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"

#include <cstdio>
#include <string>

class CWireProbeNode: public CObjectBase
{
  OBJECT_BASIC_METHODS(CWireProbeNode);
public:
  int value;
  CWireProbeNode(): value(0) {}
  virtual int operator&(CStructureSaver& f) { f.Add(2, &value); return 0; }
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
    CFileStream file;
    if (mode == "write")
    {
      first = new CWireProbeNode;
      second = new CWireProbeNode;
      first->value = 42;
      second->value = 77;
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
  if (!first || !second || !alias || first->value != 42 ||
      second->value != 77 || alias.GetBarePtr() != second.GetBarePtr())
  {
    std::fprintf(stderr, "structure wire contents mismatch\n");
    return 4;
  }
  std::printf("first=%d second=%d alias=1\n", first->value, second->value);
  return 0;
}
