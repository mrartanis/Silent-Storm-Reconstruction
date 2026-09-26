#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../DBFormat/DataFormat.h"
#include "../Main/RPGUnitInfo.h"
#include "../Main/RPGAttackMech.h"

#include <cmath>
#include <cstdio>

static bool Near(float a, float b)
{
	return std::fabs(a - b) < 0.0001f;
}

int main()
{
	NRPG::CAttackPortion hit;
	if (!Near(hit.GetPushCorpseCoeff(), 0)) return 1;
	hit.fPushCoeff = 0.17f;
	if (!Near(hit.GetPushCorpseCoeff(), 0)) return 2;
	hit.fPushCoeff = 0.18f;
	if (!Near(hit.GetPushCorpseCoeff(), 0.28f)) return 3;
	hit.fPushCoeff = 2.0f;
	if (!Near(hit.GetPushCorpseCoeff(), 1.5f)) return 4;
	hit.nDmgType = 0;
	if (hit.CanRicochet()) return 5;
	hit.nDmgType = 5;
	if (hit.CanRicochet()) return 6;
	hit.nDmgType = 1;
	if (!hit.CanRicochet()) return 7;
	CRay ray;
	ray.ptOrigin = CVec3(1, 2, 3);
	ray.ptDir = CVec3(0, 1, 0);
	hit.MakeClickOfDeath(ray);
	if (hit.atkType != NRPG::AT_CLICK_OF_DEATH || hit.nK != 1000000 ||
		hit.nDmgMin != 10000000 || hit.nDmgMax != 10000001 ||
		!Near(hit.rTtrajectory.ptOrigin.x, 1)) return 8;
	std::puts("native attack rules: push=0,0.28,1.5 ricochet=0,0,1");
	return 0;
}
