#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "aiPosition.h"
#include "aiWaypoint.h"
#include "../Misc/BasicShare.h"
#if !defined(_WIN32)
static void OutputDebugString( const char *message ) { std::fputs( message, stderr ); }
#endif
CBasicShare<int, NAI::CWaypointLoader> shareWaypoints(133);
CBasicShare<int, NAI::CUnitAIInfoLoader> shareUnits(134);
CBasicShare<int, NAI::CUnitGroupAIInfoLoader> shareUnitGroups(144);
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NAI
{
////////////////////////////////////////////////////////////////////////////////////////////////////
SCommand::SCommand(): cmd(CMD_POSE), ptPos(VNULL3), time(0), pose(WALK), dir(UP)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CWaypointLoader
////////////////////////////////////////////////////////////////////////////////////////////////////
void CWaypointLoader::Recalc()
{
	try
	{
		NGScene::CResourceOpener file( "Waypoints", GetKey() );
		
		pValue = new CWaypoint;
		pValue->operator&( *(file.operator->()) );
	}
	catch(...)
	{
		OutputDebugString( "Exception: CWaypointLoader::Recalc()\n" );
		return;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CWaypoint
////////////////////////////////////////////////////////////////////////////////////////////////////
CWaypoint::CWaypoint(): ptPos(VNULL3), nFloor(0), fRotation(0)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CUnitAIInfoLoader
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUnitAIInfoLoader::Recalc()
{
	try
	{
		NGScene::CResourceOpener file( "Units", GetKey() );
		
		pValue = new CUnitAIInfo;
		pValue->operator&( *(file.operator->()) );
	}
	catch(...)
	{
		//OutputDebugString( "Exception: CUnitAIInfoLoader::Recalc()\n" );
		return;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CUnitGroupAIInfoLoader
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUnitGroupAIInfoLoader::Recalc()
{
	try
	{
		NGScene::CResourceOpener file( "Groups", GetKey() );
		
		pValue = new CUnitAIInfo;
		pValue->operator&( *(file.operator->()) );
	}
	catch(...)
	{
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace NAI;
REGISTER_SAVELOAD_CLASS( 0xA2722170, CWaypoint )
REGISTER_SAVELOAD_CLASS( 0xA2722171, CUnitAIInfo )
REGISTER_SAVELOAD_CLASS( 0xA2722172, CUnitAIInfoLoader )
REGISTER_SAVELOAD_CLASS( 0xA2722173, CWaypointLoader )
REGISTER_SAVELOAD_CLASS( 0xA11A2141, CUnitGroupAIInfoLoader )
