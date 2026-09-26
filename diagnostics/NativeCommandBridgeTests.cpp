#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/wInterface.h"
#include "../Main/wUnitCommands.h"

#include <cstdio>

int main()
{
	CObj<NWorld::CCmdSetCommand> optional =
		new NWorld::CCmdSetCommand( nullptr, new NWorld::CCmdContinue );
	CObj<NWorld::CCmdSetCommand> required =
		new NWorld::CCmdSetCommand( nullptr, new NWorld::CCmdStartCombat );
	if ( !optional->IsSkippable() || required->IsSkippable() ) return 1;
	std::puts( "native command bridge skip policy: passed" );
	return 0;
}
