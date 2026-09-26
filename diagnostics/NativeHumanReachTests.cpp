#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
namespace NWorld
{
bool IsWithinHumanReach(const CVec3 &from, const CVec3 &target, float planeDistance);
}

#include <cstdio>

int main()
{
	const CVec3 from(0, 0, 0);
	if (!NWorld::IsWithinHumanReach(from, CVec3(1, 0, 0), 1)) return 1;
	if (NWorld::IsWithinHumanReach(from, CVec3(1.01f, 0, 0), 1)) return 2;
	if (NWorld::IsWithinHumanReach(from, CVec3(0, 0, -0.51f), 1)) return 3;
	if (NWorld::IsWithinHumanReach(from, CVec3(0, 0, 2.01f), 1)) return 4;
	if (!NWorld::IsWithinHumanReach(from, CVec3(0, 0, 2), 1)) return 5;
	std::puts("native human reach: horizontal and vertical limits");
	return 0;
}
