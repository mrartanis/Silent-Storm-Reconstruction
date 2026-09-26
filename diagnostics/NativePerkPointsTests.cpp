#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../DBFormat/DataFormat.h"
#include "../Main/rpgPerk.h"

#include <cstdio>

int main()
{
	CObj<NRPG::CPerksTree> tree = new NRPG::CPerksTree(-1);
	if (tree->GetPerkPoints() != 0) return 1;
	tree->AddPerkPoints(3);
	if (tree->GetPerkPoints() != 3) return 2;
	tree->AddPerkPoints(-2);
	if (tree->GetPerkPoints() != 1) return 3;
	tree->AddPerkPoints(-5);
	if (tree->GetPerkPoints() != 0) return 4;
	tree->AddPerkPoints(2);
	tree->TakeRandomPerks(); // Empty detached tree has no available perks.
	if (tree->GetPerkPoints() != 2) return 5;
	std::puts("native perk points: 0,3,1,0,2");
	return 0;
}
