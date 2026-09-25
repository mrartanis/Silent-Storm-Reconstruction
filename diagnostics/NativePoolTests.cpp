#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
class CStructureSaver;
#endif
#include "../Main/Pool.h"

#include <cstdio>

int main() {
  CPool<int, 4> pool;
  CPool<int, 4>::SIterator before(&pool);
  for (int value = 0; value != 10; ++value)
    *pool.Alloc() = value;
  CPool<int, 4>::SIterator it(&pool);
  for (int value = 9; value >= 0; --value, --it)
    if (it == before || *it.p != value) return 1;
  if (it != before) return 2;
  pool.Clear();
  if (CPool<int, 4>::SIterator(&pool) != before) return 3;
  *pool.Alloc() = 42;
  if (*CPool<int, 4>::SIterator(&pool).p != 42) return 4;
  std::printf("pool count=10 blocks=3 tail=42\n");
  return 0;
}
