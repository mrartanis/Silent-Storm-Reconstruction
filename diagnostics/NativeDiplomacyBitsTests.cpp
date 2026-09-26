#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../DBFormat/DataMap.h"
#include "../Main/RPGDiplomacy.h"

#include <cstdio>

int main()
{
	NRPG::SDiplomacy diplomacy;
	diplomacy.SetDiplomacyState(15, NDb::DS_ALLY);
	diplomacy.SetDiplomacyState(7, NDb::DS_ALLY);
	diplomacy.SetDiplomacyState(0, NDb::DS_NEUTRAL);
	if (diplomacy.GetDiplomacy() != 0x80008001u) return 1;
	if (diplomacy.GetDiplomacyState(15) != NDb::DS_ALLY) return 2;
	if (diplomacy.GetDiplomacyState(7) != NDb::DS_ALLY) return 3;
	if (diplomacy.GetDiplomacyState(0) != NDb::DS_NEUTRAL) return 4;
	diplomacy.SetDiplomacyState(15, NDb::DS_ENEMY);
	if (diplomacy.GetDiplomacy() != 0x00008001u) return 5;
	std::printf("native diplomacy bits: %08X,%08X\n", 0x80008001u,
		(unsigned)diplomacy.GetDiplomacy());
	return 0;
}
