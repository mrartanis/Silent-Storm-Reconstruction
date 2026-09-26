#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../Misc/Geom.h"
#endif

namespace NWorld
{
bool IsWithinHumanReach( const CVec3 &ptFrom, const CVec3 &ptTarget, float fPlaneDist )
{
	if ( sqr( ptFrom.x - ptTarget.x ) + sqr( ptFrom.y - ptTarget.y ) > sqr( fPlaneDist ) )
		return false;
	if ( ptTarget.z < ptFrom.z - 0.5 || ptTarget.z > ptFrom.z + 2 )
		return false;
	return true;
}
}
