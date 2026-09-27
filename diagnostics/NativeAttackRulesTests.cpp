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
	CObj<NDb::CRPGMaterial> material(new NDb::CRPGMaterial);
	CObj<NDb::CRPGArmor> armor(new NDb::CRPGArmor);
	material->fDensity = 2.5f;
	armor->pMaterial = material;
	if (!Near(NRPG::GetAPASubstraction(2.0f, 5.0f, armor), 7.5f)) return 9;
	if (!Near(NRPG::GetAPASubstraction(5.0f, 2.0f, armor), -7.5f)) return 10;
	int kinetic = 20;
	if (!NRPG::ApplyAPASubstraction(&kinetic, 2.0f, 5.0f, armor) || kinetic != 12) return 11;
	if (!NRPG::ApplyAPASubstraction(&kinetic, 5.0f, 2.0f, armor) || kinetic != 19) return 12;
	if (NRPG::ApplyAPASubstraction(&kinetic, 2.0f, 1.0e10f, armor) || kinetic != 0) return 13;
	kinetic = 20;
	if (NRPG::ApplyAPASubstraction(&kinetic, 1.0e10f, 0.0f, armor) || kinetic != 0) return 14;
	std::puts("native attack rules: push=0,0.28,1.5 ricochet=0,0,1");
	return 0;
}
