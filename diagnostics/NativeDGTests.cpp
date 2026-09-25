#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#endif
#include "../Main/DG.H"

#include <cstdint>
#include <cstdio>

extern int nDGIncFrameRnd;

namespace {
class CounterNode : public CFuncBase<int> {
  OBJECT_BASIC_METHODS(CounterNode);
 public:
  CounterNode() { value = 0; }
  int Current() const { return value; }
 protected:
  bool NeedUpdate() override { return true; }
  void Recalc() override { ++value; }
};
}

int main() {
  if (nDGCurrentFrame != 100) return 1;
  CObj<CounterNode> node = new CounterNode;
  CDGPtr<CounterNode> ptr(node.GetPtr());
  if (!ptr.Refresh() || node->Current() != 1 || !node->WasRefreshed()) return 2;
  if (ptr.Refresh() || node->Current() != 1) return 3;
  node->Updated();
  if (!ptr.Refresh() || node->Current() != 1) return 4;
  MarkNewDGFrame();
  if (nDGCurrentFrame != 101 || !ptr.Refresh() || node->Current() != 2) return 5;

  std::uint32_t expectedRng = 1234567u;
  int due = -1;
  for (int i = 0; i < 6; ++i) {
    expectedRng = expectedRng * 13u + 2457823u;
    SetToHoldQueue(node.GetPtr(), &due);
    if (due != nDGCurrentFrame + int(expectedRng & 63u) + 64) return 6;
  }
  if (static_cast<std::uint32_t>(nDGIncFrameRnd) != expectedRng) return 7;
  ClearHoldQueue();
  if (node->Current() != 2) return 8;
  std::printf("frame=%d value=%d rng=%u due=%d\n",
              nDGCurrentFrame, node->Current(), expectedRng, due);
  return 0;
}
