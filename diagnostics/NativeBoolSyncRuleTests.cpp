#include "../Main/StdAfx.h"
#include "../Main/DG.h"
#include "../Main/Sync.h"

int main() {
  for (int mask = 0; mask < 4; ++mask) {
    if (CUnionFunc::GetResult(mask) != (mask != 0) ||
        CIntersectionFunc::GetResult(mask) != (mask == 3) ||
        CSubtractFunc::GetResult(mask) != (mask == 1)) return 1;
  }
  return 0;
}
