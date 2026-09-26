#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/RPGItemMap.h"

#include <cstdio>

int main()
{
	CObj<NRPG::CItemsMap> map(new NRPG::CItemsMap(10, 13, true));
	CTPoint<int> size = map->GetSize();
	if (size.x != 10 || size.y != 13 || !map->GetItems().empty()) return 1;
	map->SetSize(12, 15);
	size = map->GetSize();
	if (size.x != 12 || size.y != 15) return 2;
	map->Clear(8, 9);
	size = map->GetSize();
	if (size.x != 8 || size.y != 9 || !map->GetItems().empty()) return 3;
	std::puts("native RPG item map: construct, resize, clear");
	return 0;
}
